#include "../DX11ThinWrapper.h"
#include <stdexcept>


namespace {
	DXGI_FORMAT TextureFormat2DepthStencilViewFormat(DXGI_FORMAT format) {
		switch (format) {
		case DXGI_FORMAT_R8_TYPELESS: return DXGI_FORMAT_R8_UNORM;
		case DXGI_FORMAT_R16_TYPELESS: return DXGI_FORMAT_D16_UNORM;
		case DXGI_FORMAT_R32_TYPELESS: return DXGI_FORMAT_D32_FLOAT;
		case DXGI_FORMAT_R24G8_TYPELESS: return DXGI_FORMAT_D24_UNORM_S8_UINT;
		default: return format;
		}
	}

	D3D11_TEXTURE2D_DESC CreateDepthBufferDESC(IDXGISwapChain * swapChain) {
		auto chainDesc = DX11ThinWrapper::d3::GetSwapChainDescription(swapChain);
		D3D11_TEXTURE2D_DESC descDS;
		::ZeroMemory(&descDS, sizeof(D3D11_TEXTURE2D_DESC));
		descDS.Width = chainDesc.BufferDesc.Width;
		descDS.Height = chainDesc.BufferDesc.Height;
		descDS.MipLevels = 1;                             // ミップマップを作成しない
		descDS.ArraySize = 1;
		descDS.Format = DXGI_FORMAT_R32_TYPELESS;
		descDS.SampleDesc.Count = chainDesc.SampleDesc.Count;
		descDS.SampleDesc.Quality = chainDesc.SampleDesc.Quality;
		descDS.Usage = D3D11_USAGE_DEFAULT;
		descDS.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		descDS.CPUAccessFlags = 0;
		descDS.MiscFlags = 0;
		return descDS;
	}
}

namespace DX11ThinWrapper {
	namespace d3 {
		DXGI_SWAP_CHAIN_DESC GetSwapChainDescription(IDXGISwapChain * swapChain){
			DXGI_SWAP_CHAIN_DESC chainDesc;
			auto hr = swapChain->GetDesc(&chainDesc);
			if (FAILED(hr)) throw std::runtime_error("スワップチェーンの設定が取得できませんでした.");
			return chainDesc;
		}
		std::shared_ptr<ID3D11RenderTargetView> CreateRenderTargetView(IDXGISwapChain * swapChain) {
			ID3D11RenderTargetView * view = nullptr;
			auto hr = AccessD3Device(swapChain)->CreateRenderTargetView(AccessBackBuffer(swapChain).get(), nullptr, &view);
			if (FAILED(hr)) throw std::runtime_error("レンダーターゲットビューの生成に失敗しました.");
			return std::shared_ptr<ID3D11RenderTargetView>(view, ReleaseIUnknown);
		}
		std::shared_ptr<ID3D11DepthStencilView> CreateDepthStencilView(IDXGISwapChain * swapChain) {
			auto device = AccessD3Device(swapChain);
			auto descDS = CreateDepthBufferDESC(swapChain);
			auto depthBuffer = CreateTexture2D(device.get(), descDS);

			D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
			::ZeroMemory(&descDSV, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
			descDSV.Format = TextureFormat2DepthStencilViewFormat(descDS.Format);
			descDSV.ViewDimension = (descDS.SampleDesc.Count > 1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
			descDSV.Texture2D.MipSlice = 0;

			ID3D11DepthStencilView * depthStencilView = nullptr;
			auto hr = device->CreateDepthStencilView(depthBuffer.get(), &descDSV, &depthStencilView);
			if (FAILED(hr)) throw std::runtime_error("深度ステンシルビューの生成に失敗しました.");

			return std::shared_ptr<ID3D11DepthStencilView>(depthStencilView, ReleaseIUnknown);
		}

		std::shared_ptr<IDXGISwapChain> CreateSwapChain(ID3D11Device * device, DXGI_SWAP_CHAIN_DESC sd) {
			IDXGISwapChain * swapChain = nullptr;
			auto hr = gi::AccessGIFactory(device)->CreateSwapChain(device, &sd, &swapChain);
			if (FAILED(hr)) throw std::runtime_error("スワップチェーンの生成に失敗しました.");
			return std::shared_ptr<IDXGISwapChain>(swapChain, ReleaseIUnknown);
		}

		UINT CheckMultisampleQualityLevels(ID3D11Device * device, DXGI_FORMAT format, UINT sampleCount) {
			UINT quality = 0;
			auto hr = device->CheckMultisampleQualityLevels(format, sampleCount, &quality);
			if (FAILED(hr)) throw std::runtime_error("品質レベルの数の取得に失敗しました.");
			return quality;
		}
	}
}