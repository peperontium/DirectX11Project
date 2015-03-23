#pragma once
#include <memory>
#include <d3d11.h>

namespace dx11 {
	// スワップチェインのバックバッファと、深度ステンシルバッファを描画ターゲットに設定
	void SetDefaultRenderTarget(IDXGISwapChain * swapChain);
	
	void SetDefaultViewport(IDXGISwapChain * swapChain);
	
	void SetDefaultRasterize(IDXGISwapChain * swapChain);


	// スワップチェインの生成
	std::shared_ptr<IDXGISwapChain> CreateDefaultSwapChain(
		DXGI_MODE_DESC * displayMode, HWND hWnd, ID3D11Device * device, bool useMultiSample
	);
};