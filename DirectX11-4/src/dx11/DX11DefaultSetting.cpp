#include "DX11DefaultSetting.h"

namespace dx11 {

	D3D11_SAMPLER_DESC SamplerState(D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addressMode) {
		D3D11_SAMPLER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));

		desc.Filter = filter;
		desc.AddressU = addressMode;
		desc.AddressV = addressMode;
		desc.AddressW = addressMode;
		desc.MaxAnisotropy = 16;
		desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		desc.MaxLOD = D3D11_FLOAT32_MAX;

		return desc;
	}
}