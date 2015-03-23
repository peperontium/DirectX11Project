#include "../DX11ThinWrapper.h"

#include <stdexcept>
#include "../WICTextureLoader.h"

namespace DX11ThinWrapper{
	namespace d3{
		std::shared_ptr<ID3D11ShaderResourceView> CreateWICTextureFromFile(ID3D11Device * device, const wchar_t * path){
			ID3D11ShaderResourceView * textureView;
			auto hr = DirectX::CreateWICTextureFromFile(device, path, nullptr, &textureView);
			if (FAILED(hr)) throw std::runtime_error("テクスチャファイルが読み込めませんでした．");
			return std::shared_ptr<ID3D11ShaderResourceView>(textureView, ReleaseIUnknown);
		}
	}
}