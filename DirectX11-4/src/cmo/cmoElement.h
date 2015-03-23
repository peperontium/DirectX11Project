#pragma once



#include <memory>
#include <vector>
#include <unordered_map>
#include <map>
#include <string>

#include "../DXGIFormatDetection.h"

#include <DirectXMath.h>
#include <d3d11.h>



namespace cmo {
	
	//	モデルのボーン最大数、シェーダー側と一致させてネ
	const UINT Max_Bone = 32;


	//	cmo側での規定値、変更するな
	const UINT Max_Texture = 8;
	const UINT Num_Bone_Influences = 4;
	using IndexType = uint16_t;
	//	また、ボーンやテクスチャ名辺りはすべてwcharで返ってくるので注意


	namespace element {


		struct Material {
			struct TextureData {
				std::shared_ptr<ID3D11ShaderResourceView>	pTexture;
				std::wstring	textureName;
			};

			//!	名前はメッシュ側に	名前→配列添字のmap作るので無しで

			DirectX::XMFLOAT4   Ambient;
			DirectX::XMFLOAT4   Diffuse;
			DirectX::XMFLOAT4   Specular;		//色,RGBAを0.0~1.0で4つ
			float               SpecularPower;
			DirectX::XMFLOAT4   Emissive;
			DirectX::XMFLOAT4X4	UVTransform;

			TextureData	TextureArray[Max_Texture];
		};

		//!	いわゆるBounding Box及びBounding Circle	の情報
		struct MeshExtents {
			MeshExtents() :
				CenterX(0), CenterY(0), CenterZ(0),
				Radius(-1),
				MinX(0), MinY(0), MinZ(0),
				MaxX(0), MaxY(0), MaxZ(0)
			{}

			float CenterX, CenterY, CenterZ;
			float Radius;

			float MinX, MinY, MinZ;
			float MaxX, MaxY, MaxZ;
		};

		struct SubMesh {
			UINT MaterialIndex;
			UINT IndexBufferIndex;
			UINT VertexBufferIndex;
			UINT StartIndex;
			UINT PrimCount;
		};


		//!	剛体モデルの頂点
		struct Vertex {
			DirectX::XMFLOAT3 Position;
			DirectX::XMFLOAT3 Normal;
			DirectX::XMFLOAT4 Tangent;
			UINT color;
			DirectX::XMFLOAT2 TextureCoordinates;

		};

		//!	スキンメッシュするモデルの頂点の追加情報
		//!	読み込みの時とは違うボーンインデックスの型を用いる。バッファサイズ節約のため。
		struct SkinningVertex {
			using IndexType = uint8_t;

			IndexType boneIndex[Num_Bone_Influences];
			float boneWeight[Num_Bone_Influences];
			
		};


		//!	ボーン情報、ボーン名→配列添字は別でmap<string,int>を作って保存。
		struct Bone {
			INT ParentIndex;
			//!	逆姿勢行列
			DirectX::XMFLOAT4X4 InvBindPos;
			DirectX::XMFLOAT4X4 BindPos;
			DirectX::XMFLOAT4X4 LocalTransform;
		};

		//!	アニメーションキー
		struct Keyframe {
			Keyframe() {};
			//!	拡大縮小スケーリング
			DirectX::XMFLOAT3	Scaling;
			//!	平行移動
			DirectX::XMFLOAT3	Translation;
			//!	回転クォータニオン
			DirectX::XMFLOAT4	RotationQt;
		};

	};	//	end of namespace cmo::element

	//!	アニメーション情報。StarterKit参照
	struct AnimClip {
		AnimClip() :StartTime(0), EndTime(0){}
		float StartTime;
		float EndTime;
		//!	[ボーンIndex][フレーム時間]	でのキーフレーム配列
		std::vector < std::map<float, element::Keyframe>>	BoneKeyframe;
	};



}; // end of namespace cmo

//static_assert(sizeof(cmo::element::Material) == 132, "CMO Mesh structure size incorrect");
static_assert(sizeof(cmo::element::SubMesh) == 20, "CMO Mesh structure size incorrect");
static_assert(sizeof(cmo::element::Vertex) == 52, "CMO Mesh structure size incorrect");
//static_assert(sizeof(cmo::element::SkinningVertex) == 32, "CMO Mesh structure size incorrect");
static_assert(sizeof(cmo::element::MeshExtents) == 40, "CMO Mesh structure size incorrect");
static_assert(sizeof(cmo::element::Bone) == 196, "CMO Mesh structure size incorrect");
//static_assert(sizeof(cmo::element::Keyframe) == 72, "CMO Mesh structure size incorrect");
