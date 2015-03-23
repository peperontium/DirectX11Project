#include "cmoMesh.h"


#include <algorithm>

#include "cmoLoader.h"

#include "../dx11//DX11ThinWrapper.h"
#include "../dx11/DX11GlobalDevice.h"



using namespace DirectX;
using namespace DX11ThinWrapper;


namespace {
		
	//	モデル名を除いてディレクトリ名だけを取り出す
	std::wstring extractDirectoryPath(const wchar_t* modelPath) {
		std::wstring path = modelPath;


		auto find = path.find_last_of(L"\\");
		if (find == std::wstring::npos) {
			//	区切り文字は二種類試す
			find = path.find_last_of(L"/");

			//	見つからなかったらカレントディレクトリ
			if (find == std::wstring::npos)
				return std::wstring(L".\\");
		}

		path = std::move(path.substr(0,find + 1));

		return path;
	}


	//	とりあえず線形補完
	void interpolateAnimationKeyframe(
		XMFLOAT4X4* outMtx, float time,
		const std::map<float, cmo::element::Keyframe>& keyframeArray) {
		
		using namespace DirectX;

		//	クォータニオンは正規化したものを渡すこと（読み込み時に正規化済みのはず）

		if (keyframeArray.count(time) != 0) {
			//	そのフレームのキーが存在する時
			auto kf = keyframeArray.at(time);
			XMMATRIX scaleMtx = XMMatrixScaling(kf.Scaling.x, kf.Scaling.y, kf.Scaling.z);
			XMMATRIX rotMtx   = XMMatrixRotationQuaternion(XMLoadFloat4(&kf.RotationQt));

			XMStoreFloat4x4(outMtx,scaleMtx*rotMtx);
			outMtx->_41 = kf.Translation.x;
			outMtx->_42 = kf.Translation.y;
			outMtx->_43 = kf.Translation.z;

		} else {
			//	そのフレームのキーがない時、補完で行列算出
			auto first  = keyframeArray.upper_bound(time);
			auto second = first--;

			//	first*(1-ratio) + second*ratio	でキーフレーム混ぜる
			float ratio = (time - first->first) / (second->first - first->first);

			XMMATRIX scaleMtx = XMMatrixScaling(
				first->second.Scaling.x*(1 - ratio) + second->second.Scaling.x*ratio,
				first->second.Scaling.y*(1 - ratio) + second->second.Scaling.y*ratio,
				first->second.Scaling.z*(1 - ratio) + second->second.Scaling.z*ratio
				);
			XMMATRIX rotMtx = XMMatrixRotationQuaternion(
					XMQuaternionSlerp(
						XMLoadFloat4(&first->second.RotationQt),
						XMLoadFloat4(&second->second.RotationQt),
						ratio
					)
				);
			XMStoreFloat4x4(outMtx, scaleMtx*rotMtx);
			outMtx->_41 = first->second.Translation.x*(1 - ratio) + second->second.Translation.x*ratio;
			outMtx->_42 = first->second.Translation.y*(1 - ratio) + second->second.Translation.y*ratio;
			outMtx->_43 = first->second.Translation.z*(1 - ratio) + second->second.Translation.z*ratio;
		}
	}
}

//	Rigid Model
namespace cmo {
	//	実体定義
	std::weak_ptr<ID3D11Buffer>		Mesh::s_mtxConstBufferShared;

	//------------------------
	//	protected functions
	//------------------------
	void Mesh::_InitMatrixBuffer(ID3D11Device *device) {

		//	作成済みの共用バッファがあるならそれを用いる
		_mtxConstBuffer = Mesh::s_mtxConstBufferShared.lock();

		if (_mtxConstBuffer == nullptr) {
			//	未作成の場合は作る
			XMStoreFloat4x4(&_mtxWorld, XMMatrixIdentity());
			_mtxConstBuffer = d3::CreateConstantBuffer(device, &_mtxWorld, sizeof(XMFLOAT4X4), D3D11_CPU_ACCESS_WRITE);
			Mesh::s_mtxConstBufferShared = _mtxConstBuffer;
		} else {
			//	バッファ作成済みの場合単位行列で初期化
			this->updateMatrix(XMMatrixIdentity());
		}
	}

	//	コピー用代入演算子
	Mesh& Mesh::operator=(const Mesh& source) {

		/******************
		 *	shallow-copy類
		 ******************/
		//	バッファ周り
		this->_submeshArray		 = source._submeshArray;
		this->_vertexBufferArray = source._vertexBufferArray;
		this->_indexBufferArray  = source._indexBufferArray;

		//	シェーダー
		this->_vertexShader = source._vertexShader;
		this->_pixelShader	= source._pixelShader;
		this->_inputLayout	= source._inputLayout;

		//	メッシュ情報
		this->_extent	= source._extent;
		this->_name		= source._name;


		/******************
		*	deep-copy類
		******************/
		this->_materialArray.reserve(source._materialArray.size());
		for (auto& mat :  source._materialArray) {
			this->_materialArray.emplace_back(std::make_shared<cmo::element::Material>(*mat));
		}
		this->_materialIndexTable= source._materialIndexTable;
		


		//	行列データを移す。
		//	デバイスはグローバルから引っ張ってくる。許して
		this->_InitMatrixBuffer(dx11::AccessDX11Device());
		this->updateMatrix(
			XMLoadFloat4x4(&source._mtxWorld));


		return(*this);	
	}



	//------------------------
	//	public functions
	//------------------------
	HRESULT Mesh::load(ID3D11Device* device, const wchar_t* filePath, const wchar_t* textureDirectory) {
		
		std::wstring textureDir;
		if (textureDirectory == nullptr) {
			textureDir = extractDirectoryPath(filePath);
		} else {
			textureDir = textureDirectory;
		}


		//	すでに持っているメッシュ情報削除
		_ClearBuffers();


		//	メッシュ情報読み込んでいく
		loader::BinaryReader binaryReader;
		HRESULT hr = binaryReader.importFile(filePath);
		if (FAILED(hr))
			return hr;


		auto numMesh = binaryReader.read<UINT>();

		if (*numMesh == 0) {
			OutputDebugString(TEXT("\n--------------------\nERROR: No meshes found\n--------------------\n"));
			return E_FAIL;
		}

		
//		_meshArray.resize(*numMesh);
//		_pMeshIndexTable = std::make_shared<std::unordered_map<std::wstring, unsigned int> >();
//		_pMeshIndexTable->reserve(*numMesh);

		//	メッシュを一つだけ読み込む
		//	本来はメッシュの数だけループ
		for (size_t meshId = 0; meshId < *numMesh; ++meshId) {
			
//			auto& currentMesh = _meshArray[meshId];
//			auto& currentMesh = _pMesh;
//			currentMesh = std::make_shared<cmo::Mesh>();


			//	メッシュ名取得
			auto _name = loader::getString(&binaryReader);
			if (_name.empty())
				return E_FAIL;



			//	マテリアル読み込み
			if ( ! loader::getMaterialArray(&_materialArray, &_materialIndexTable, device, textureDir.c_str(), &binaryReader))
				return E_FAIL;


			// ボーン情報あるかどうか
			bool hasSkeleton = false;
			{
				auto bSkeleton = binaryReader.read<BYTE>();
				if (bSkeleton == nullptr)
					return E_FAIL;

			}


			//	サブメッシュ読み込み
			_submeshArray = loader::getSubMeshArray(&binaryReader);
			if (_submeshArray.empty())
				return E_FAIL;


			// インデックスバッファ読み込みおよび作成
			_indexBufferArray = loader::getIndexBufferArray(device, &binaryReader);
			if (_indexBufferArray.empty())
				return E_FAIL;


			// 頂点バッファ読み込みおよび作成
			_vertexBufferArray = loader::getVertexBufferArray(device, &binaryReader);
			if (_vertexBufferArray.empty())
				return E_FAIL;


			// Skinning vertex buffers
			//	読み飛ばします
			loader::skipSkinningVertexData(&binaryReader);


			// メッシュ情報	Extents
			_extent = loader::getMeshInfo(&binaryReader);
			if (_extent->Radius < 0)
				return E_FAIL;

			//	アニメーション、読み飛ばす
			if (hasSkeleton) {
				if (!loader::skipBoneAndAnimation(&binaryReader))
					return E_FAIL;
			}


			//	とりあえず一つだけ
			break;

		}	//	end of loop (*numMesh)


		//	シェーダーへ行列送るためのバッファ初期化
		_InitMatrixBuffer(device);


		return S_OK;
	}

	
	void Mesh::applyVS(std::shared_ptr<ID3D11VertexShader>& vertexShader, std::weak_ptr<ID3DBlob> blob, ID3D11Device *device) {

		auto p = blob.lock();
		if (p == nullptr)
			return;

		static D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT	, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL"	, 0, DXGI_FORMAT_R32G32B32_FLOAT	, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TANGENT"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR"	, 0, DXGI_FORMAT_R8G8B8A8_UNORM		, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT		, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		_inputLayout = d3::CreateInputLayout(device, layout, _countof(layout), p.get());
		_vertexShader = vertexShader;
	}


	void Mesh::replaceMaterial(const wchar_t* dstMatName, std::shared_ptr<cmo::element::Material> &srcMaterial, const wchar_t* srcMatName) {
#if (DEBUG || _DEBUG)
		bool update = false;
#endif
		auto it = _materialIndexTable.find(dstMatName);
		if (it != _materialIndexTable.end()) {
			//	マテリアル更新
			unsigned int index = _materialIndexTable[dstMatName];
			_materialArray[index] = srcMaterial;

			//	マテリアル→インデックスのテーブル更新
			_materialIndexTable.erase(it);
			_materialIndexTable.emplace(srcMatName, index);

#if (DEBUG || _DEBUG)
			update = true;
#endif	
		}
			
		
		assert(("更新対象のマテリアルが見つかりません", update));

	}

	//	ループぶん回す以外に方法無くないですかコレ
	void Mesh::replaceTexture(const wchar_t* dstTextureName, std::shared_ptr<ID3D11ShaderResourceView> &srcTexture, const wchar_t* srcTextureName) {
#if (DEBUG || _DEBUG)
		bool update = false;
#endif

		for (auto& mat : _materialArray) {
			for (auto& texture : mat->TextureArray) {
				if (texture.textureName == dstTextureName) {
					//	テクスチャ更新
					texture.pTexture	= srcTexture;
					texture.textureName = srcTextureName;
#if (DEBUG || _DEBUG)
					update = true;
#endif
				}
			}
		}
		

		assert(( "更新対象のテクスチャが見つかりません",update));

	}

	void Mesh::render(ID3D11DeviceContext* context, UINT startSlot)const {

		assert(_vertexShader != nullptr);
		assert(_pixelShader != nullptr);

		//	行列更新
		d3::mapping(_mtxConstBuffer.get(), context, [&](D3D11_MAPPED_SUBRESOURCE resource) {
			auto param = (XMFLOAT4X4*)(resource.pData);
			XMStoreFloat4x4(param,
				XMMatrixTranspose(axis::getAxisConvertMatrix(axis::ModelCoordinate::Blender)*XMLoadFloat4x4(&_mtxWorld))
				);
		});


		//	入力レイアウト、描画方式の設定
		context->IASetInputLayout(_inputLayout.get());
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		
		//	シェーダー設定
		context->VSSetShader(_vertexShader.get(), nullptr, 0);
		context->PSSetShader(_pixelShader.get() , nullptr, 0);

		//	ワールド変換行列設定
		ID3D11Buffer * constBuffers[] = { _mtxConstBuffer.get() };
		//	↓ここの第一引数（設定対象のバッファの場所）はシェーダーに応じて変更する必要あり
		context->VSSetConstantBuffers(startSlot, 1, constBuffers);



		//	描画

		// Loop over each submesh.
		for (auto& submesh : _submeshArray) {

			if (submesh->IndexBufferIndex < _indexBufferArray.size() &&
				submesh->VertexBufferIndex < _vertexBufferArray.size()) {

				//	頂点バッファ、インデックスバッファの設定
				UINT stride = sizeof(cmo::element::Vertex);
				UINT offset = 0;
				ID3D11Buffer* vb[] = { _vertexBufferArray[submesh->VertexBufferIndex].get() };
				context->IASetVertexBuffers(0, 1, vb, &stride, &offset);
				context->IASetIndexBuffer(_indexBufferArray[submesh->IndexBufferIndex].get(), IndexFormatof<cmo::IndexType>::Vertex, 0);
			}

			if (submesh->MaterialIndex < _materialArray.size()) {
				cmo::element::Material* pMaterial = _materialArray[submesh->MaterialIndex].get();

				/*********************************************************
				*	必要に応じてマテリアル情報をシェーダーに送ってください
				*	material.Ambientなどなど。
				**********************************************************/

				for (UINT texIndex = 0; texIndex < cmo::Max_Texture; ++texIndex) {
					ID3D11ShaderResourceView* texture = pMaterial->TextureArray[texIndex].pTexture.get();

					context->PSSetShaderResources(0 + texIndex, 1, &texture);
				}

				// Draw the Mesh
				context->DrawIndexed(submesh->PrimCount * 3, submesh->StartIndex, 0);
			}
		}


	}

}	// end of namespace cmo


//	Skinning Model
namespace cmo {
	//	実体
	std::weak_ptr<ID3D11Buffer>		SkinnedMesh::s_boneMtxConstBufferShared;

	//------------------------
	//	private functions
	//------------------------

	void SkinnedMesh::_InitMatrixBuffer(ID3D11Device *device) {

		//	作成済みの共用バッファがあるならそれを用いる
		_mtxConstBuffer = SkinnedMesh::s_boneMtxConstBufferShared.lock();

		if (_mtxConstBuffer == nullptr) {
			//	未作成の場合は作る
			XMStoreFloat4x4(&_mtxWorld, XMMatrixIdentity());
			_mtxConstBuffer = d3::CreateConstantBuffer(device, nullptr, sizeof(XMFLOAT4X4)*cmo::Max_Bone, D3D11_CPU_ACCESS_WRITE);
			SkinnedMesh::s_boneMtxConstBufferShared = _mtxConstBuffer;
		} else {
			//	バッファ作成済みの場合単位行列で初期化
			this->updateMatrix(XMMatrixIdentity());
		}
	}

	void SkinnedMesh::_UpdateBoneTransform() {
		
		float currentFrame = (_animFrame / 60.0f)*(_currentAnim->second.EndTime - _currentAnim->second.StartTime) + _currentAnim->second.StartTime;

		//	各ボーンのローカル変換行列取り出す
		const auto& KeyFrames = _currentAnim->second.BoneKeyframe;
		
		for (size_t boneId = 0; boneId < KeyFrames.size(); ++boneId)
			interpolateAnimationKeyframe(&_boneMtxArray[boneId],currentFrame,KeyFrames[boneId]);


		_isTransformCombined.assign((*_pBoneArray).size(), false);

		//	各ボーンのローカル変換を計算
		for (UINT b = 0; b < (*_pBoneArray).size(); ++b) {
			_CombineBoneTransforms(b);
		}
		
		XMMATRIX worldMtx = XMLoadFloat4x4(&_mtxWorld);

		//	逆姿勢×変換行列	で各ボーンのワールド変換行列を求める
		for (UINT b = 0; b < (*_pBoneArray).size(); ++b) {
			XMMATRIX invPose = XMLoadFloat4x4(&(*_pBoneArray)[b].InvBindPos);
			XMMATRIX transformMtx = XMLoadFloat4x4(&_boneMtxArray[b]);


			XMStoreFloat4x4(&_boneMtxArray[b],
				XMMatrixTranspose(
					(invPose * transformMtx)*axis::getAxisConvertMatrix(axis::ModelCoordinate::Blender) *worldMtx
				)
			);
		}

	}

	void SkinnedMesh::_CombineBoneTransforms(INT currentBoneIndex) {
		cmo::element::Bone& bone = (*_pBoneArray)[currentBoneIndex];

		//	ルートのボーン
		if (bone.ParentIndex < 0) {
			//XMMATRIX currentMtx = XMLoadFloat4x4(&_boneMtxArray[currentBoneIndex]);
			//XMMATRIX parentMtx = XMLoadFloat4x4(&_mtxWorld);

			//XMMATRIX ret = currentMtx * parentMtx;

			XMStoreFloat4x4(&_boneMtxArray[currentBoneIndex], XMLoadFloat4x4(&_boneMtxArray[currentBoneIndex]));


			_isTransformCombined[currentBoneIndex] = true;
			return;
		}

		if (_isTransformCombined[currentBoneIndex] || bone.ParentIndex == (currentBoneIndex)) {
			_isTransformCombined[currentBoneIndex] = true;
			return;
		}

		//	親ボーンをたどって再帰的に結合
		_CombineBoneTransforms(bone.ParentIndex);


		//	ワールドへの変換行列を算出
		//	cmoの元のfbx側が行優先のため転置してから計算
		XMMATRIX currentMtx = XMMatrixTranspose(XMLoadFloat4x4(&_boneMtxArray[currentBoneIndex]));
		XMMATRIX parentMtx = XMMatrixTranspose(XMLoadFloat4x4(&_boneMtxArray[bone.ParentIndex]));
				

		XMStoreFloat4x4(&_boneMtxArray[currentBoneIndex], XMMatrixTranspose(parentMtx * currentMtx));

		_isTransformCombined[currentBoneIndex] = true;
	}

	SkinnedMesh& SkinnedMesh::operator=(const SkinnedMesh& mesh) {
		 
		/******************
		*	shallow-copy類
		******************/
		this->_skinningVertexBufferArray = mesh._skinningVertexBufferArray;
		this->_pBoneArray		= mesh._pBoneArray;
		this->_pBoneIndexTable	= mesh._pBoneIndexTable;
		this->_pAnimationTable	= mesh._pAnimationTable;


		/******************
		*	deep-copy類
		******************/
		this->_animFrame	= mesh._animFrame;
		this->_boneMtxArray = mesh._boneMtxArray;
		this->_isTransformCombined = mesh._isTransformCombined;
		this->_currentAnim = mesh._currentAnim;

		//	基底クラスの物はそちらにコピー処理を任せる
		Mesh::operator=(mesh);
		
		return(*this);
		
	}


	//------------------------
	//	public override functions
	//------------------------
	HRESULT SkinnedMesh::load(ID3D11Device* device, const wchar_t* filePath, const wchar_t* textureDirectory) {

		std::wstring textureDir;
		if (textureDirectory == nullptr) {
			textureDir = extractDirectoryPath(filePath);
		} else {
			textureDir = textureDirectory;
		}


		//	すでに持っているメッシュ情報削除
		_ClearBuffers();


		//	メッシュ情報読み込んでいく
		loader::BinaryReader binaryReader;
		HRESULT hr = binaryReader.importFile(filePath);
		if (FAILED(hr))
			return hr;


		auto numMesh = binaryReader.read<UINT>();

		if (*numMesh == 0) {
			OutputDebugString(TEXT("\n--------------------\nERROR: No meshes found\n--------------------\n"));
			return E_FAIL;
		}


		//		_meshArray.resize(*numMesh);
		//		_pMeshIndexTable = std::make_shared<std::unordered_map<std::wstring, unsigned int> >();
		//		_pMeshIndexTable->reserve(*numMesh);

		//	メッシュを一つだけ読み込む
		//	本来はメッシュの数だけループ
		for (size_t meshId = 0; meshId < *numMesh; ++meshId) {

			//			auto& currentMesh = _meshArray[meshId];
			//			auto& currentMesh = _pMesh;
			//			currentMesh = std::make_shared<cmo::Mesh>();


			//	メッシュ名取得
			auto _name = loader::getString(&binaryReader);
			if (_name.empty())
				return E_FAIL;



			//	マテリアル読み込み
			if (!loader::getMaterialArray(&_materialArray, &_materialIndexTable, device, textureDir.c_str(), &binaryReader))
				return E_FAIL;


			// ボーン情報あるかどうか
			bool hasSkeleton = false;
			{
				auto bSkeleton = binaryReader.read<BYTE>();
				if (bSkeleton == nullptr)
					return E_FAIL;

				hasSkeleton = ((*bSkeleton) != 0);
			}


			//	サブメッシュ読み込み
			_submeshArray = loader::getSubMeshArray(&binaryReader);
			if (_submeshArray.empty())
				return E_FAIL;


			// インデックスバッファ読み込みおよび作成
			_indexBufferArray = loader::getIndexBufferArray(device, &binaryReader);
			if (_indexBufferArray.empty())
				return E_FAIL;


			// 頂点バッファ読み込みおよび作成
			_vertexBufferArray = loader::getVertexBufferArray(device, &binaryReader);
			if (_vertexBufferArray.empty())
				return E_FAIL;


			// Skinning vertex buffers
			if(!loader::getSkinningVertexBufferArray(&_skinningVertexBufferArray,device,&binaryReader))
				return E_FAIL;


			// メッシュ情報	Extents
			_extent = loader::getMeshInfo(&binaryReader);
			if (_extent->Radius < 0)
				return E_FAIL;

			//	アニメーションとボーン
			if (hasSkeleton) {
				if (!loader::getBoneAndAnimation(&_pBoneArray,&_pBoneIndexTable,&_pAnimationTable,&binaryReader))
					return E_FAIL;

				//	デフォルトのアニメーション設定
				_currentAnim = (*_pAnimationTable).begin();
				//	ボーン姿勢行列初期化
				_boneMtxArray.resize((*_pBoneArray).size());
				_isTransformCombined.assign((*_pBoneArray).size(), false);
			} else {
				OutputDebugStringW(L"No skeltal data found in (\n");
				OutputDebugStringW(filePath);
				OutputDebugStringW(L")\n");
				assert(false);
			}


			//	とりあえず一つだけ
			break;

		}	//	end of loop (*numMesh)


		//	シェーダーへ行列送るためのバッファ初期化
		_InitMatrixBuffer(device);


		return S_OK;
	}


	void SkinnedMesh::applyVS(std::shared_ptr<ID3D11VertexShader>& vertexShader, std::weak_ptr<ID3DBlob> blob, ID3D11Device *device) {
		using cmo::element::SkinningVertex;

		auto p = blob.lock();
		if (p == nullptr)
			return;

		static D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT	, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL"	, 0, DXGI_FORMAT_R32G32B32_FLOAT	, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TANGENT"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR"	, 0, DXGI_FORMAT_R8G8B8A8_UNORM		, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT		, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "BLENDINDICES", 0, IndexFormatof<SkinningVertex::IndexType>::Bone	, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "BLENDWEIGHT"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		
		_inputLayout = d3::CreateInputLayout(device, layout, _countof(layout), p.get());
		_vertexShader = vertexShader;
	}


	void SkinnedMesh::render(ID3D11DeviceContext* context, UINT startSlot)const {

		assert(_vertexShader != nullptr);
		assert(_pixelShader != nullptr);

		//	行列更新
		//	ラムダキャプチャ用の一時変数
		auto& boneMtxArray = this->_boneMtxArray;
		d3::mapping(_mtxConstBuffer.get(), context, [&](D3D11_MAPPED_SUBRESOURCE resource) {
			auto param = (XMFLOAT4X4*)(resource.pData);
			std::copy(_boneMtxArray.begin(), _boneMtxArray.end(), param);
		});


		//	入力レイアウト、描画方式の設定
		context->IASetInputLayout(_inputLayout.get());
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//	シェーダー設定
		context->VSSetShader(_vertexShader.get(), nullptr, 0);
		context->PSSetShader(_pixelShader.get(), nullptr, 0);

		//	ワールド変換行列設定
		ID3D11Buffer * boneBuffers[] = { _mtxConstBuffer.get() };
		//	↓ここの第一引数（設定対象のバッファの場所）はシェーダーに応じて変更する必要あり
		context->VSSetConstantBuffers(startSlot, 1, boneBuffers);


		//	描画

		// Loop over each submesh.
		for (auto& submesh : _submeshArray) {

			if (submesh->IndexBufferIndex < _indexBufferArray.size() &&
				submesh->VertexBufferIndex < _vertexBufferArray.size()) {

				//	スキニング用頂点バッファ、インデックスバッファの設定
				ID3D11Buffer* vbs[] =
				{
					_vertexBufferArray[submesh->VertexBufferIndex].get(),
					_skinningVertexBufferArray[submesh->VertexBufferIndex].get()
				};

				UINT stride[] = { sizeof(cmo::element::Vertex), sizeof(cmo::element::SkinningVertex) };
				UINT offset[] = { 0, 0 };
				context->IASetVertexBuffers(0, 2, vbs, stride, offset);
				context->IASetIndexBuffer(_indexBufferArray[submesh->IndexBufferIndex].get(), IndexFormatof<cmo::IndexType>::Vertex, 0);

			}

			if (submesh->MaterialIndex < _materialArray.size()) {
				cmo::element::Material* pMaterial = _materialArray[submesh->MaterialIndex].get();

				/*********************************************************
				*	必要に応じてマテリアル情報をシェーダーに送ってください
				*	material.Ambientなどなど。
				**********************************************************/

				for (UINT texIndex = 0; texIndex < cmo::Max_Texture; ++texIndex) {
					ID3D11ShaderResourceView* texture = pMaterial->TextureArray[texIndex].pTexture.get();

					context->PSSetShaderResources(0 + texIndex, 1, &texture); break;
				}

				// Draw the Mesh
				context->DrawIndexed(submesh->PrimCount * 3, submesh->StartIndex, 0);
			}
		}

		// Clear the extra vertex buffer.
		ID3D11Buffer* vbs[1] = { nullptr };
		UINT stride = 0;
		UINT offset = 0;
		context->IASetVertexBuffers(1, 1, vbs, &stride, &offset);

	}

}// end of namespace cmo