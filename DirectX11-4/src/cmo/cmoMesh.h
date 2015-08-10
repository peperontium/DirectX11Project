#pragma once

#include "cmoElement.h"
#include "AxisConvert.h"
#include <algorithm>


//	スキンメッシュ参考
//	https://code.msdn.microsoft.com/windowsapps/Visual-Studio-3D-Starter-455a15f1/view/Discussions

namespace cmo {

	//!	普通のメッシュモデル
	class Mesh {
	protected:

		//!	描画用バッファ類
		std::vector<std::shared_ptr<element::SubMesh>>	_submeshArray;
		std::vector<std::shared_ptr<element::Material>>	_materialArray;
		std::vector<std::shared_ptr<ID3D11Buffer>>		_vertexBufferArray;
		std::vector<std::shared_ptr<ID3D11Buffer>>		_indexBufferArray;


		//!	マテリアルへのアクセス用
		std::unordered_map<std::wstring, unsigned int>	_materialIndexTable;

		//!	シェーダー周り
		std::shared_ptr<ID3D11VertexShader>	_vertexShader;
		std::shared_ptr<ID3D11PixelShader>	_pixelShader;
		std::shared_ptr<ID3D11InputLayout>	_inputLayout;

		//!	メッシュのBounding Box及びBounding Circle
		std::shared_ptr<element::MeshExtents>			_extent;
		//!	メッシュ名・・・要る？
		std::wstring				_name;

		//!	現在の行列保存用、コピー時に使用する
		DirectX::XMFLOAT4X4					_mtxWorld;


		//!	ワールド座標変換行列をシェーダーに送るための定数バッファ
		std::shared_ptr<ID3D11Buffer>		_mtxConstBuffer;


		//!	定数バッファはメッシュ間で共用するためそれの保存用。あとでもう少し考えて組みなおす
		static std::weak_ptr<ID3D11Buffer>		s_mtxConstBufferShared;


		//!	バッファ類初期化
		virtual void _ClearBuffers() {
			_submeshArray.clear();
			_materialArray.clear();
			_vertexBufferArray.clear();
			_indexBufferArray.clear();

			_materialIndexTable.clear();

			_extent.reset();
			_name.clear();

		};

		//!	行列用バッファに初期値（単位行列）設定
		virtual void _InitMatrixBuffer(ID3D11Device *device);

	public:
		//!	コンストラクタ
		Mesh() :
			_mtxWorld(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1){}

		//!	デストラクタ
		~Mesh() {}

		//!	メッシュを一つだけロード、ロード済みの場合は上書き
		/**
		 * @param device			デバイスへのポインタ
		 * @param filePath			読み込む .cmoモデルファイルのパス
		 * @param textureDirecctory	テクスチャを探すディレクトリ名、指定しない場合はモデルと同じ階層から探す
		 */
		virtual HRESULT load(ID3D11Device* device, const wchar_t* filePath, const wchar_t* textureDirectory = nullptr);
		

		//!	マテリアル取得
		const std::shared_ptr<cmo::element::Material>	getMaterial(const wchar_t* matName) const {
			if (_materialIndexTable.count(matName) != 0) {
				return(_materialArray[_materialIndexTable(matName)]);
			}
			

			return(nullptr);
		}

		
		//!	描画時に使う頂点シェーダーの設定。
		//!	入力レイアウトも同時に更新する
		virtual void applyVS(std::shared_ptr<ID3D11VertexShader>& vertexShader, std::weak_ptr<ID3DBlob> blob, ID3D11Device *device);

		
		//!	描画時に使うピクセルシェーダーの設定
		void applyPS(std::shared_ptr<ID3D11PixelShader>& pixelShader) {
			_pixelShader = pixelShader;
		}


		//!	マテリアルを全て置き換える
		/**
		 * @param dstMatName	置き換え対象のマテリアル名
		 * @param srcMaterial	新たに設定するマテリアル
		 * @param srcMatName	新たに設定するマテリアルの名前
		 */
		void replaceMaterial(const wchar_t* dstMatName, std::shared_ptr<cmo::element::Material> &srcMaterial, const wchar_t* srcMatName);
		

		//!	テクスチャを全て置き換える
		/**
		 * @param dstTextureName	置き換え対象のテクスチャ名
		 * @param srcTexture		新たに設定するテクスチャ
		 * @param srcTextureName	新たに設定するテクスチャの名前
		 */
		void replaceTexture(const wchar_t* dstTextureName, std::shared_ptr<ID3D11ShaderResourceView> &srcTexture, const wchar_t* srcTextureName);
		

		//! ワールド変換行列の設定
		virtual void updateMatrix(const DirectX::XMMATRIX& mtxWorld) {
			//	現在の状態保存（コピーする時用）
			XMStoreFloat4x4(&_mtxWorld,mtxWorld);
		}


		//! 描画
		/**
		 *	@param context		コンテキストへのポインタ
		 *	@param startSlot	ワールド変換行列を設定する、シェーダー側のバッファスロットNo
		 */
		virtual void render(ID3D11DeviceContext* context, UINT startSlot)const;



	public:
		/**
		 *	モデルコピー用のコピーコンストラクタ、代入演算子
		 */
		Mesh(const Mesh& mesh) {
			//	代入演算子の使いまわし
			*this = mesh;
		}
		Mesh& operator=(const Mesh& mesh);

		//!	全てのメッシュを配列として読み込む static関数。
		/**
		* @param device				デバイスへのポインタ
		* @param filePath			読み込む .cmoモデルファイルのパス
		* @param textureDirecctory	テクスチャを探すディレクトリ名、指定しない場合はモデルと同じ階層から探す
		*/
		static std::vector<cmo::Mesh> LoadMeshArray(ID3D11Device* device, const wchar_t* filePath, const wchar_t* textureDirectory = nullptr);

	};	// end of class Mesh { ...



	//!	スキンメッシュモデル
	class SkinnedMesh : public Mesh {
	protected:
		//!	アニメーション現在フレーム(0.0~60.0)
		float _animFrame;

		//!	スキンメッシュ用
		std::vector<std::shared_ptr<ID3D11Buffer>>	_skinningVertexBufferArray;
		//!	ボーンのワールド変換行列
		std::vector<DirectX::XMFLOAT4X4>	_boneMtxArray;
		//!	変換行列が変更済みかどうか
		std::vector<bool>					_isTransformCombined;
		//!	ボーン情報。共用可能リソースのためポインタで管理
		std::shared_ptr<std::vector<element::Bone>>			_pBoneArray;
		//!	ボーン名→配列添字テーブル。同じく共用できるものなのでポインタ
		std::shared_ptr<std::unordered_map<std::wstring, unsigned int>>	_pBoneIndexTable;
		//!	アニメーション名->キーフレーム情報。これも同じく共用できるものなのでポインタ
		std::shared_ptr<std::unordered_map<std::wstring, AnimClip>>		_pAnimationTable;
		//!	現在アニメーション
		std::unordered_map<std::wstring, AnimClip>::iterator	_currentAnim;
		

		//!	ボーン行列用定数バッファ。スキニングメッシュ間で共用。
		static std::weak_ptr<ID3D11Buffer>		s_boneMtxConstBufferShared;



		//!	行列用バッファに初期値（単位行列）設定
		void _InitMatrixBuffer(ID3D11Device *device) override;

		//!	ボーン行列をすべて更新
		void _UpdateBoneTransform();

		//! 再帰的にボーン変換を適用
		void _CombineBoneTransforms(INT currentBoneIndex);

		//!	バッファクリア
		void _ClearBuffers()override {
			Mesh::_ClearBuffers();

			_skinningVertexBufferArray.clear();
			_boneMtxArray.clear();
			_isTransformCombined.clear();
			_pBoneArray.reset();
			_pBoneIndexTable.reset();
			_pAnimationTable.reset();
			_currentAnim = decltype(_currentAnim)();
		}


	public:
		//!	コンストラクタ
		SkinnedMesh() :_animFrame(0.0f){
		}
		//!	デストラクタ
		~SkinnedMesh() {}
		
		//!	ボーン名からワールド変換行列取得
		//!	行優先行列を返す
		void getBoneMatrix( DirectX::XMFLOAT4X4* pMatrix, const wchar_t* boneName)const {
			using namespace DirectX;
			if (_pBoneIndexTable->count(boneName) != 0) {
				XMStoreFloat4x4(pMatrix,XMMatrixTranspose(XMLoadFloat4x4(&_boneMtxArray[_pBoneIndexTable->at(boneName)])));
			}
		}


		//!	使用するアニメーションを設定
		void setAnimation(const wchar_t* animName) {
			if (_pAnimationTable->count(animName) != 0) {
				_currentAnim = _pAnimationTable->find(animName);
			}
		}

		
		//!	アニメーション時間更新
		/**
		* @param frame	アニメーションのフレーム設定、0~60の範囲で設定
		*/
		void setAnimationFrame(float frame) {
			_animFrame = std::min(std::max(frame, 0.0f), 60.0f);
		}
		
		
	public:
		/*******************
		 * override methods
		 *******************/

		//!	メッシュを一つだけロード、ロード済みの場合は上書き。
		/**
		* @param device				デバイスへのポインタ
		* @param filePath			読み込む .cmoモデルファイルのパス
		* @param textureDirecctory	テクスチャを探すディレクトリ名、指定しない場合はモデルと同じ階層から探す
		*/
		HRESULT load(ID3D11Device* device, const wchar_t* filePath, const wchar_t* textureDirectory = nullptr) override;
		

		//!	描画時に使う頂点シェーダーの設定。
		//!	入力レイアウトも同時に更新する
		virtual void applyVS(std::shared_ptr<ID3D11VertexShader>& vertexShader, std::weak_ptr<ID3DBlob> blob, ID3D11Device *device) override;


		//! ワールド変換行列の設定
		virtual void updateMatrix(const DirectX::XMMATRIX& mtxWorld)override{
			//	ルートの状態保存
			Mesh::updateMatrix(mtxWorld);
			//	ボーン変換
			_UpdateBoneTransform();
		}


		//! 描画
		/**
		*	@param context		コンテキストへのポインタ
		*	@param startSlot	ワールド変換行列を設定する、シェーダー側のバッファスロットNo
		*/
		virtual void render(ID3D11DeviceContext* context, UINT startSlot)const override;



	public:
		/*******************
		* コピーコンストラクタ、代入演算子
		*******************/
		SkinnedMesh(const SkinnedMesh& mesh) {
			//	代入演算子使いまわし
			*this = mesh;
		}
		SkinnedMesh& operator=(const SkinnedMesh& mesh);


		//!	全てのメッシュを配列として読み込む
		/**
		* @param device				デバイスへのポインタ
		* @param filePath			読み込む .cmoモデルファイルのパス
		* @param textureDirecctory	テクスチャを探すディレクトリ名、指定しない場合はモデルと同じ階層から探す
		*/
		static std::vector<cmo::SkinnedMesh> LoadMeshArray(ID3D11Device* device, const wchar_t* filePath, const wchar_t* textureDirectory = nullptr);


	};	// end of class SkinnedMesh { ...


};// end of namespace cmo