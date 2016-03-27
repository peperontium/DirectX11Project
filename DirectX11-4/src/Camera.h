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
		DirectX::XMFLOAT4X4 _cameraMtx;
		bool _mtxDirty;

		const static DirectX::XMFLOAT4 ZeroEyePosition;
		static std::weak_ptr<ID3D11Buffer> s_mtxConstBufferShared;


	public:
		Camera(ID3D11Device* device);
		void setEyePosition(const DirectX::XMFLOAT4& eyePosition) {
			_eyePosition = eyePosition;
			_mtxDirty = true;
		}
		void setTracePosition(const DirectX::XMFLOAT4* pTracePosition) {
			if (pTracePosition == nullptr)
				_pTracePosition = &ZeroEyePosition;
			else
				_pTracePosition = pTracePosition;

			_mtxDirty = true;
		}
		void rotateX(float radian) {
			DirectX::XMStoreFloat4(&_eyePosition,
				DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&_eyePosition), DirectX::XMMatrixRotationX(radian))
				);
			_mtxDirty = true;
		}
		void rotateY(float radian) {
			DirectX::XMStoreFloat4(&_eyePosition,
				DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&_eyePosition), DirectX::XMMatrixRotationY(radian))
				);
			_mtxDirty = true;
		}
		void rotateZ(float radian) {
			DirectX::XMStoreFloat4(&_eyePosition,
				DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&_eyePosition), DirectX::XMMatrixRotationZ(radian))
				);
			_mtxDirty = true;
		}
		void update() {
			if (_mtxDirty) {
				auto mtxView = DirectX::XMMatrixLookAtLH(
					DirectX::XMLoadFloat4(&_eyePosition),
					DirectX::XMLoadFloat4(_pTracePosition),
					DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1)
					);
				XMStoreFloat4x4(&_cameraMtx, mtxView);

				_mtxDirty = false;
			}
		}
		DirectX::XMFLOAT4X4 getMatrix()const {
			return _cameraMtx;
		}
		void setBuffer(SceneLayer3D* scene3d, UINT startSlot)const;
	};
}