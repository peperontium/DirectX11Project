#include "D2DCanvasLayer.h"

#include "./ShaderBytes.h"



namespace {

	//	テクスチャからシェーダー・リソース・ビューを作成
	std::shared_ptr<ID3D11ShaderResourceView> CreateShaderResourceView(ID3D11Device* device, ID3D11Texture2D * resourceTexture) {

		ID3D11ShaderResourceView * srv;
		auto hr = device->CreateShaderResourceView(resourceTexture, nullptr, &srv);
		if (FAILED(hr)) throw std::runtime_error("ID3D11ShaderResourceViewの作成に失敗しました.");
		return std::shared_ptr<ID3D11ShaderResourceView>(srv, DX11ThinWrapper::ReleaseIUnknown);
	};

	//	深度ステンシルビュー設定オブジェクトの作成
	std::shared_ptr<ID3D11DepthStencilState> CreateDepthStencilState(ID3D11Device* device, const D3D11_DEPTH_STENCIL_DESC& desc) {
		ID3D11DepthStencilState* dss;
		HRESULT hr = device->CreateDepthStencilState(&desc, &dss);
		if (FAILED(hr))	throw std::runtime_error("ID3D11DepthStencilStateの作成に失敗");
		return std::shared_ptr<ID3D11DepthStencilState>(dss, DX11ThinWrapper::ReleaseIUnknown);

	};
}


namespace d2 {

	std::weak_ptr<D2DCanvasLayer::RenderingLayer>	D2DCanvasLayer::s_sharedPaintLayer;

	void D2DCanvasLayer::RenderingLayer::init(ID3D11Device* device) {

		static D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		_vertexShader = DX11ThinWrapper::d3::CreateVertexShader(device, cso::compiledShader::simpleVS, sizeof(cso::compiledShader::simpleVS));
		_inputLayout = DX11ThinWrapper::d3::CreateInputLayout(device, layout, _countof(layout), cso::compiledShader::simpleVS, sizeof(cso::compiledShader::simpleVS));

		_pixelShader = DX11ThinWrapper::d3::CreatePixelShader(
			device, cso::compiledShader::simplePS, sizeof(cso::compiledShader::simplePS)
			);

		CanvasVertex verts[] = {
			CanvasVertex(-1.0f, -1.0f, 0.0f, 0.0f, 1.0f),
			CanvasVertex(-1.0f, 1.0f, 0.0f, 0.0f, 0.0f),
			CanvasVertex(1.0f, -1.0f, 0.0f, 1.0f, 1.0f),
			CanvasVertex(1.0f, 1.0f, 0.0f, 1.0f, 0.0f),
		};

		_vertexBuffer = DX11ThinWrapper::d3::CreateVertexBuffer(device, verts, sizeof(CanvasVertex)* 4);


		//	深度テストを無効にするための深度ステンシルステートの作成
		D3D11_DEPTH_STENCIL_DESC dsd = {};
		dsd.DepthEnable = FALSE;
		dsd.StencilEnable = FALSE;
		_depthStencilState = CreateDepthStencilState(device, dsd);

		//	αブレンド設定
		D3D11_BLEND_DESC blendDesc[] = { {
				FALSE, FALSE, {
					{
						TRUE,
						D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD,
						D3D11_BLEND_ZERO, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD,
						D3D11_COLOR_WRITE_ENABLE_ALL
					}
				}
			} };

		_blendState = DX11ThinWrapper::d3::CreateBlendState(device, blendDesc);
	}

	void D2DCanvasLayer::RenderingLayer::draw(ID3D11DeviceContext* context, ID3D11ShaderResourceView *texture){

		//	深度ステンシルを一時的に無効化する。第二引数なんだこれ公式見てもわからんぞ
		context->OMSetDepthStencilState(_depthStencilState.get(), 0);

		//	αブレンド有効化
		FLOAT blendFactor[] = { 1, 1, 1, 1 };
		context->OMSetBlendState(_blendState.get(), blendFactor, 0xffffffff);

		context->IASetInputLayout(_inputLayout.get());
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		

		context->VSSetShader(_vertexShader.get(), nullptr, 0);
		context->PSSetShader(_pixelShader.get(), nullptr, 0);

		ID3D11Buffer* vbs[] = { _vertexBuffer.get() };
		const UINT stride = sizeof(CanvasVertex);
		const UINT offset = 0;
		context->IASetVertexBuffers(0, 1, vbs, &stride, &offset);

		context->PSSetShaderResources(0, 1, &texture);

		context->Draw(4, 0);

		//	深度テストをデフォルトに戻す。デフォルトで良いのか？
		context->OMSetDepthStencilState(nullptr, 0);
	}

	void D2DCanvasLayer::init(IDXGISwapChain* swapchain) {

		using namespace DX11ThinWrapper;

		// 共有するテクスチャを D3D 11 から用意。
		// ここではバックバッファと同じサイズのテクスチャを作成。
		D3D11_TEXTURE2D_DESC backBufferDesc, canvasDesc;

		auto backBuffer3D = d3::AccessBackBuffer(swapchain);

		backBuffer3D->GetDesc(&backBufferDesc);

		// 作成するテクスチャ情報の設定。
		// ・DXGI_FORMAT_B8G8R8A8_UNORM は固定。
		// ・D3D11_BIND_RENDER_TARGET は D2D での描画対象とするために必須。
		// ・D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX はテクスチャを共有するのに必須。
		ZeroMemory(&canvasDesc, sizeof(canvasDesc));
		canvasDesc.Width = backBufferDesc.Width;
		canvasDesc.Height = backBufferDesc.Height;
		canvasDesc.MipLevels = 1;		//非ミップマップテクスチャのみ共有可能
		canvasDesc.ArraySize = 1;
		canvasDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		canvasDesc.SampleDesc.Count = 1;
		canvasDesc.Usage = D3D11_USAGE_DEFAULT;
		canvasDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		canvasDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

		
		ID3D11Device* device = d3::AccessD3Device(swapchain).get();
		auto tex2D = d3::CreateTexture2D(device, canvasDesc);
		
		//	共有のためのD3D11のキーミューテックスを取得
		_keyedMutex11 = QueryInterface<IDXGIKeyedMutex>(tex2D.get());


		//	DX10.1側で使う共有サーフェイスを作成
		auto surface10 = dx10::OpenSharedResource<IDXGISurface1>(tex2D.get());
		

		//	共有のためのD3D10.1のキーミューテックスを取得
		_keyedMutex10 = QueryInterface<IDXGIKeyedMutex>(surface10.get());

		// D2D のレンダーターゲットを D3D 10.1 の共有サーフェイスから生成
		_renderTarget2D = d2::CreateDXGISurfaceRenderTarget(
			surface10.get(), D2D1::RenderTargetProperties(
			D2D1_RENDER_TARGET_TYPE_HARDWARE, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED))
			);


		//	共有テクスチャを元にシェーダーリソースビューの作成
		_deviceSharedTexture = CreateShaderResourceView(device, tex2D.get());


		_paintLayer = D2DCanvasLayer::s_sharedPaintLayer.lock();
		if (_paintLayer == nullptr) {
			D2DCanvasLayer::s_sharedPaintLayer = _paintLayer = std::make_shared<RenderingLayer>();
			_paintLayer->init(device);
		}
	}
	
}