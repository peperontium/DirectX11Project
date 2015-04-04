#include "stdafx.h"

#pragma comment(lib, "winmm.lib")
#include <mmsystem.h>

#include "./d2d/Sprite.h"

int APIENTRY _tWinMain(
	HINSTANCE hInstance,
	HINSTANCE,
	LPTSTR,
	INT
) {
	::CoInitialize(NULL); // なくても動くけど終了時例外を吐く
	
	// ウィンドウ生成
	DXGI_MODE_DESC modeDesc = DX11ThinWrapper::gi::GetOptDisplayMode(800, 600);
	win::Window window(
		TEXT("window"), TEXT("window"), {0, 0, modeDesc.Width, modeDesc.Height}, WS_OVERLAPPEDWINDOW, win::DefaultProcedure
	);
	
	MSG msg = {};

	{
		// 初期化
		dx11::DX11DeviceSharedGuard guard;
		auto device = dx11::AccessDX11Device();

		auto context = DX11ThinWrapper::d3::AccessD3Context(device);
		auto swapChain = dx11::CreateDefaultSwapChain(&modeDesc, window.getHWnd(), device, true);
		
		dx11::SetDefaultRenderTarget(swapChain.get());
		dx11::SetDefaultViewport(swapChain.get());


		//	シェーダー作成
		auto vertex_blob = DX11ThinWrapper::d3::CompileShader(_T("./assets/shader/sample.hlsl"), "RenderVS", VERTEX_SHADER_PROFILE);
		auto vertexShader = DX11ThinWrapper::d3::CreateVertexShader(device, vertex_blob.get());
		
		//	スキニング頂点シェーダ―
		auto skinningVS_blob = DX11ThinWrapper::d3::CompileShader(_T("./assets/shader/skinning.hlsl"), "RenderVS", VERTEX_SHADER_PROFILE);
		auto skinningVS = DX11ThinWrapper::d3::CreateVertexShader(device, skinningVS_blob.get());

		//	ピクセルシェーダは共通のものを使うよ
		auto pixelShader = DX11ThinWrapper::d3::CreatePixelShader(
			device, DX11ThinWrapper::d3::CompileShader(_T("./assets/shader/sample.hlsl"), "RenderPS", PIXEL_SHADER_PROFILE).get()
		);

		cmo::SkinnedMesh skinnedMesh;
		skinnedMesh.load(device, L"./assets/model/skinning.cmo", nullptr);
		skinnedMesh.applyPS(pixelShader);
		skinnedMesh.applyVS(skinningVS, skinningVS_blob,device);
		skinnedMesh.setAnimation(L"walk");


		DirectX::XMFLOAT4X4 param;

		//	プロジェクション行列設定
		//	定数バッファは float*4の倍数のサイズである必要がある
		//	その他はまりそうなところ ： https://twitter.com/43x2/status/144821841977028608

		float aspectRatio = modeDesc.Width / static_cast<float>(modeDesc.Height);
		DirectX::XMMATRIX mtxProj = DirectX::XMMatrixPerspectiveFovLH(3.1415926f / 6.0f, aspectRatio, 1.0f, 100.0f);
		XMStoreFloat4x4(&param, DirectX::XMMatrixTranspose(mtxProj));
		auto projBuffer = DX11ThinWrapper::d3::CreateConstantBuffer(
			device, &param, sizeof(DirectX::XMFLOAT4X4), D3D11_CPU_ACCESS_WRITE
			);
		ID3D11Buffer * projectionBuffers[] = { projBuffer.get() };
		//	スロット0にプロジェクション行列設定
		context->VSSetConstantBuffers(0, 1, projectionBuffers);
		


		// カメラ行列設定
		//	バッファサイズ等に関してはプロジェクションの場合と同様	
		DirectX::XMMATRIX mtxView = DirectX::XMMatrixTranspose(
			DirectX::XMMatrixLookAtLH(
			DirectX::XMVectorSet(0.0f, 0.0f, -5.0f, 1),
			DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1),
			DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1)
			)
		);
		XMStoreFloat4x4(&param, DirectX::XMMatrixTranspose(mtxView));
		auto cameraBuffer = DX11ThinWrapper::d3::CreateConstantBuffer(
			device, &param, sizeof(DirectX::XMFLOAT4X4), D3D11_CPU_ACCESS_WRITE
			);




		// ターゲットビュー
		auto rv = DX11ThinWrapper::d3::AccessRenderTargetViews(context.get(), 1)[0];
		auto dv = DX11ThinWrapper::d3::AccessDepthStencilView(context.get());

		// ウィンドウ表示
		::ShowWindow(window.getHWnd(), SW_SHOW);
		::UpdateWindow(window.getHWnd());


		// サンプラー設定忘れてたので
		D3D11_SAMPLER_DESC samplerDesc;
		samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;		// サンプリング時に使用するフィルタ。ここでは異方性フィルターを使用する。
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;	// 0 ～ 1 の範囲外にある u テクスチャー座標の描画方法
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;	// 0 ～ 1 の範囲外にある v テクスチャー座標
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;	// 0 ～ 1 の範囲外にある w テクスチャー座標
		samplerDesc.MipLODBias = 0;			// 計算されたミップマップ レベルからのバイアス
		samplerDesc.MaxAnisotropy = 16;		// サンプリングに異方性補間を使用している場合の限界値。有効な値は 1 ～ 16
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;  // 比較オプション
		samplerDesc.BorderColor[0] = 0.0f;
		samplerDesc.BorderColor[1] = 0.0f;
		samplerDesc.BorderColor[2] = 0.0f;
		samplerDesc.BorderColor[3] = 0.0f;
		samplerDesc.MinLOD = 0;						// アクセス可能なミップマップの下限値
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;		// アクセス可能なミップマップの上限値
		auto sampler = DX11ThinWrapper::d3::CreateSampler(device, samplerDesc);
		ID3D11SamplerState* states[] = { sampler.get() };
		context->PSSetSamplers(0, 1, states);


		//--------------------------
		//	DirectX10.1デバイスを11のものと同じ設定で作成
		//--------------------------
		dx10::DX10DeviceSharedGuard deviceGuard10(device);
		d2::Canvas2D canvas2D;

		canvas2D.init(device, modeDesc.Width, modeDesc.Height);
		auto txtes = DX11ThinWrapper::d3::CreateWICTextureFromFile(device,L"./assets/model/body.png");
		d2::Sprite sprite(txtes, &canvas2D);
		d2::TextSprite textspr(&canvas2D,L"メイリオ",50.0f);

		int curTime = 0, prevTime = timeGetTime();
		const int Frame_Per_Second = 60;
		int timer = 0;
		
		// メインループ
		do {
			if (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			} else {

				curTime = timeGetTime();
				if (curTime - prevTime >= 1000 / Frame_Per_Second) {
					prevTime = curTime;
					timer++;

					// バックバッファおよび深度バッファをクリア
					static float ClearColor[4] = { 0.3f, 0.3f, 1.0f, 1.0f };
					context->ClearRenderTargetView(rv.get(), ClearColor);
					context->ClearDepthStencilView(dv.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

					// カメラの設定
					DX11ThinWrapper::d3::mapping(cameraBuffer.get(), context.get(), [&](D3D11_MAPPED_SUBRESOURCE resource) {
						auto param = static_cast<DirectX::XMFLOAT4X4 *>(resource.pData);
						auto mtxView = DirectX::XMMatrixLookAtLH(
							DirectX::XMVectorSet(0.0f, 2.0f, -10.0f, 1),
							DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1),
							DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1)
							);
						XMStoreFloat4x4(param, DirectX::XMMatrixTranspose(mtxView));
					});
					ID3D11Buffer * cameraBuffers[] = { cameraBuffer.get() };
					//	スロット1にカメラ行列を設定
					context->VSSetConstantBuffers(1, 1, cameraBuffers);


					//	スロット0にプロジェクション行列設定
					context->VSSetConstantBuffers(0, 1, projectionBuffers);


					//	スキンメッシュモデルの行列更新、描画
					skinnedMesh.setAnimationFrame(timer % 60);
					skinnedMesh.updateMatrix(
						DirectX::XMMatrixRotationY(DirectX::XM_PI/3.5)*DirectX::XMMatrixTranslation(0, -1, 0.5)
						);
					skinnedMesh.render(context.get(), 2);

					
					canvas2D.beginDraw(context.get());
					
					if (timer > 120) {
						sprite.setTransform(D2D1::Matrix3x2F::Translation(0,0));
						sprite.render(context.get());
					}
					wchar_t text[100];
					wsprintf(text, L"timer = %d", timer);
					textspr.drawText(text,lstrlenW(text),D2D1::RectF(0,400,600,800));
					textspr.render(context.get());

					canvas2D.endDraw(context.get());
					/**/
					if (FAILED(swapChain->Present(0, 0))) break;
				}
			}

		} while (msg.message != WM_QUIT);
	}

	::CoUninitialize(); // なくても動くけど終了時例外を吐く
	return msg.wParam;
}