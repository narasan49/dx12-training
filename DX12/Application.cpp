#include "stdafx.h"
#include "Application.h"
#include <tchar.h>
#include <Windows.h>

#include "DX12.h"
#include "PMDRenderer.h"
#include "PMDActor.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")

using namespace std;
using namespace DirectX;
using namespace Microsoft::WRL;

const unsigned int WINDOW_HEIGHT = 720;
const unsigned int WINDOW_WIDTH = 1280;

namespace
{
	LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		// ウィンドウが破棄されたら呼ばれる
		if (msg == WM_DESTROY)
		{
			PostQuitMessage(0);
			return 0;
		}
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
}

Application& Application::Instance() 
{
	static Application instance;
	return instance;
}

void Application::CreateGameWindow()
{
	mWindowClass = {};
	mWindowClass.cbSize = sizeof(WNDCLASSEX);
	mWindowClass.lpfnWndProc = (WNDPROC)WindowProcedure;
	mWindowClass.lpszClassName = _T("DX12Sample");
	mWindowClass.hInstance = GetModuleHandle(nullptr);
	RegisterClassEx(&mWindowClass);
	RECT wrc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
	// ウィンドウサイズ補正
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウオブジェクト生成
	mHwnd = CreateWindow(
		mWindowClass.lpszClassName,
		_T("DX12Test"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,
		nullptr,
		nullptr,
		mWindowClass.hInstance,
		nullptr
	);

}

bool Application::Init()
{
	CreateGameWindow();
	mDX12.reset(new DX12(mHwnd));
	mRenderer.reset(new PMDRenderer(*mDX12));

	
	mActors.resize(2);
	mActors[0].reset(new PMDActor("Model/初音ミク.pmd", *mDX12, "Motion/motion.vmd"));
	mActors[0]->PlayAnimation();
	mActors[1].reset(new PMDActor("Model/鏡音リン.pmd", *mDX12, "Motion/motion.vmd"));
	mActors[1]->PlayAnimation();
	Transform transform = {};
	transform.world = ::XMMatrixTranslation(10, 0, 10);
	mActors[1]->SetTransform(transform);
	return true;
}

void Application::Run()
{
	ShowWindow(mHwnd, SW_SHOW);
	MSG msg = {};
	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
		{
			break;
		}
		//mPMDActor->Update();
		for (auto& actor : mActors)
		{
			actor->Update();
		}
		mDX12->Update();

		// light depthへの描画
		mRenderer->SetShadowPipeLine();
		mDX12->DrawToLightDepth();
		//mPMDActor->Draw(true);
		for (auto& actor : mActors)
		{
			actor->Draw(true);
		}

		// ぺらポリゴンへの描画
		mDX12->BeginDrawPera();

		// ぺらポリゴンに描画してテクスチャにする
		mDX12->getCommandList()->SetPipelineState(mRenderer->getPipeline().Get());
		mDX12->getCommandList()->SetGraphicsRootSignature(mRenderer->getRootSignature().Get());
		mDX12->getCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		mDX12->Draw();
		//mPMDActor->Draw();
		for (auto& actor : mActors)
		{
			actor->Draw();
		}
		mDX12->EndDrawPera();

		// 縮小バッファ
		mDX12->DrawShrinkTextureForBlur();

		// バックバッファへの描画準備
		mDX12->BeginDraw();
		// 
		mDX12->DrawPera();
		mDX12->EndDraw();

		mDX12->getSwapChain()->Present(1, 0);
	}
}