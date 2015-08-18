#pragma once

#include <d3d11.h>
#include <memory>
#include <DirectXMath.h>
#include <d2d1_1.h>

namespace d2d {

	class CanvasLayer2D {
	private:
		struct SpriteVertex {
			DirectX::XMFLOAT3 pos;
			DirectX::XMFLOAT2 texture;
			SpriteVertex() {}
			SpriteVertex(float x, float y, float z,
				float texture_x, float texture_y) : pos(x, y, z), texture(texture_x, texture_y) {
			}
		};

	public:
		enum BlendMode { Default, PreMultiPlyedAlpha, Add, BlendMode_Max };

	private:
		std::shared_ptr<ID3D11Buffer>			_vertexBuffer;
		std::shared_ptr<ID3D11InputLayout>		_inputLayout;
		std::shared_ptr<ID3D11VertexShader>		_vertexShader;
		std::shared_ptr<ID3D11PixelShader>		_pixelShader;
		std::shared_ptr<ID3D11Buffer>			_constantBuffer;
		//!	深度テスト無効化
		std::shared_ptr<ID3D11DepthStencilState>	_depthStencilState;
		//!	アルファブレンド有効化、その他合成方法指定
		std::shared_ptr<ID3D11BlendState>		_blendState[BlendMode_Max];
		
		//!	2D->3D変換用
		int	_width, _height;


	public:
		void init(ID3D11Device* device, int canvasWidth, int canvasHeight);
		int getWidth()const {
			return _width;
		}
		int getHeight()const {
			return _height;
		}
		void updateConstantBuffer( ID3D11DeviceContext* context, const D2D_MATRIX_3X2_F& transform,
			const D2D_MATRIX_3X2_F& textureTransform);

		void renderTexture(ID3D11DeviceContext* context, ID3D11ShaderResourceView *texture, BlendMode blendMode = BlendMode::Default);

		//	各種リソースをセットして深度ステンシル無効化、描画開始
		void beginDraw(ID3D11DeviceContext* context) {
			//	深度ステンシルを一時的に無効化する。第二引数なんだこれ公式見てもわからんぞ
			context->OMSetDepthStencilState(_depthStencilState.get(), 0);

			context->IASetInputLayout(_inputLayout.get());
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);


			context->VSSetShader(_vertexShader.get(), nullptr, 0);
			context->PSSetShader(_pixelShader.get(), nullptr, 0);

			ID3D11Buffer* vbs[] = { _vertexBuffer.get() };
			const UINT stride = sizeof(SpriteVertex);
			const UINT offset = 0;
			context->IASetVertexBuffers(0, 1, vbs, &stride, &offset);

		}
		//	深度ステンシルを元に戻す
		void endDraw(ID3D11DeviceContext* context) {
			//	深度テストをデフォルトに戻す。デフォルトで良いのか？
			context->OMSetDepthStencilState(nullptr, 0);
		}
	};

}