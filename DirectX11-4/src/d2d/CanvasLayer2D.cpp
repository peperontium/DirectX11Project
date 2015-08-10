#include "CanvasLayer2D.h"


#include "../dx11/DirectXShaderLevelDefine.h"

#include "./ShaderBytes.h"
#include "../dx11/DX11ThinWrapper.h"


namespace {
	struct SpriteConstBuffer {
		//	シェーダーのレジスタは一つ当たりfloat4つ当てるので2x4行列を用いる。_*4成分は使わない
		float transform[2][4];
		float texTransform[2][4];
	};
}

namespace d2 {

	void CanvasLayer2D::init(ID3D11Device* device, int canvasWidth, int canvasHeight) {

		using namespace DX11ThinWrapper;

		_width = canvasWidth;
		_height = canvasHeight;

		static D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};


		_vertexShader = d3::CreateVertexShader(device, cso::compiledShader::simpleVS, sizeof(cso::compiledShader::simpleVS));
		_inputLayout = d3::CreateInputLayout(device, layout, _countof(layout), cso::compiledShader::simpleVS, sizeof(cso::compiledShader::simpleVS));


		_pixelShader = d3::CreatePixelShader(
			device, cso::compiledShader::simplePS, sizeof(cso::compiledShader::simplePS)
			);

		SpriteVertex verts[] = {
			SpriteVertex(0.0f, -2.0f, 1.0f, 0.0f, 1.0f),
			SpriteVertex(0.0f, 0.0f, 1.0f, 0.0f, 0.0f),
			SpriteVertex(2.0f, -2.0f, 1.0f, 1.0f, 1.0f),
			SpriteVertex(2.0f, 0.0f, 1.0f, 1.0f, 0.0f),
		};

		_vertexBuffer = d3::CreateVertexBuffer(device, verts, sizeof(SpriteVertex)* 4);


		//	深度テストを無効にするための深度ステンシルステートの作成
		D3D11_DEPTH_STENCIL_DESC dsd = {};
		dsd.DepthEnable = FALSE;
		dsd.StencilEnable = FALSE;
		_depthStencilState = d3::CreateDepthStencilState(device, dsd);


		//	ブレンド設定
		{
			D3D11_BLEND_DESC blendDesc[] = { {
				FALSE, FALSE, {
					{
						TRUE,
						D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD,
						D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
						D3D11_COLOR_WRITE_ENABLE_ALL
					}
				}
			} };
			_blendState[BlendMode::Default] = d3::CreateBlendState(device, blendDesc);
		}
		{
			D3D11_BLEND_DESC blendDesc[] = { {
				FALSE, FALSE, {
					{
						TRUE,
						D3D11_BLEND_ONE, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD,
						D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
						D3D11_COLOR_WRITE_ENABLE_ALL
					}
				}
			} };
			_blendState[BlendMode::PreMultiPlyedAlpha] = d3::CreateBlendState(device, blendDesc);
		}
		{
			D3D11_BLEND_DESC blendDesc[] = { {
				FALSE, FALSE, {
					{
						TRUE,
						D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD,
						D3D11_BLEND_ZERO, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD,
						D3D11_COLOR_WRITE_ENABLE_ALL
					}
				}
			} };
			_blendState[BlendMode::Add] = d3::CreateBlendState(device, blendDesc);
		}



		//	定数バッファつくる
		_constantBuffer = d3::CreateConstantBuffer(device, nullptr, sizeof(SpriteConstBuffer), D3D11_CPU_ACCESS_WRITE);
	}


	void CanvasLayer2D::updateConstantBuffer(ID3D11DeviceContext* context, const D2D_MATRIX_3X2_F& transform,
		const D2D_MATRIX_3X2_F& textureTransform) {
		
		DX11ThinWrapper::d3::mapping(_constantBuffer.get(), context, [&](D3D11_MAPPED_SUBRESOURCE resource){
			auto param = static_cast<SpriteConstBuffer *>(resource.pData);

			//	転置しつつデータ転送
			param->transform[0][0] = transform._11;
			param->transform[1][0] = transform._12;
			param->transform[0][1] = transform._21;
			param->transform[1][1] = transform._22;
			param->transform[0][2] = transform._31;
			param->transform[1][2] = transform._32;

			param->texTransform[0][0] = textureTransform._11;
			param->texTransform[1][0] = textureTransform._12;
			param->texTransform[0][1] = textureTransform._21;
			param->texTransform[1][1] = textureTransform._22;
			param->texTransform[0][2] = textureTransform._31;
			param->texTransform[1][2] = textureTransform._32;
		});
	}


	void CanvasLayer2D::renderTexture(ID3D11DeviceContext* context, ID3D11ShaderResourceView *texture, BlendMode blendMode) {

			FLOAT blendFactor[] = { 1, 1, 1, 1 };
			context->OMSetBlendState(_blendState[blendMode].get(), blendFactor, 0xffffffff);

			ID3D11Buffer* buffers[] = {_constantBuffer.get()};
			context->VSSetConstantBuffers(3, 1, buffers);

			context->PSSetShaderResources(0, 1, &texture);

			context->Draw(4, 0);
	}
}
