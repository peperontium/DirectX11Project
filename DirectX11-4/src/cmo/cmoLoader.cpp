#include "cmoLoader.h"

#include "cmoElement.h"

#include "../dx11/DX11ThinWrapper.h"


using namespace DX11ThinWrapper;

namespace {



	//	絶対パスからテクスチャ名をだけを抽出
	//	todo:コンソールでテクスチャ名のみを取り出して上書きする、これに似た仕様のものを作る
	std::wstring extractTextureName(const wchar_t* textureFullPath, const wchar_t* Extension = L".png") {
		std::wstring name = textureFullPath;
		
		std::wstring::size_type	find;

		//	__ を _ に置き換える、ディレクトリの区切りも_となっているため一旦マーカーに変更
		//	多分元が_一つだったら二つに変換されてる
		while (true) {
			find = name.find(L"__");
			if (find == std::wstring::npos)
				break;

			name.replace(find, 2, L"*");
		}

		//	ディレクトリを除いてテクスチャ名のみを取り出す
		find = name.find_last_of(L"_");
		if (find == std::wstring::npos)
			return std::wstring();

		name = std::move(name.substr(find + 1));


		//	さっき置き換えたマーカーを_に戻す
		while (true) {
			find = name.find_last_of(L"*");
			if (find == std::wstring::npos)
				break;

			name.replace(find, 1, L"_");
		}

		//	png.ddsを指定された拡張子に置き換える
		find = name.find(L".png.dds");
		if (find == std::wstring::npos)
			return name;

		name.replace(find, 8, Extension);

	
		return name;
	}

	//	変換行列からキーフレーム情報の抽出
	//	内部で値変更するから引数はコピーで
	inline cmo::element::Keyframe parseKeyframe(DirectX::XMFLOAT4X4 transformMtx) {
		using namespace DirectX;
		cmo::element::Keyframe kf;

		//	平行移動取り出し
		kf.Translation.x = transformMtx._41;
		kf.Translation.y = transformMtx._42;
		kf.Translation.z = transformMtx._43;

		//	スケーリング取り出し
		kf.Scaling.x = sqrt(pow(transformMtx._11, 2) + pow(transformMtx._12, 2) + pow(transformMtx._13, 2));
		kf.Scaling.y = sqrt(pow(transformMtx._21, 2) + pow(transformMtx._22, 2) + pow(transformMtx._23, 2));
		kf.Scaling.z = sqrt(pow(transformMtx._31, 2) + pow(transformMtx._32, 2) + pow(transformMtx._33, 2));

		//	行列のスケーリング成分を取り除く
		transformMtx._11 /= kf.Scaling.x;	transformMtx._12 /= kf.Scaling.x;	transformMtx._13 /= kf.Scaling.x;
		transformMtx._21 /= kf.Scaling.y;	transformMtx._22 /= kf.Scaling.y;	transformMtx._23 /= kf.Scaling.y;
		transformMtx._31 /= kf.Scaling.z;	transformMtx._32 /= kf.Scaling.z;	transformMtx._33 /= kf.Scaling.z;


		//	回転クォータニオン取り出し、正規化
		XMStoreFloat4(&kf.RotationQt,
			XMQuaternionNormalize(XMQuaternionRotationMatrix(XMLoadFloat4x4(&transformMtx)))
			);

		return kf;
	}


	//	読み込み用のマテリアル（element::Materialよりもsizeof(std::wstring)だけ小さい）
	struct MaterialInput {
		DirectX::XMFLOAT4   Ambient;
		DirectX::XMFLOAT4   Diffuse;
		DirectX::XMFLOAT4   Specular;
		float               SpecularPower;
		DirectX::XMFLOAT4   Emissive;
		DirectX::XMFLOAT4X4 UVTransform;
	};
	static_assert(sizeof(MaterialInput) == 132, "CMO Mesh structure size incorrect");

	//	読み込み用のスキニング頂点情報
	struct SkinningVertexInput {
		UINT boneIndex[cmo::Num_Bone_Influences];
		float boneWeight[cmo::Num_Bone_Influences];
	};
	static_assert(sizeof(SkinningVertexInput) == 32, "CMO Mesh structure size incorrect");

	//!	アニメーション読み込む時用の情報
	struct Clip {
		float StartTime;
		float EndTime;
		UINT  numAnimKeys;
	};
	static_assert(sizeof(Clip) == 12, "CMO Mesh structure size incorrect");


	//!	読み込み用アニメーションキーフレーム
	struct KeyframeInput {
		KeyframeInput() : BoneIndex(0), Time(0.0f) {}

		UINT BoneIndex;
		float Time;
		DirectX::XMFLOAT4X4 Transform;
	};
	static_assert(sizeof(KeyframeInput) == 72, "CMO Mesh structure size incorrect");
}


namespace cmo {

	namespace loader {

		HRESULT BinaryReader::importFile(const wchar_t* fileName) {

			struct handle_closer { void operator()(HANDLE h) { if (h) CloseHandle(h); } };
			typedef public std::unique_ptr<void, handle_closer> ScopedHandle;
			const static auto safe_handle = [](HANDLE h)-> HANDLE { return (h == INVALID_HANDLE_VALUE) ? 0 : h; };


			// open the file
			ScopedHandle hFile(safe_handle(CreateFileW(fileName,
				GENERIC_READ,
				FILE_SHARE_READ,
				nullptr,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				nullptr)));

			if (!hFile)
				return HRESULT_FROM_WIN32(GetLastError());

			// Get the file size
			LARGE_INTEGER FileSize = { 0 };

			FILE_STANDARD_INFO fileInfo;
			if (!GetFileInformationByHandleEx(hFile.get(), FileStandardInfo, &fileInfo, sizeof(fileInfo))) {
				return HRESULT_FROM_WIN32(GetLastError());
			}
			FileSize = fileInfo.EndOfFile;

			// File is too big for 32-bit allocation, so reject read
			if (FileSize.HighPart > 0) {
				return E_FAIL;
			}

			// Need at least enough data to contain the mesh count
			if (FileSize.LowPart < sizeof(UINT)) {
				return E_FAIL;
			}

			// create enough space for the file data
			_meshData.reset(new uint8_t[FileSize.LowPart]);
			if (_meshData == nullptr) {
				return E_OUTOFMEMORY;
			}

			// read the data in
			DWORD BytesRead = 0;
			if (!ReadFile(hFile.get(), _meshData.get(), FileSize.LowPart, &BytesRead, NULL)) {
				return HRESULT_FROM_WIN32(GetLastError());
			}

			if (BytesRead < FileSize.LowPart) {
				return E_FAIL;
			}

			_dataSize = FileSize.LowPart;

			return S_OK;
		}


		bool getMaterialArray(std::vector<std::shared_ptr<cmo::element::Material>> *materialArray, std::unordered_map<std::wstring, unsigned int> *materialIndexTable,
			ID3D11Device* device, const wchar_t* textureDirectory, BinaryReader * meshData) {
			
			//	Materials count
			const UINT* numMaterial = meshData->read<UINT>();

			if (numMaterial == nullptr)
				return false;


			materialArray->reserve(*numMaterial);
			materialIndexTable->reserve(*numMaterial);
			for (UINT j = 0; j < *numMaterial; ++j) {

				// Material name
				auto matName = loader::getString(meshData);
				//if (matName.empty())
				//	return false;

				// Material settings
				auto matSetting = meshData->read<MaterialInput>();

				if (matSetting == nullptr)
					return false;


				// Pixel shader name
				//	一旦使いません。使いたい人は各自で設定してね
				auto psName = loader::getString(meshData);


				auto pMat = std::make_shared<cmo::element::Material>();
//				pMat->Name = matName;
				materialIndexTable->emplace(matName,j);
				pMat->Ambient = matSetting->Ambient;
				pMat->Diffuse = matSetting->Diffuse;
				pMat->Specular = matSetting->Specular;
				pMat->SpecularPower = matSetting->SpecularPower;
				pMat->Emissive = matSetting->Emissive;

				//	マテリアル毎のUV変換情報。
				//	今回はシェーダー側でBlenderからとして決め打ちして使用していません。
				pMat->UVTransform		= matSetting->UVTransform;


				for (UINT t = 0; t < cmo::Max_Texture; ++t) {
					auto textureName = loader::getString(meshData);

					if (!textureName.empty()) {

						auto texName = extractTextureName(textureName.c_str());
						texName.insert(0, textureDirectory );

						pMat->TextureArray[t] = {
							d3::CreateWICTextureFromFile(device, texName.c_str()),
							texName
						};
					}
				}

				materialArray->emplace_back(pMat);
//				materialArray.emplace(j,pMat);
			}

			return true;

		}
	

		std::vector<std::shared_ptr<cmo::element::SubMesh>>		getSubMeshArray(BinaryReader * meshData) {
			std::vector<std::shared_ptr<cmo::element::SubMesh>> submeshArray;

			auto numSubmesh = meshData->read<UINT>();
			if (numSubmesh == nullptr)
				return (std::vector<std::shared_ptr<cmo::element::SubMesh>>());

			if (*numSubmesh == 0) {
				OutputDebugString(TEXT("\nWARNING: No sub - meshes found\n"));
				assert(false);
			} else {
				auto subMesh = meshData->read<element::SubMesh>(*numSubmesh);

				if (subMesh == nullptr)
					return (std::vector<std::shared_ptr<cmo::element::SubMesh>>());


				submeshArray.reserve(*numSubmesh);

				for (UINT j = 0; j < *numSubmesh; ++j) {
					submeshArray.emplace_back(std::make_shared<cmo::element::SubMesh>(subMesh[j]));
				}
			}


			return submeshArray;
		}


		std::vector<std::shared_ptr<ID3D11Buffer>>	getIndexBufferArray(ID3D11Device* device, BinaryReader * meshData) {
			std::vector<std::shared_ptr<ID3D11Buffer>> indexBufferArray;
			

			auto numIBs = meshData->read<UINT>();
			
			if (numIBs == nullptr)
				return(std::vector<std::shared_ptr<ID3D11Buffer>>());

			if (*numIBs == 0) {
				OutputDebugString(TEXT("\nWARNING: No IBs found\n"));
				assert(false);
			} else {
				indexBufferArray.reserve(*numIBs);

				for (UINT j = 0; j < *numIBs; ++j) {
					auto numIndexes = meshData->read<UINT>();

					if (numIndexes == nullptr)
						return(std::vector<std::shared_ptr<ID3D11Buffer>>());


					if (*numIndexes == 0) {
						OutputDebugString(TEXT("WARNING: Empty IB found \n"));
						assert(false);
					} else {

						//	create index buffer
						size_t ibBytes = sizeof(cmo::IndexType) * (*numIndexes);


						auto indexes = (meshData->read<cmo::IndexType>(*numIndexes));
						if (indexes == nullptr)
							return(std::vector<std::shared_ptr<ID3D11Buffer>>());

						indexBufferArray.emplace_back(
							d3::CreateIndexBuffer(device, const_cast<cmo::IndexType*>(indexes), ibBytes)
							);
					}
				}
			}

			return(indexBufferArray);
		}


		std::vector<std::shared_ptr<ID3D11Buffer>>	getVertexBufferArray(ID3D11Device* device, BinaryReader * meshData) {
			std::vector<std::shared_ptr<ID3D11Buffer>> vertexBufferArray;

			auto numVBs = meshData->read<UINT>();
			 if (numVBs == nullptr)
				 return(std::vector<std::shared_ptr<ID3D11Buffer>>());

			if (*numVBs == 0) {
				OutputDebugStringW(L"WARNING: No VBs found");
				assert(false);
			} else {
				vertexBufferArray.reserve(*numVBs);

				for (UINT j = 0; j < *numVBs; ++j) {
					auto numVerts = meshData->read<UINT>();
					if (numVerts == nullptr)
						return(std::vector<std::shared_ptr<ID3D11Buffer>>());

					if (*numVerts == 0) {
						OutputDebugStringW(L"WARNING: Empty VB found");
						assert(false);
					} else {
						size_t vbBytes = sizeof(element::Vertex) * (*numVerts);

						auto verts = meshData->read<cmo::element::Vertex>(*numVerts);
						if (verts == nullptr)
							return(std::vector<std::shared_ptr<ID3D11Buffer>>());

						vertexBufferArray.emplace_back(
							d3::CreateVertexBuffer(device, const_cast<element::Vertex*>(verts), vbBytes)
							);
					}
				}
			}

			return vertexBufferArray;
		}
	

		bool getSkinningVertexBufferArray(
			std::vector<std::shared_ptr<ID3D11Buffer>>* skinningVertexBuffer, ID3D11Device* device, BinaryReader * meshData) {
			
			//	頂点情報一時保存
			std::vector<cmo::element::SkinningVertex>	skinningVertex;


			auto numSkinVBs = meshData->read<UINT>();
			if (numSkinVBs == nullptr)
				return false;

			if (*numSkinVBs != 0) {
				skinningVertexBuffer->reserve(*numSkinVBs);

				for (UINT i = 0; i < *numSkinVBs; ++i) {
					skinningVertex.clear();

					auto numVerts = meshData->read<UINT>();
					if (numVerts == nullptr)
						return false;

					if (*numVerts == 0) {
						assert(("WARNING: Empty SkinVB found",false));
					} else {
						skinningVertex.resize(*numVerts);


						auto verts = meshData->read<SkinningVertexInput>(*numVerts);
						if (verts == nullptr)
							return false;
						
						// Read in the vertex data.
						for (auto& vertex : skinningVertex)
						{
							for (size_t j = 0; j < cmo::Num_Bone_Influences; ++j)
							{
								// Convert indices type.
								vertex.boneIndex[j] = static_cast<cmo::element::SkinningVertex::IndexType>(verts->boneIndex[j]);
								vertex.boneWeight[j] = verts->boneWeight[j];
							}

							++verts;
						}

						size_t vbBytes = sizeof(cmo::element::SkinningVertex) * (*numVerts);
						skinningVertexBuffer->emplace_back(
						d3::CreateVertexBuffer(device, skinningVertex.data(), vbBytes)
						);

					}

				}
			}

			return true;
		}


		bool skipSkinningVertexData(BinaryReader * meshData) {
			auto numSkinVBs = meshData->read<UINT>();
			if (numSkinVBs == nullptr)
				return false;

			if (*numSkinVBs != 0) {
				for (UINT j = 0; j < *numSkinVBs; ++j) {
					auto numVerts = meshData->read<UINT>();
					if (numVerts == nullptr)
						return false;
					if (*numVerts != 0)
						meshData->read<SkinningVertexInput>(*numVerts);

				}
			}

			return true;
			
		}


		bool getBoneAndAnimation(
			std::shared_ptr<std::vector<element::Bone>> *ppBoneArray,
			std::shared_ptr<std::unordered_map<std::wstring, unsigned int>> *ppBoneIndexTable,
			std::shared_ptr<std::unordered_map<std::wstring, AnimClip>> *ppAnimationTable,
			BinaryReader * meshData) {
			
			auto numBones = meshData->read<UINT>();
			if (numBones == nullptr)
				return false;

			if (*numBones == 0) {
				assert(("WARNING: Animation bone data is missing", false));
			} else {
				(*ppBoneArray) = std::make_shared<std::vector<element::Bone>>();
				(*ppBoneArray)->reserve(*numBones);
				(*ppBoneIndexTable) = std::make_shared<std::unordered_map<std::wstring, unsigned int>>();
				(*ppBoneIndexTable)->reserve(*numBones);

				for (UINT j = 0; j < *numBones; ++j) {
					auto boneName = loader::getString(meshData);
					if (boneName.empty())
						return false;

					// Bone settings
					auto bone = meshData->read<element::Bone>();
					if (bone == nullptr)
						return false;

					(*ppBoneArray)->emplace_back(*bone);
					(*ppBoneIndexTable)->emplace(boneName, j);

					if (bone->ParentIndex >= static_cast<decltype(bone->ParentIndex)>(*numBones)) {
						assert(("WARNING: Bone references bone not present",false));
					}
					
				}
			}


			// Animation Clips
			auto numClips = meshData->read<UINT>();
			if (numClips == nullptr)
				return false;

			if (*numClips == 0) {
				assert(false);
				//wprintf(L"WARNING: No animation clips found\n");
			} else {
				(*ppAnimationTable) = std::make_shared<std::unordered_map<std::wstring, AnimClip>>();
				(*ppAnimationTable)->reserve(*numClips);

				for (UINT i = 0; i < *numClips; ++i) {
					// Clip name

					auto clipName = loader::getString(meshData);
					if (clipName.empty())
						return false;

					auto& animClip = (*ppAnimationTable)->emplace(clipName, cmo::AnimClip()).first->second;
					
					animClip.StartTime	= *(meshData->read<float>());
					animClip.EndTime	= *(meshData->read<float>());
					UINT numAnimKeys	= *(meshData->read<UINT>());
					animClip.BoneKeyframe.resize(*numBones);

					if (numAnimKeys == 0) {
						assert(false);
						//wprintf(L"WARNING: Animation clip %s for mesh %s missing numAnimKeys", clipName, meshName);
					} else {
						auto keys = meshData->read<KeyframeInput>(numAnimKeys);
						if (keys == nullptr)
							return false;
						
						auto& boneKeyframes = animClip.BoneKeyframe;
						for (UINT k = 0; k < numAnimKeys; ++k) {
							UINT boneIndex = keys[k].BoneIndex;
							element::Keyframe kf = parseKeyframe(keys[k].Transform);

							boneKeyframes[boneIndex].emplace( keys[k].Time, kf);
						}
							
					}

				}
			}

			return true;
		}


		bool skipBoneAndAnimation(BinaryReader * meshData) {
			auto numBones = meshData->read<UINT>();
			if (numBones == nullptr)
				return false;

			if (*numBones != 0){
				for (UINT j = 0; j < *numBones; ++j) {
					auto boneName = loader::getString(meshData);
					if (boneName.empty())
						return false;

					// Bone settings
					auto bone = meshData->read<element::Bone>();
					if (bone == nullptr)
						return false;
					
				}

			}

			// Animation Clips
			auto numClips = meshData->read<UINT>();
			if (numClips == nullptr)
				return false;

			if (*numClips != 0){

				for (UINT j = 0; j < *numClips; ++j) {
					// Clip name

					auto clipName = loader::getString(meshData);
					if (clipName.empty())
						return false;

					float StartTime		= *(meshData->read<float>());
					float EndTime		= *(meshData->read<float>());
					UINT numAnimKeys	= *(meshData->read<UINT>());

					if (numAnimKeys != 0){
						auto keys = meshData->read<KeyframeInput>(numAnimKeys);
						if (keys == nullptr)
							return false;
					}
				}
			}

			return true;
		}
		

	}	//	end of namespace cmo::loader

}	//	end of namespace cmo	
