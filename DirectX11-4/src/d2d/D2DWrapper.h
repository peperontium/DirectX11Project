#pragma once

#include <d2d1.h>
#include <dwrite.h>
#include <wchar.h>
#include <memory>

#include <stdexcept>


namespace d2 {
	//!	DXGIのサーフェイスへの描画を行うDirect2Dレンダーターゲットを作成
	std::shared_ptr<ID2D1RenderTarget> CreateDXGISurfaceRenderTarget(
		IDXGISurface *surface, const D2D1_RENDER_TARGET_PROPERTIES& rtp = D2D1::RenderTargetProperties());



	std::shared_ptr<ID2D1SolidColorBrush> CreateSolidColorBrush(
		ID2D1RenderTarget* renderTarget, const D2D1_COLOR_F &color);
	


	std::shared_ptr<IDWriteTextFormat> CreateTextFormat(
		LPCWSTR					fontName,		//	フォント名
		float					fontSize,		//	フォントサイズ
		DWRITE_FONT_WEIGHT		fontWeight = DWRITE_FONT_WEIGHT_NORMAL,		//	太さ
		DWRITE_FONT_STYLE		fontStyle  = DWRITE_FONT_STYLE_NORMAL,		//	スタイル(斜体等)
		DWRITE_FONT_STRETCH		fontStretch = DWRITE_FONT_STRETCH_NORMAL,	//	幅(横幅)
		IDWriteFontCollection*	fontCollection = nullptr,	//	フォントコレクション(nullptrでシステムの持つフォントを使う)
		LPCWSTR					locale = L"ja-jp"			//	ロケール、基本変更なしでいい
		);

} //	end of namespace d2