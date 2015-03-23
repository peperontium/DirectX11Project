#pragma once

#include "./DX10Wrapper1.h"
#include "./D2DWrapper.h"

//	最終的にd2名前空間に含めるかは未定

//	DX10.1デバイスの作成後にこのキャンバスレイヤーを作成させるような設計に
namespace d2 {


	class D2DCanvasLayer {
	protected:
		//!	3Dでのキャンバスデータ
		//!	構造体名微妙な気がする
		class RenderingLayer {
		private:
			struct CanvasVertex {
				DirectX::XMFLOAT3 pos;
				DirectX::XMFLOAT2 texture;
				CanvasVertex() {}
				CanvasVertex(float x, float y, float z,
					float texture_x, float texture_y) : pos(x, y, z), texture(texture_x, texture_y) {
				}
			};
			std::shared_ptr<ID3D11Buffer>		_vertexBuffer;
			std::shared_ptr<ID3D11InputLayout>	_inputLayout;
			std::shared_ptr<ID3D11VertexShader>	_vertexShader;
			std::shared_ptr<ID3D11PixelShader>	_pixelShader;
			//!	深度テスト無効化するために
			std::shared_ptr<ID3D11DepthStencilState>	_depthStencilState;
			//!	アルファブレンド有効化
			std::shared_ptr<ID3D11BlendState>	_blendState;

		public:
			void draw(ID3D11DeviceContext* context, ID3D11ShaderResourceView *texture);
			void init(ID3D11Device *device);
		};

	protected:
		//!	DX11とDX10.1間の共有テクスチャ。これを経由してD3レイヤーへの描画を行う
		std::shared_ptr<ID3D11ShaderResourceView>	_deviceSharedTexture;

		//!	DX11とDX10.1の排他制御用Mutex
		std::shared_ptr<IDXGIKeyedMutex>	_keyedMutex10, _keyedMutex11;

		//!	Direct2D側のレンダーターゲット
		std::shared_ptr<ID2D1RenderTarget>	_renderTarget2D;

		//!	描画に必要なものまとめ
		std::shared_ptr<RenderingLayer>	_paintLayer;

		//!	オブジェクト間共有可能リソース
		static std::weak_ptr<RenderingLayer>		s_sharedPaintLayer;

	public:
		D2DCanvasLayer() {}
		~D2DCanvasLayer() {}
		
		//!	初期化
		void init(IDXGISwapChain* swapchain);
		//!	Direct2Dでの描画を開始、返り値のID2D1RenderTargetを用いて描画する
		std::weak_ptr<ID2D1RenderTarget> beginDraw() {
			
			// D3D 11 側からのテクスチャの使用を中断して
			_keyedMutex11->ReleaseSync(0);
			// D3D 10.1 (D2D) 側からのテクスチャの使用を開始
			// 待機時間は無限としています。
			_keyedMutex10->AcquireSync(0, INFINITE);

			// 描画開始
			_renderTarget2D->BeginDraw();
			_renderTarget2D->Clear(D2D1::ColorF(0.0f,0.0f,0.0f,0.0f));


			return _renderTarget2D;
		}
		//!	Direct2Dでの描画を終了、結果を3Dレイヤーに描画
		void endDraw(ID3D11DeviceContext* context) {

			// 描画終了
			_renderTarget2D->EndDraw();
			
			// D3D 10.1 (D2D) 側からのテクスチャの使用を終了して
			_keyedMutex10->ReleaseSync(1);
			// D3D 11 側からのテクスチャの使用を開始する
			_keyedMutex11->AcquireSync(1, INFINITE);

			// D2Dで描画したテクスチャをD3D側のバックバッファへ転送
			_paintLayer->draw(context,_deviceSharedTexture.get());
		}
		
		//!	このキャンバスへの描画に使うブラシを作成する
		std::shared_ptr<ID2D1SolidColorBrush> createBrush(const D2D1::ColorF& color = D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f)) {
			
			return d2::CreateSolidColorBrush(_renderTarget2D.get(),color);
		}
	};

}