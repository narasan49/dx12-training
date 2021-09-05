#pragma once
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl.h>

#include <vector>
#include <map>
//#include <iostream>

#include <DirectXMath.h>
#include <DirectXTex.h>
#include <d3dx12.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")

using namespace std;
using namespace DirectX;
using namespace Microsoft::WRL;

class DX12;
class PMDRenderer;
class PMDActor;

struct Size
{
	int Width;
	int Height;
	Size() {}
	Size(int width, int height)
		: Width(width), Height(height)
	{}
};

class Application
{
public:
	static Application& Instance();

	void CreateGameWindow();
	bool Init();
	void Run();
	//void Termiante();

	~Application() {}
	Size GetWindowSize();

private:
	Application() {}

	WNDCLASSEX mWindowClass;
	HWND mHwnd;
	shared_ptr<DX12> mDX12;
	shared_ptr<PMDRenderer> mRenderer;
	shared_ptr<PMDActor> mPMDActor;
	std::vector<shared_ptr<PMDActor>> mActors;
};

