#include "D2DWrapper.h"


#pragma comment (lib, "Dwrite.lib")

#include "../comUtil.h"


namespace {

	std::shared_ptr<IDWriteFactory> CreateDWriteFactory() {

		IDWriteFactory* pWriteFactory = nullptr;
		HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(&pWriteFactory));

		if (FAILED(hr))
			throw std::runtime_error("DWriteFactoryの作成に失敗しました");

		return std::shared_ptr<IDWriteFactory>(pWriteFactory, comUtil::ReleaseIUnknown);
	}
}


namespace d2d {


	std::shared_ptr<IDWriteTextFormat> CreateTextFormat(
		LPCWSTR					fontName,		//	フォント名
		float					fontSize,		//	フォントサイズ
		DWRITE_FONT_WEIGHT		fontWeight,		//	太さ
		DWRITE_FONT_STYLE		fontStyle,		//	スタイル(斜体等)
		DWRITE_FONT_STRETCH		fontStretch,	//	幅(横幅)
		IDWriteFontCollection*	fontCollection,	//	フォントコレクション(nullptrでシステムの持つフォントを使う)
		LPCWSTR					locale	//	ロケール、基本変更なしでいい
		) {

		IDWriteTextFormat* pFormat = nullptr;
		HRESULT hr = CreateDWriteFactory()->CreateTextFormat(
			fontName,fontCollection,fontWeight,fontStyle,fontStretch,fontSize,locale,&pFormat);

		if (FAILED(hr))
			throw std::runtime_error("IDWriteTextFormatの作成に失敗しました");

		return std::shared_ptr<IDWriteTextFormat>(pFormat, comUtil::ReleaseIUnknown);

	}
}