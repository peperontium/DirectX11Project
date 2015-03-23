#include "../DX11ThinWrapper.h"
#include <stdexcept>

namespace {
	std::shared_ptr<ID3D11Buffer> CreateBuffer(
		ID3D11Device* device, void* data, size_t size, UINT cpuAccessFlag, D3D11_BIND_FLAG BindFlag
	) {
		auto resource = std::make_shared<D3D11_SUBRESOURCE_DATA>();
		resource->pSysMem = data;
		resource->SysMemPitch = 0;
		resource->SysMemSlicePitch = 0;

		D3D11_BUFFER_DESC BufferDesc = {};
		BufferDesc.ByteWidth = size;
		BufferDesc.CPUAccessFlags = cpuAccessFlag;
		BufferDesc.Usage = (cpuAccessFlag == 0) ? D3D11_USAGE_DEFAULT : D3D11_USAGE_DYNAMIC;
		BufferDesc.MiscFlags = 0;
		BufferDesc.BindFlags = BindFlag;
		if (BindFlag == D3D11_BIND_STREAM_OUTPUT) BufferDesc.BindFlags |= D3D11_BIND_VERTEX_BUFFER;

		ID3D11Buffer* buffer = nullptr;
		auto hr = device->CreateBuffer(&BufferDesc, (data == nullptr) ? nullptr : resource.get(), &buffer);
		if (FAILED(hr)) throw std::runtime_error("ID3D11BufferÇÃê∂ê¨Ç…é∏îsÇµÇ‹ÇµÇΩ.");
		return std::shared_ptr<ID3D11Buffer>(buffer, DX11ThinWrapper::ReleaseIUnknown);
	}
}

namespace DX11ThinWrapper{
	namespace d3{
		std::shared_ptr<ID3D11Buffer> CreateVertexBuffer(ID3D11Device* device, void* data, size_t size, UINT cpuAccessFlag) {
			return CreateBuffer(device, data, size, cpuAccessFlag, D3D11_BIND_VERTEX_BUFFER);
		}

		std::shared_ptr<ID3D11Buffer> CreateIndexBuffer(ID3D11Device* device, void* data, size_t size, UINT cpuAccessFlag) {
			return CreateBuffer(device, data, size, cpuAccessFlag, D3D11_BIND_INDEX_BUFFER);
		}

		std::shared_ptr<ID3D11Buffer> CreateConstantBuffer(ID3D11Device* device, void* data, size_t size, UINT cpuAccessFlag) {
			return CreateBuffer(device, data, size, cpuAccessFlag, D3D11_BIND_CONSTANT_BUFFER);
		}
	}
}