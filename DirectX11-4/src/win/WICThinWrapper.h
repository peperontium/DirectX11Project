#pragma once
#include <wincodec.h>
#include <wincodecsdk.h>
#include "com_ptr.hpp"

namespace WICThinWrapper {
	win::com_ptr<IWICImagingFactory> CreateWicFactory();
	win::com_ptr<IWICBitmapDecoder> CreateDecoder(const TCHAR * path);
	win::com_ptr<IWICBitmapFrameDecode> AccessFrame(IWICBitmapDecoder * dec);
	win::com_ptr<IWICFormatConverter> CreateConverter();
};