#include "WICThinWrapper.h"
#include <stdexcept>

#pragma comment(lib, "WindowsCodecs.lib")

namespace WICThinWrapper {
	win::com_ptr<IWICImagingFactory> CreateWicFactory() {
		IWICImagingFactory * factory = nullptr;
		auto hr = ::CoCreateInstance(
			CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (void**)&factory
		);
		if (FAILED(hr)) throw std::runtime_error("IWICImagingFactoryÇÃê∂ê¨Ç…é∏îsÇµÇ‹ÇµÇΩ.");
		return win::com_ptr<IWICImagingFactory>(factory);
	}
	win::com_ptr<IWICBitmapDecoder> CreateDecoder(const TCHAR * path) {
		IWICBitmapDecoder * dec = nullptr;
		auto hr = CreateWicFactory()->CreateDecoderFromFilename(
			path, NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &dec
		);

		if (FAILED(hr)) throw std::runtime_error("IWICBitmapDecoderÇÃê∂ê¨Ç…é∏îsÇµÇ‹ÇµÇΩ.");
		return win::com_ptr<IWICBitmapDecoder>(dec);
	}
	win::com_ptr<IWICBitmapFrameDecode> AccessFrame(IWICBitmapDecoder * dec) {
		IWICBitmapFrameDecode * frame = nullptr;
		if (FAILED(dec->GetFrame(0, &frame))) throw std::runtime_error("IWICBitmapFrameDecodeÇÃê∂ê¨Ç…é∏îsÇµÇ‹ÇµÇΩ.");
		return win::com_ptr<IWICBitmapFrameDecode>(frame);
	}
	win::com_ptr<IWICFormatConverter> CreateConverter() {
		IWICFormatConverter * converter = nullptr;
		auto hr = CreateWicFactory()->CreateFormatConverter(&converter);
		if (FAILED(hr)) throw std::runtime_error("IWICFormatConverterÇÃê∂ê¨Ç…é∏îsÇµÇ‹ÇµÇΩ.");
		return win::com_ptr<IWICFormatConverter>(converter);
	}
};