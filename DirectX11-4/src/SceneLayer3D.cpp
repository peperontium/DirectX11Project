#include "SceneLayer3D.h"

#include "./dx11/DX11ThinWrapper.h"
#include "./dx11/DX11DefaultSetting.h"

namespace d3d {

	const float SceneLayer3D::ClipNearZ = 0.5f;
	const float SceneLayer3D::ClipFarZ = 100.0f;


	void SceneLayer3D::init(ID3D11Device* device, const DXGI_MODE_DESC& displayMode, HWND hWnd) {

		_context = DX11ThinWrapper::d3::AccessD3Context(device);
		_swapChain = dx11::CreateDefaultSwapChain(&displayMode, hWnd, device, true);

		dx11::SetDefaultRenderTarget(_swapChain.get());
		dx11::SetDefaultViewport(_swapChain.get());

		// ターゲットビュー
		_renderTargetView = DX11ThinWrapper::d3::AccessRenderTargetViews(_context.get(), 1)[0];
		_depthStencilView = DX11ThinWrapper::d3::AccessDepthStencilView(_context.get());


		//	プロジェクション行列初期設定
		//	定数バッファは float*4の倍数のサイズである必要がある
		//	その他はまりそうなところ ： https://twitter.com/43x2/status/144821841977028608
		_projectionMtxBuffer = DX11ThinWrapper::d3::CreateConstantBuffer(
			device, nullptr, sizeof(DirectX::XMFLOAT4X4), D3D11_CPU_ACCESS_WRITE
			);
		_aspectRatio = displayMode.Width / static_cast<float>(displayMode.Height);

		_constantBuffer = DX11ThinWrapper::d3::CreateConstantBuffer(
			device, nullptr, ConstantBufferSize, D3D11_CPU_ACCESS_WRITE
			);
	}

	void SceneLayer3D::setCustomSamplar(ID3D11Device* device) {

		if (_samplerState == nullptr) {
			D3D11_SAMPLER_DESC samplerDesc = dx11::SamplerState(D3D11_FILTER_ANISOTROPIC, D3D11_TEXTURE_ADDRESS_WRAP);

			_samplerState = DX11ThinWrapper::d3::CreateSampler(device, samplerDesc);
		}

		ID3D11SamplerState* states[] = { _samplerState.get() };
		_context->PSSetSamplers(0, 1, states);
	}
	
	void SceneLayer3D::setCustomBlendState(ID3D11Device* device) {
		//	ブレンド設定
		
		if (_blendState == nullptr) {
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

		FLOAT blendFactor[] = { 1, 1, 1, 1 };
		_context->OMSetBlendState(_blendState.get(), blendFactor, 0xffffffff);
	}

	void SceneLayer3D::setProjection(float fovRadian, UINT startSlot) {

		//	視野角に変更がある場合バッファ更新
		if (_fovRadian != fovRadian) {
			DirectX::XMFLOAT4X4 param;
			DX11ThinWrapper::d3::mapping(_projectionMtxBuffer.get(), _context.get(), [&](D3D11_MAPPED_SUBRESOURCE resource) {
				auto param = static_cast<DirectX::XMFLOAT4X4 *>(resource.pData);

				DirectX::XMMATRIX mtxProj = DirectX::XMMatrixPerspectiveFovLH(
					fovRadian, _aspectRatio, ClipNearZ, ClipFarZ);
				XMStoreFloat4x4(param, DirectX::XMMatrixTranspose(mtxProj));
			});
		}

		_fovRadian = fovRadian;
		ID3D11Buffer * projectionBuffers[] = { _projectionMtxBuffer.get() };
		_context->VSSetConstantBuffers(startSlot, 1, projectionBuffers);

	}

	void SceneLayer3D::setConstants(std::function<void(D3D11_MAPPED_SUBRESOURCE)> mapFunc, UINT startSlot) {
		DX11ThinWrapper::d3::mapping(_constantBuffer.get(),_context.get(),mapFunc);

		ID3D11Buffer * constantBuffers[] = { _constantBuffer.get() };
		_context->VSSetConstantBuffers(startSlot, 1, constantBuffers);
	}
}
