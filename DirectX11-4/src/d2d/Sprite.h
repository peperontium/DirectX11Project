#pragma once

#include "CanvasLayer2D.h"
#include <string>

namespace d2 {
	
	class SpriteBase {
	protected:
		std::shared_ptr<ID3D11ShaderResourceView>	_texture;
		D2D_MATRIX_3X2_F _transform;
		D2D_MATRIX_3X2_F _textureTransform;
		CanvasLayer2D* _targetCanvas;
		SpriteBase(std::shared_ptr<ID3D11ShaderResourceView> texture, CanvasLayer2D* canvas) :
			_texture(texture),
			_transform(D2D1::Matrix3x2F::Identity()),
			_textureTransform(D2D1::Matrix3x2F::Identity()),
			_targetCanvas(canvas){
		}

	public:
		~SpriteBase() {}
	};

	class Sprite : public SpriteBase{
	private:
		int _texWidth, _texHeight;

		void _ReadTextureSize();

		//	2Dでの変換行列から3Dの座標変換行列を取得
		D2D_MATRIX_3X2_F _GetTransformMatrix3D(const D2D_MATRIX_3X2_F& transform2D)const {

			float canvasWidth = _targetCanvas->getWidth()*1.0f;
			float canvasHeight = _targetCanvas->getHeight()*1.0f;

			D2D_MATRIX_3X2_F converted = transform2D;
			converted._11 *= _texWidth / canvasWidth;
			converted._12 *= _texHeight /canvasHeight;
			converted._21 *= _texWidth / canvasWidth;
			converted._22 *= _texHeight / canvasHeight;
			
			converted._31 = (converted._31/(canvasWidth / 2)) - 1;
			converted._32 = 1 - (converted._32 / (canvasHeight / 2));

			return converted;
		}

	public:
		Sprite(std::shared_ptr<ID3D11ShaderResourceView> texture, CanvasLayer2D* canvas) :
			SpriteBase(texture,canvas)
		{
			_ReadTextureSize();
		}
		~Sprite() {}
		void setTexture(std::shared_ptr<ID3D11ShaderResourceView> texture) {
			_texture = texture;
			_ReadTextureSize();
		}
		void render(ID3D11DeviceContext* context, CanvasLayer2D::BlendMode blendMode = CanvasLayer2D::BlendMode::Default)const{
			//	T /= screen -> S *= (tex/scr)
			D2D_MATRIX_3X2_F transform3D = _GetTransformMatrix3D(_transform);
			_targetCanvas->updateConstantBuffer(context, transform3D, _textureTransform);
			_targetCanvas->renderTexture(context, _texture.get(), blendMode);
		}
		void setTransform(const D2D_MATRIX_3X2_F& transform) {
			_transform = transform;
		}
		const D2D_MATRIX_3X2_F& getTransform() const{
			return _transform;
		}
		void setTextureTransform(const D2D_MATRIX_3X2_F& texTransform) {
			_textureTransform = texTransform;
		}
		const D2D_MATRIX_3X2_F& getTextureTransform() const {
			return _textureTransform;
		}
	};

	class TextSprite : public SpriteBase {
	private:
		//!	DX11とDX10.1の排他制御用Mutex
		std::shared_ptr<IDXGIKeyedMutex>	_keyedMutex10, _keyedMutex11;
		//!	Direct2D側のレンダーターゲット
		std::shared_ptr<ID2D1RenderTarget>	_renderTarget2D;
		
		mutable bool	  _usingTextureByD2D;

		std::shared_ptr<ID2D1SolidColorBrush>	_brush;
		std::shared_ptr<IDWriteTextFormat>		_format;

		void _BeginD2DRendering()const {
			// D3D 11 側からのテクスチャの使用を中断して
			_keyedMutex11->ReleaseSync(0);
			// D3D 10.1 (D2D) 側からのテクスチャの使用を開始
			// 待機時間は無限としています。
			_keyedMutex10->AcquireSync(0, INFINITE);

			// 描画開始
			_renderTarget2D->BeginDraw();
			_renderTarget2D->Clear(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.0f));
		}
		void _EndD2DRendering()const{

			// 描画終了
			_renderTarget2D->EndDraw();

			// D3D 10.1 (D2D) 側からのテクスチャの使用を終了して
			_keyedMutex10->ReleaseSync(1);
			// D3D 11 側からのテクスチャの使用を開始する
			_keyedMutex11->AcquireSync(1, INFINITE);
		}
		
	public:
		TextSprite(CanvasLayer2D* canvas,
			const wchar_t* fontName = L"メイリオ", float fontSize = 20.0f, D2D_COLOR_F color = D2D1::ColorF(D2D1::ColorF::White));
		void setColor(D2D_COLOR_F color) {
			_brush->SetColor(color);
		}
		void setFormat(std::shared_ptr<IDWriteTextFormat>& format) {
			_format = format;
		}
		void drawText(const wchar_t* text, int length, const D2D1_RECT_F& drawRect)const{
			if (! _usingTextureByD2D) {
				_BeginD2DRendering();
				_usingTextureByD2D = true;
			}
			_renderTarget2D->DrawTextW(text, length, _format.get(), drawRect, _brush.get());
		}
		void drawText(const std::wstring& text, const D2D1_RECT_F& drawRect)const  {
			this->drawText(text.c_str(), text.length(),drawRect);
		}
		void render(ID3D11DeviceContext* context)const{
			if (_usingTextureByD2D) {
				_EndD2DRendering();
				_usingTextureByD2D = false;
			}
			_targetCanvas->updateConstantBuffer(context,_transform, _textureTransform);
			_targetCanvas->renderTexture(context, _texture.get(), CanvasLayer2D::BlendMode::PreMultiPlyedAlpha);
		}
	};

}