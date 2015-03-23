#include "DX10Wrapper1.h"

#pragma comment (lib, "d3d10_1.lib")
#pragma comment (lib, "d2d1.lib")


#include "../dx11/DX11ThinWrapper.h"

namespace {

	std::weak_ptr<ID3D10Device1> g_device10;


	std::shared_ptr<ID3D10Device1> initDirect3DDevice10_1(ID3D11Device *pDX11Device) {
		
		UINT createDeviceFlag = D3D10_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(DEBUG) || defined(_DEBUG)
		createDeviceFlag |= D3D10_CREATE_DEVICE_DEBUG;
#endif

		ID3D10Device1* device10 = nullptr;

		//	DX11と共用のため、アダプターは必ず同じものを用いる。
		//	D3D10_DRIVER_TYPE_HARDWARE と D3D10_CREATE_DEVICE_BGRA_SUPPORT を指定。
		HRESULT hr = D3D10CreateDevice1(
			DX11ThinWrapper::gi::AccessAdapter(pDX11Device).get(),
			D3D10_DRIVER_TYPE_HARDWARE,
			nullptr,
			createDeviceFlag,
			D3D10_FEATURE_LEVEL_9_3,
			D3D10_1_SDK_VERSION,
			&device10
			);
		if (SUCCEEDED(hr)) {
			return std::shared_ptr<ID3D10Device1>(device10, DX11ThinWrapper::ReleaseIUnknown);
		}
		
		throw std::runtime_error("ID3D10Device1の生成に失敗しました.");
		return std::shared_ptr<ID3D10Device1>();

	}

}

namespace dx10 {
	DX10DeviceSharedGuard::DX10DeviceSharedGuard(ID3D11Device *pDX11Device) {
		if (auto device = g_device10.lock()) {
			_device = device;
		} else {
			g_device10 = _device = initDirect3DDevice10_1(pDX11Device);
		}
	}

	::ID3D10Device1 * AccessDX10Device() {
		return g_device10.lock().get();
	}


	namespace thin_template {
		LPVOID OpenSharedResource_base(ID3D11Texture2D* texture, const IID& uuid){


			HANDLE sharedHandle;
			// 共有のためのハンドルを取得
			if (FAILED(DX11ThinWrapper::QueryInterface<IDXGIResource>(texture)->GetSharedHandle(&sharedHandle)))
				throw std::runtime_error("共有ハンドルの作成に失敗");


			LPVOID sharedObject = nullptr;
			// DX10.1 で共有オブジェクトを生成
			if (FAILED(AccessDX10Device()->OpenSharedResource(sharedHandle, uuid, &sharedObject)))
				throw std::runtime_error("共有リソースの作成に失敗");	
			
			return sharedObject;
		}

	} //	end of namespace thin_template

} //	end of namespace dx10 