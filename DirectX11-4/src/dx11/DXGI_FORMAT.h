#pragma once
#include <cstdint>
#include <dxgiformat.h>

#ifdef INDEX_UINT16
#define INDEX_FORMAT_ENUM DXGI_FORMAT_R16_UINT
#define INDEX_FORMAT_TYPE uint16_t
#elif INDEX_UINT32
#define INDEX_FORMAT_ENUM DXGI_FORMAT_R32_UINT
#define INDEX_FORMAT_TYPE UINT
#else
static_assert(false, "DXGI_FORMAT is not defined.");
#endif