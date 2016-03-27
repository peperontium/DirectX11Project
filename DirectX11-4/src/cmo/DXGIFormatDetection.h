#pragma once


#include <dxgiformat.h>
#include <type_traits>


//	ƒ‚ƒfƒ‹‚ÌIndex‚ÌŒ^‚ÆDXGI_FORMAT‚ÌŠÖ˜A•t‚¯
template <typename T>	struct IndexFormatof {
	static const DXGI_FORMAT Bone = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN; 
	static const DXGI_FORMAT Vertex	= DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	static_assert(!std::is_same<T, T>::value, "Unknown index format type");
};

template <>	struct IndexFormatof<uint8_t> {
	static const DXGI_FORMAT Bone = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT;
	static const DXGI_FORMAT Vertex	= DXGI_FORMAT::DXGI_FORMAT_R8_UINT;
};

template <>	struct IndexFormatof<uint16_t> {
	static const DXGI_FORMAT Bone = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT;
	static const DXGI_FORMAT Vertex	= DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
};

template <>	struct IndexFormatof<uint32_t> {
	static const DXGI_FORMAT Bone = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_UINT;
	static const DXGI_FORMAT Vertex	= DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
};