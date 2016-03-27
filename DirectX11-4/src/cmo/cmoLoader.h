#pragma once

#include "cmoElement.h"
#include <d3d11.h>



//	参考ソースコード：↓URL内一番下のcmodump.cpp
//	https://directxmesh.codeplex.com/wikipage?title=CMO



//	cmoへの変換参考
//	http://masafumi.cocolog-nifty.com/masafumis_diary/2013/02/windows-storefb.html
//	『アセットへ追加』とあるが、適当にプロジェクト内にフィルタ作ってそこに入れてやればいい。
//	追加したら、以下に従ってfbxファイルもビルドに含めてやればいい
//	http://msdn.microsoft.com/ja-jp/library/windows/apps/hh972446.aspx


// .CMO files
// UINT - Mesh count
// { [Mesh count]
//      UINT - Length of name
//      wchar_t[] - Name of mesh (if length > 0)
//      UINT - Material count
//      { [Material count]
//          UINT - Length of material name
//          wchar_t[] - Name of material (if length > 0)
//          Material structure
//          UINT - Length of pixel shader name
//          wchar_t[] - Name of pixel shader (if length > 0)
//          { [8]
//              UINT - Length of texture name
//              wchar_t[] - Name of texture (if length > 0)
//          }
//      }
//      BYTE - 1 if there is skeletal animation data present
//      UINT - SubMesh count
//      { [SubMesh count]
//          SubMesh structure
//      }
//      UINT - IB Count
//      { [IB Count]
//          UINT - Number of USHORTs in IB
//          USHORT[] - Array of indices
//      }
//      UINT - VB Count
//      { [VB Count]
//          UINT - Number of verts in VB
//          Vertex[] - Array of vertices
//      }
//      UINT - Skinning VB Count
//      { [Skinning VB Count]
//          UINT - Number of verts in Skinning VB
//          SkinningVertex[] - Array of skinning verts
//      }
//      MeshExtents structure
//      [If skeleton animation data is not present, file ends here]
//      UINT - Bone count
//      { [Bone count]
//          UINT - Length of bone name
//          wchar_t[] - Bone name (if length > 0)
//          Bone structure
//      }
//      UINT - Animation clip count
//      { [Animation clip count]
//          UINT - Length of clip name
//          wchar_t[] - Clip name (if length > 0)
//          float - Start time
//          float - End time
//          UINT - Keyframe count
//          { [Keyframe count]
//              Keyframe structure
//          }
//      }
// }

/*
*	モデル内の画像をddsテクスチャへ変換時、.png.ddsみたいになっちゃう＆ディレクトリがえらいことになる（
*	→前者はソースコード内で置き換えるorDDS使わない、後者は関数引数で相対パス渡すようにするか…うげげ
*/




namespace cmo {

	namespace loader {

		class BinaryReader{
		private :
			std::unique_ptr<uint8_t[]> _meshData;
			size_t	_dataSize;
			size_t	_usedSize;

		public :
			BinaryReader(): 
				_meshData(nullptr),
				_dataSize(0),
				_usedSize(0){}

			//!	ファイルから情報取り込み
			HRESULT importFile(const wchar_t* fileName);

			//!	次のデータを読み込み可能かどうか
			inline bool isValid()const{
				if (_dataSize < _usedSize) {
					//wprintf(L"ERROR: Unexpected \"End of file\" \n");
					assert(false);
					return false;
				}
				else {
					return true;
				}
			}

			//!	指定された型のデータをを読み込み
			//!	@param count : 読み込む個数
			template <typename T>
			inline const T* read(UINT count = 1){
				
				assert(this->isValid());

				auto data = reinterpret_cast<const T*>(_meshData.get() + _usedSize);
				_usedSize += sizeof(T)* count;

#if (DEBUG || _DEBUG)
				if (!this->isValid())
					return nullptr;
#endif
				return data;
			}
		};



		//!	文字列取得、usedSizeを読み込んだ文だけ進める
		inline std::wstring	getString(BinaryReader* meshData) {

			auto length = meshData->read<UINT>();
			if (!meshData->isValid())
				return std::wstring();

			std::wstring name = meshData->read<wchar_t>(*length);
			if (!meshData->isValid())
				return std::wstring();

			return	name;
		}


		//!	マテリアル読み込み
		//	textureDirectoryはテクスチャを探すディレクトリ
		bool getMaterialArray(std::vector<std::shared_ptr<cmo::element::Material>> *materialArray, std::unordered_map<std::wstring, unsigned int> *materialIndexTable,
			ID3D11Device* device, const wchar_t* textureDirectory, BinaryReader * meshData);


		//!	サブメッシュ読み込み
		std::vector<std::shared_ptr<cmo::element::SubMesh>>		getSubMeshArray(BinaryReader * meshData);


		//!	インデックスバッファの読み込み及び作成
		std::vector<std::shared_ptr<ID3D11Buffer>>	getIndexBufferArray(ID3D11Device* device, BinaryReader * meshData);

		
		//!	頂点バッファの読み込み及び作成
		std::vector<std::shared_ptr<ID3D11Buffer>>	getVertexBufferArray(ID3D11Device* device, BinaryReader * meshData);


		//!	メッシュ拡張情報読み込み
		inline std::shared_ptr<cmo::element::MeshExtents>	getMeshInfo(BinaryReader* meshData) {
			
			auto extents = meshData->read<cmo::element::MeshExtents>();

			if (!meshData->isValid())
				return nullptr;

			//	copy-constructor呼んでるよ
			return std::make_shared<cmo::element::MeshExtents>(*extents);
		}


		//!	スキンメッシュ用頂点バッファの読み込みおよび作成
		bool getSkinningVertexBufferArray(std::vector<std::shared_ptr<ID3D11Buffer>>* skinningVertexBuffer, ID3D11Device* device, BinaryReader * meshData);


		//!	スキンメッシュ用頂点読み飛ばし
		bool skipSkinningVertexData(BinaryReader * meshData);


		//!	ボーン情報及びアニメーション取得
		bool getBoneAndAnimation(
			std::shared_ptr<std::vector<element::Bone>> *ppBoneArray,
			std::shared_ptr<std::unordered_map<std::wstring, unsigned int>> *ppBoneIndexTable,
			std::shared_ptr<std::unordered_map<std::wstring, AnimClip>> *ppAnimationTable,
			BinaryReader *meshData);

		//!	ボーン情報及びアニメーション読み飛ばし
		bool skipBoneAndAnimation(BinaryReader * meshData);

	};	//	end of namespace cmo::loader

}; // end of namespace cmo
