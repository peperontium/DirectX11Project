#include "D2DWrapper.h"

#include <stdexcept>
#pragma comment (lib, "d2d1.lib")

#include "../comUtil.h"

namespace {
	std::shared_ptr<ID2D1Factory> CreateD2D1Factory() {

		ID2D1Factory *pD2DFactory;
		HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, __uuidof(ID2D1Factory), (LPVOID*)&pD2DFactory);
		if (FAILED(hr))
			throw std::runtime_error("D2D1Factoryの作成に失敗しました");
		
		return std::shared_ptr<ID2D1Factory>(pD2DFactory, comUtil::ReleaseIUnknown);
	}
}

namespace d2 {


	std::shared_ptr<ID2D1RenderTarget> CreateDXGISurfaceRenderTarget(
		IDXGISurface *surface, const D2D1_RENDER_TARGET_PROPERTIES& rtp) {
		
		ID2D1RenderTarget* pRenderTarget = nullptr;
		HRESULT hr = CreateD2D1Factory()->CreateDxgiSurfaceRenderTarget(surface, &rtp, &pRenderTarget);
		if (FAILED(hr))
			throw std::runtime_error("DXGIサーフェイスへのレンダーターゲットの作成に失敗しました");

		return std::shared_ptr<ID2D1RenderTarget>(pRenderTarget, comUtil::ReleaseIUnknown);
		
	}


	std::shared_ptr<ID2D1SolidColorBrush> CreateSolidColorBrush(
		ID2D1RenderTarget *renderTarget, const D2D1_COLOR_F &color) {

		ID2D1SolidColorBrush *pBrush = nullptr;
		HRESULT hr = renderTarget->CreateSolidColorBrush(color, &pBrush);
		if (FAILED(hr))
			throw(std::runtime_error("ID2D1SolidColorBrush　色ブラシの作成に失敗しました"));
		
		return(std::shared_ptr<ID2D1SolidColorBrush>(pBrush, comUtil::ReleaseIUnknown));
	}


}