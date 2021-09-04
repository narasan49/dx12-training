#include "stdafx.h"
#include <Windows.h>
#include <tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#include <vector>
#include <map>

#include <DirectXMath.h>
#include <DirectXTex.h>
#include <d3dx12.h>

#include "Application.h"
#include "PMDActor.h"
#include "PMDRenderer.h"

#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")

using namespace std;
using namespace DirectX;

//コンソール画面にフォーマット付き文字列を表示
//デバッグ用
void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif // _DEBUG
}


#ifdef _DEBUG
int main()
{
	HRESULT result = S_OK;
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif // _DEBUG
	DebugOutputFormatString("Show window test.\n");

	auto& app = Application::Instance();
	if (!app.Init())
	{
		return -1;
	}
	app.Run();
	return 0;
}