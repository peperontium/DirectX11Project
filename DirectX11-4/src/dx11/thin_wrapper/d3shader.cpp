#include "../DX11ThinWrapper.h"
#include <stdexcept>

#pragma comment(lib, "d3dcompiler.lib")

namespace {
	template<typename S> std::shared_ptr<S> CreateShader(
		ID3D11Device* device, const byte* shader, size_t size,
		HRESULT(__stdcall ID3D11Device::*func)(const void *, SIZE_T, ID3D11ClassLinkage *, S **)
		) {
		S * shaderObject = nullptr;
		auto hr = (device->*func)(shader, size, nullptr, &shaderObject);
		if (FAILED(hr)) throw std::runtime_error("シェーダーの生成に失敗しました.");
		return std::shared_ptr<S>(shaderObject, comUtil::ReleaseIUnknown);
	}
}

namespace DX11ThinWrapper {
	namespace d3 {
		std::shared_ptr<ID3DBlob> CompileShader(TCHAR source[], CHAR entryPoint[], CHAR target[]) {
			// 行列を列優先で設定し、古い形式の記述を許可しないようにする
			UINT flag1 = D3D10_SHADER_PACK_MATRIX_COLUMN_MAJOR | D3D10_SHADER_ENABLE_STRICTNESS;

			// 最適化レベルを設定する
			#if defined(DEBUG) || defined(_DEBUG)
				flag1 |= D3D10_SHADER_OPTIMIZATION_LEVEL0;
			#else
				flag1 |= D3D10_SHADER_OPTIMIZATION_LEVEL3;
			#endif

			ID3DBlob* blob = nullptr;
			ID3DBlob* pErr = nullptr;
			auto hr = ::D3DCompileFromFile(source, nullptr, nullptr, entryPoint, target, flag1, 0, &blob, &pErr);
			if (FAILED(hr)) {
				if (pErr != nullptr && pErr->GetBufferPointer() != nullptr) {
					//	hlslファイルのコンパイルエラーを出力
					OutputDebugStringA(reinterpret_cast<CHAR *>(pErr->GetBufferPointer()));
					pErr->Release();
				}
				if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
					OutputDebugString(source);
					OutputDebugString(TEXT(" : hlsl/fx file not found\n"));
				}
				throw std::runtime_error("シェーダーファイルのコンパイルに失敗しました.");
			}
			return std::shared_ptr<ID3DBlob>(blob, comUtil::ReleaseIUnknown);
		}

		std::shared_ptr<ID3D11PixelShader> CreatePixelShader(ID3D11Device* device, const byte* shaderBytes, size_t byteSize) {
			return CreateShader(
				device, shaderBytes, byteSize, &ID3D11Device::CreatePixelShader
			);
		}
		std::shared_ptr<ID3D11VertexShader> CreateVertexShader(ID3D11Device* device, const byte* shaderBytes, size_t byteSize) {
			return CreateShader(
				device, shaderBytes, byteSize, &ID3D11Device::CreateVertexShader
			);
		}
		std::shared_ptr<ID3D11GeometryShader> CreateGeometryShader(ID3D11Device* device, const byte* shaderBytes, size_t byteSize) {
			return CreateShader(
				device, shaderBytes, byteSize, &ID3D11Device::CreateGeometryShader
			);
		}
		std::shared_ptr<ID3D11InputLayout> CreateInputLayout(
			ID3D11Device* device, D3D11_INPUT_ELEMENT_DESC layoutDesc[], UINT numOfLayout, const byte* shaderBytes, size_t byteSize
		) {
			ID3D11InputLayout* layout;
			auto hr = device->CreateInputLayout(
				layoutDesc, numOfLayout, shaderBytes, byteSize, &layout
			);
			if (FAILED(hr)) throw std::runtime_error("入力レイアウトの生成に失敗しました.");
			return std::shared_ptr<ID3D11InputLayout>(layout, comUtil::ReleaseIUnknown);
		}
	}
}