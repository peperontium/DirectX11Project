#include "Camera.h"

#include "./dx11/DX11ThinWrapper.h"

namespace d3 {
	std::weak_ptr<ID3D11Buffer> Camera::s_mtxConstBufferShared;
	
	const DirectX::XMFLOAT4 Camera::ZeroEyePosition = DirectX::XMFLOAT4(0,0,0,1);

	Camera::Camera(ID3D11Device* device) :
		_eyePosition(0,0,-8.0,1),
		_pTracePosition(&ZeroEyePosition)
	{

		DirectX::XMFLOAT4X4 param;
		DirectX::XMMATRIX mtxView = DirectX::XMMatrixTranspose(
			DirectX::XMMatrixLookAtLH(
			DirectX::XMLoadFloat4(&_eyePosition),
			DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1),
			DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1)
			)
			);
		XMStoreFloat4x4(&param, DirectX::XMMatrixTranspose(mtxView));


		auto pBuf = Camera::s_mtxConstBufferShared.lock();
		if (pBuf == nullptr) {
			Camera::s_mtxConstBufferShared = _mtxConstBuffer = DX11ThinWrapper::d3::CreateConstantBuffer(
				device,	&param, sizeof(DirectX::XMFLOAT4X4), D3D11_CPU_ACCESS_WRITE
				);
		} else {
			_mtxConstBuffer = pBuf;
		}
	}

	void Camera::setBuffer(ID3D11DeviceContext* context, UINT startSlot)const {

		const DirectX::XMFLOAT4& eyePos =  _eyePosition;
		const DirectX::XMFLOAT4 * pTarget = _pTracePosition;

		DX11ThinWrapper::d3::mapping(_mtxConstBuffer.get(), context, [&](D3D11_MAPPED_SUBRESOURCE resource){
			auto param = static_cast<DirectX::XMFLOAT4X4 *>(resource.pData);
			auto mtxView = DirectX::XMMatrixLookAtLH(
				DirectX::XMLoadFloat4(&eyePos),
				DirectX::XMLoadFloat4(pTarget),
				DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1)
				);
			XMStoreFloat4x4(param, DirectX::XMMatrixTranspose(mtxView));
		});
		ID3D11Buffer * cameraBuffers[] = { _mtxConstBuffer.get() };
		//	スロット1にカメラ行列を設定
		context->VSSetConstantBuffers(startSlot, 1, cameraBuffers);
	}
}