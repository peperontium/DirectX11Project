#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <memory>

namespace d3d {

	class Camera {
	private:
		DirectX::XMFLOAT4	_eyePosition;
		const DirectX::XMFLOAT4*	_pTracePosition;
		std::shared_ptr<ID3D11Buffer>	_mtxConstBuffer;

		const static DirectX::XMFLOAT4 ZeroEyePosition;

		static std::weak_ptr<ID3D11Buffer> s_mtxConstBufferShared;


	public:
		Camera(ID3D11Device* device);
		void setEyePosition(const DirectX::XMFLOAT4& eyePosition) {
			_eyePosition = eyePosition;
		}
		void setTracePosition(const DirectX::XMFLOAT4* pTracePosition) {
			if (pTracePosition == nullptr)
				_pTracePosition = &ZeroEyePosition;
			else
				_pTracePosition = pTracePosition;
		}
		void rotateX(float radian) {
			DirectX::XMStoreFloat4(&_eyePosition,
				DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&_eyePosition), DirectX::XMMatrixRotationX(radian))
				);
		}
		void rotateY(float radian) {
			DirectX::XMStoreFloat4(&_eyePosition,
				DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&_eyePosition), DirectX::XMMatrixRotationY(radian))
				);
		}
		void rotateZ(float radian) {
			DirectX::XMStoreFloat4(&_eyePosition,
				DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&_eyePosition), DirectX::XMMatrixRotationZ(radian))
				);
		}
		void multiplyMatrix(const DirectX::XMMATRIX& mtx) {
			DirectX::XMStoreFloat4(&_eyePosition,
				DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&_eyePosition), mtx)
				);
		}
		void setBuffer(SceneLayer3D* scene3d, UINT startSlot)const;
	};
}