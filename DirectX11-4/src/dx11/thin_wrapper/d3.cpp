#include "../DX11ThinWrapper.h"
#include <stdexcept>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")

namespace DX11ThinWrapper {
	namespace d3 {
		std::shared_ptr<ID3D11Device> InitDirect3D() {
			D3D_DRIVER_TYPE driverTypes[] = {
				D3D_DRIVER_TYPE_HARDWARE,
				D3D_DRIVER_TYPE_WARP,
				D3D_DRIVER_TYPE_REFERENCE,
				D3D_DRIVER_TYPE_SOFTWARE,
			};

			UINT createDeviceFlag = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(DEBUG) || defined(_DEBUG)
			createDeviceFlag |= D3D11_CREATE_DEVICE_DEBUG;
#endif

			D3D_FEATURE_LEVEL featureLevels[] = {
				D3D_FEATURE_LEVEL_11_1,
				D3D_FEATURE_LEVEL_11_0,
				D3D_FEATURE_LEVEL_10_1,
				D3D_FEATURE_LEVEL_10_0,
			};

			ID3D11Device * device = nullptr;
			for (auto type : driverTypes) {
				auto hr = ::D3D11CreateDevice(
					nullptr,            // 使用するアダプターを設定。nullptrの場合はデフォルトのアダプター。
					type,               // D3D_DRIVER_TYPEのいずれか。ドライバーの種類。
					                    // pAdapterが nullptr 以外の場合は、D3D_DRIVER_TYPE_UNKNOWNを指定する。
					nullptr,            // ソフトウェアラスタライザを実装するDLLへのハンドル。
					                    // D3D_DRIVER_TYPE を D3D_DRIVER_TYPE_SOFTWARE に設定している場合は nullptr にできない。
					createDeviceFlag,   // D3D11_CREATE_DEVICE_FLAGの組み合わせ。デバイスを作成時に使用されるパラメータ。
					featureLevels,      // D3D_FEATURE_LEVELのポインタ
					_countof(featureLevels), // D3D_FEATURE_LEVEL配列の要素数
					D3D11_SDK_VERSION,  // DirectX SDKのバージョン。この値は固定。
					&device,            // 初期化されたデバイス
					nullptr,            // 採用されたフィーチャーレベル
					nullptr             // 初期化されたデバイスコンテキスト
				);
				if (SUCCEEDED(hr)) break;
			}
			if (!device) throw std::runtime_error("ID3D11Deviceの生成に失敗しました.");
			return std::shared_ptr<ID3D11Device>(device, comUtil::ReleaseIUnknown);
		}

		std::shared_ptr<ID3D11Texture2D> CreateTexture2D(ID3D11Device * device, const D3D11_TEXTURE2D_DESC & descDS) {
			ID3D11Texture2D* buffer = nullptr;
			auto hr = device->CreateTexture2D(&descDS, nullptr, &buffer);
			if (FAILED(hr)) throw std::runtime_error("ID3D11Texture2Dの生成に失敗しました.");
			return std::shared_ptr<ID3D11Texture2D>(buffer, comUtil::ReleaseIUnknown);
		}

		std::shared_ptr<ID3D11Texture2D> AccessBackBuffer(IDXGISwapChain * swapChain) {
			ID3D11Texture2D * backBuffer = nullptr;
			auto hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
			if (FAILED(hr)) throw std::runtime_error("スワップチェーンからバックバッファへのアクセスに失敗しました.");
			return std::shared_ptr<ID3D11Texture2D>(backBuffer, comUtil::ReleaseIUnknown);
		}
		std::shared_ptr<ID3D11Device> AccessD3Device(IDXGISwapChain * swapChain) {
			ID3D11Device * device = nullptr;
			auto hr = swapChain->GetDevice(__uuidof(ID3D11Device), (void **)&device);
			if (FAILED(hr)) throw std::runtime_error("スワップチェーンからデバイスへのアクセスに失敗しました.");
			return std::shared_ptr<ID3D11Device>(device, comUtil::ReleaseIUnknown);
		}
		std::shared_ptr<ID3D11DeviceContext> AccessD3Context(ID3D11Device * device) {
			ID3D11DeviceContext * context = nullptr;
			device->GetImmediateContext(&context);
			return std::shared_ptr<ID3D11DeviceContext>(context, comUtil::ReleaseIUnknown);
		}
		std::shared_ptr<ID3D11DeviceContext> AccessD3Context(IDXGISwapChain * swapChain) {
			return AccessD3Context(AccessD3Device(swapChain).get());
		}

		std::shared_ptr<ID3D11DepthStencilView> AccessDepthStencilView(ID3D11DeviceContext * context) {
			ID3D11DepthStencilView * dv;
			context->OMGetRenderTargets(1, NULL, &dv);
			return std::shared_ptr<ID3D11DepthStencilView>(dv, comUtil::ReleaseIUnknown);
		}
		std::vector<std::shared_ptr<ID3D11RenderTargetView>> AccessRenderTargetViews(
			ID3D11DeviceContext * context, UINT numOfViews
		) {
			ID3D11RenderTargetView * rv;
			context->OMGetRenderTargets(numOfViews, &rv, nullptr);

			std::vector<std::shared_ptr<ID3D11RenderTargetView>> rvs;
			for (UINT i = 0; i < numOfViews; ++i) rvs.emplace_back(rv + i, comUtil::ReleaseIUnknown);
			
			return rvs;
		}

		std::shared_ptr<ID3D11Resource> AccessResource(ID3D11View * view) {
			ID3D11Resource * resource;
			view->GetResource(&resource);
			return std::shared_ptr<ID3D11Resource>(resource, comUtil::ReleaseIUnknown);
		}

		std::shared_ptr<ID3D11SamplerState> CreateSampler(ID3D11Device * device, const D3D11_SAMPLER_DESC & desc) {
			ID3D11SamplerState * sampler;
			auto hr = device->CreateSamplerState(&desc, &sampler);
			if (FAILED(hr)) throw std::runtime_error("サンプラーの作成に失敗しました.");
			return std::shared_ptr<ID3D11SamplerState>(sampler, comUtil::ReleaseIUnknown);
		}

		std::shared_ptr<ID3D11BlendState> CreateBlendState(ID3D11Device * device, const D3D11_BLEND_DESC *desc) {
			ID3D11BlendState * blendState;
			auto hr = device->CreateBlendState(desc, &blendState);
			if (FAILED(hr)) throw std::runtime_error("ブレンドステートの作成に失敗しました.");
			return std::shared_ptr<ID3D11BlendState>(blendState, comUtil::ReleaseIUnknown);
		}


		std::shared_ptr<ID3D11DepthStencilState> CreateDepthStencilState(ID3D11Device* device, const D3D11_DEPTH_STENCIL_DESC& desc) {
			ID3D11DepthStencilState* dss;
			HRESULT hr = device->CreateDepthStencilState(&desc, &dss);
			if (FAILED(hr))	throw std::runtime_error("ID3D11DepthStencilStateの作成に失敗");
			return std::shared_ptr<ID3D11DepthStencilState>(dss, comUtil::ReleaseIUnknown);

		};


		//	テクスチャからシェーダー・リソース・ビューを作成
		std::shared_ptr<ID3D11ShaderResourceView> CreateShaderResourceView(ID3D11Device* device, ID3D11Texture2D * resourceTexture) {

			ID3D11ShaderResourceView * srv;
			auto hr = device->CreateShaderResourceView(resourceTexture, nullptr, &srv);
			if (FAILED(hr)) throw std::runtime_error("ID3D11ShaderResourceViewの作成に失敗しました.");
			return std::shared_ptr<ID3D11ShaderResourceView>(srv, comUtil::ReleaseIUnknown);
		};

		

		void mapping(
			ID3D11Resource * buffer, ID3D11DeviceContext * context, std::function<void(D3D11_MAPPED_SUBRESOURCE)> function
		) {
			D3D11_MAPPED_SUBRESOURCE resource;
			auto hr = context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
			if (SUCCEEDED(hr)) {
				function(resource);
				context->Unmap(buffer, 0);
			}
		}
	}
}