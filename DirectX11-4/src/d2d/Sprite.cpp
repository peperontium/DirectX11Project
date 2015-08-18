#include "Sprite.h"

#include "DX10Wrapper1.h"
#include "D2DWrapper.h"
#include "../dx11/DX11GlobalDevice.h"
#include "../dx11/DX11ThinWrapper.h"

namespace d2d {

	//	設計気持ち悪いけれど他に手段ないのかなぁ
	void Sprite::_ReadTextureSize() {

			_texWidth = 0;
			_texHeight = 0;

			auto resource = DX11ThinWrapper::d3::AccessResource(_texture.get());
			if (resource == nullptr)
				return;

			D3D11_RESOURCE_DIMENSION type;
			resource->GetType(&type);

			if (type != D3D11_RESOURCE_DIMENSION::D3D11_RESOURCE_DIMENSION_TEXTURE2D)
				return;

			ID3D11Texture2D* tex2D = reinterpret_cast<ID3D11Texture2D*>(resource.get());
			D3D11_TEXTURE2D_DESC desc;
			tex2D->GetDesc(&desc);

			_texWidth = desc.Width;
			_texHeight = desc.Height;
	}



	TextSprite::TextSprite(CanvasLayer2D* canvas2d, const wchar_t* fontName, float fontSize, D2D_COLOR_F color) :
		SpriteBase(nullptr),
		_usingTextureByD2D(false) {

		// 作成するテクスチャ情報の設定。
		// ・DXGI_FORMAT_B8G8R8A8_UNORM は固定。
		// ・D3D11_BIND_RENDER_TARGET は D2D での描画対象とするために必須。
		// ・D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX はテクスチャを共有するのに必須。
		// ・非ミップマップテクスチャのみ共有可能なため、MipLevels = 1は確定。
		D3D11_TEXTURE2D_DESC canvasDesc = {};
		canvasDesc.Width = canvas2d->getWidth();
		canvasDesc.Height = canvas2d->getHeight();
		canvasDesc.MipLevels = 1;
		canvasDesc.ArraySize = 1;
		canvasDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		canvasDesc.SampleDesc.Count = 1;
		canvasDesc.Usage = D3D11_USAGE_DEFAULT;
		canvasDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		canvasDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;


		ID3D11Device* device = dx11::AccessDX11Device();

		auto tex2D = DX11ThinWrapper::d3::CreateTexture2D(device, canvasDesc);

		//	共有のためのD3D11のキーミューテックスを取得
		_keyedMutex11 = comUtil::QueryInterface<IDXGIKeyedMutex>(tex2D.get());


		//	DX10.1側で使う共有サーフェイスを作成
		auto surface10 = dx10::OpenSharedResource<IDXGISurface1>(tex2D.get());


		//	共有のためのD3D10.1のキーミューテックスを取得
		_keyedMutex10 = comUtil::QueryInterface<IDXGIKeyedMutex>(surface10.get());

		// D2D のレンダーターゲットを D3D 10.1 の共有サーフェイスから生成
		_renderTarget2D = d2d::CreateDXGISurfaceRenderTarget(
			surface10.get(), D2D1::RenderTargetProperties(
			D2D1_RENDER_TARGET_TYPE_HARDWARE, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED))
			);


		//	共有テクスチャを基にシェーダーリソースビューの作成
		_texture = DX11ThinWrapper::d3::CreateShaderResourceView(device, tex2D.get());

		//	描画位置設定
		_transform._31 = -1.0f, _transform._32 = 1.0f;

		//	デフォルト設定
		_format = d2d::CreateTextFormat(fontName,fontSize);
		_brush  = d2d::CreateSolidColorBrush(_renderTarget2D.get(),color);
	}
}
