/*
 *	定数バッファ
 */
cbuffer Projection : register(b0) {
	matrix	mtxProj		: packoffset(c0);
};

cbuffer Camera : register(b1) {
	matrix	mtxView		: packoffset(c0);
};

cbuffer World : register(b2) {
	matrix	mtxWorld	: packoffset(c0);
};




/*
 *	サンプラー類
 */

Texture2D Texture0 : register(t0);
Texture2D Texture1 : register(t1);
Texture2D Texture2 : register(t2);
Texture2D Texture3 : register(t3);
Texture2D Texture4 : register(t4);
Texture2D Texture5 : register(t5);
Texture2D Texture6 : register(t6);
Texture2D Texture7 : register(t7);



SamplerState SamLinear : register(s0);


//! 頂点属性
/*!
システム側に渡すセマンティクスはSV_の接頭辞がついている
*/
struct InputVS {
	float4 pos		: POSITION;
	float3 normal	: NORMAL;
	float4 tangent	: TANGENT;
	float4 color	: COLOR;
	float2 tex		: TEXCOORD;	//	x,yの順
};

struct OutputVS {
	float4	pos			: SV_POSITION;
	float2	tex			: TEXTURE;			// テクスチャUV
};

//! 頂点シェーダ
OutputVS RenderVS(InputVS inVert) {
	OutputVS	outVert;

	matrix	mtxVP = mul(mtxView, mtxProj);
	matrix	mtxWVP = mul(mtxWorld, mtxVP);
	outVert.pos = mul(inVert.pos, mtxWVP);

	outVert.tex = inVert.tex;

	//	Blenderからの出力として決め打ち、y反転
	//	ちゃんとやる場合はMaterialからUVTransformMtxを引っ張ってきて掛ける必要あり
	outVert.tex.y = 1-outVert.tex.y;

	return outVert;
}


//! ピクセルシェーダ
float4 RenderPS(OutputVS inPixel) : SV_TARGET0{
	
	float4 color = Texture0.Sample(SamLinear, inPixel.tex);
//	color.rgb *= color.a;
	// テクスチャ取得
	return color;         // テクセル読み込み
	
	//return inPixel.color;
}
