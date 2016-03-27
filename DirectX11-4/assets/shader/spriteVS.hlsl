/*
*	定数バッファ
*/

//	デフォルトは列優先(column_major)
cbuffer Transform : register(b3) {
	column_major float3x2	mtxTransform	: packoffset(c0);
	column_major float3x2	mtxTextureTransform : packoffset(c2);
	//	色
};


//! 頂点属性
/*!
システム側に渡すセマンティクスはSV_の接頭辞がついている
*/
struct InputVS {
	float4 pos		: POSITION;
	float2 tex		: TEXCOORD;	//	x,yの順
};

struct OutputVS {
	float4	pos			: SV_POSITION;
	float2	tex			: TEXTURE;			// テクスチャUV
};

//! 頂点シェーダ
OutputVS RenderVS(InputVS inVert){
	OutputVS	outVert;

	outVert.pos = float4(mul(inVert.pos.xyz,mtxTransform).xy, 0, 1);

	outVert.tex = mul(float4(inVert.tex.xy, 1,0), mtxTextureTransform).xy;

	return outVert;
}

