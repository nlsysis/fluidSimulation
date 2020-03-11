#include "d3dApp.h"
#include <WindowsX.h>
#include <sstream>
#include <memory>
#include <vector>
#include "font/Font2D.h"
#include "..\imgui\imgui_impl_win32.h"
#include "..\imgui\imgui_impl_dx11.h"

/**
	*@brief  This is just used to forward Windows messages from a global window
	 procedure to our member function window procedure because we cannot
	assign a member function to WNDCLASS::lpfnWndProc.
*/
namespace
{ 
	D3DApp* gd3dApp = nullptr;
}

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	/// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	/// before CreateWindow returns, and thus before mhMainWnd is valid.
	return gd3dApp->MsgProc(hwnd, msg, wParam, lParam);
}

/**
	*@brief  Initial the paraments of directx and window.
	*@param[in] hInstance the instance of this windows application.
*/
D3DApp::D3DApp(HINSTANCE hInstance)
	: mhAppInst(hInstance),
	mMainWndCaption(L"D3D11 Application"),
	md3dDriverType(D3D_DRIVER_TYPE_HARDWARE),
	mClientWidth(800),
	mClientHeight(600),
	mEnable4xMsaa(false),
	mhMainWnd(0),
	mAppPaused(false),
	mMinimized(false),
	mMaximized(false),
	mResizing(false),
	m4xMsaaQuality(0),

	md3dDevice(0),
	md3dDeviceContext(0),
	mSwapChain(0),
	mDepthStencilBuffer(0),
	mRenderTargetView(0),
	mDepthStencilView(0),
	m_rasterState(0),
	mScreenNear(1.0f),
	mScreenDepth(1000.0f)
{
	ZeroMemory(&mScreenViewport, sizeof(D3D11_VIEWPORT));
	gd3dApp = this;
}

/**
	*@brief  Release the resources of this application.
*/
D3DApp::~D3DApp()
{
	ReleaseCOM(mRenderTargetView);
	ReleaseCOM(mDepthStencilView);
	ReleaseCOM(mSwapChain);
	ReleaseCOM(mDepthStencilBuffer);

	/// Restore all default settings.
	if (md3dDeviceContext)
		md3dDeviceContext->ClearState();

	ReleaseCOM(md3dDeviceContext);
	ReleaseCOM(md3dDevice);

	ImGui_ImplWin32_Shutdown();
}

/**
	*@brief  Get the instance of this applicatin.
	*@return[HINSTANCE] The instance of this application.
*/
HINSTANCE D3DApp::GetAppInst()const
{
	return mhAppInst;
}

/**
	*@brief  Get the handle of this applicatin.
	*@return[HWND] The handle of this application.
*/
HWND D3DApp::GetMainWnd()const
{
	return mhMainWnd;
}

/**
	*@brief  Get the aspect ratio of the window.
	*@return[float] The aspect ratio of the window.
*/
float D3DApp::GetAspectRatio()const
{
	return static_cast<float>(mClientWidth) / mClientHeight;
}

/**
	*@brief  The main part of the application.
	*@return[int] The windows message parameters.
*/
int D3DApp::Run()
{
	MSG msg = { 0 };

	mTimer.Reset();

	while (msg.message != WM_QUIT)
	{
		/// If there are Window messages then process them.
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		/// Otherwise, do animation/game stuff.
		else
		{
			mTimer.Tick();

			if (!mAppPaused)
			{
				CalculateFrameStats();
				UpdateScene(mTimer.DeltaTime());
				DrawScene();
				DrawScene(mTimer.DeltaTime());
			}
			else
			{
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}

/**
	*@brief  Initial the devices of mouse and keyboard and window and directx.
	*@return[bool] If the initial is succeed.
*/
bool D3DApp::Init()
{
	m_pMouse = std::make_unique<DirectX::Mouse>();
	m_pKeyboard = std::make_unique<DirectX::Keyboard>();

	if (!InitMainWindow())
		return false;

	if (!InitDirect3D())
		return false;

	Font2D::FontPrint::InitFont(md3dDevice,mhMainWnd,mClientWidth,mClientHeight);
	return true;
}

/**
	*@brief  Change the size of windows and reset some parameters. 
*/
void D3DApp::OnResize()
{
	assert(md3dDeviceContext);
	assert(md3dDevice);
	assert(mSwapChain);

	/// Release the old views, as they hold references to the buffers we
	/// will be destroying.  Also release the old depth/stencil buffer.
	ReleaseCOM(mRenderTargetView);
	ReleaseCOM(mDepthStencilView);
	ReleaseCOM(mDepthStencilBuffer);

	/// Resize the swap chain and recreate the render target view.
	HR(mSwapChain->ResizeBuffers(1, mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
	ID3D11Texture2D* backBuffer;
	HR(mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)));
	HR(md3dDevice->CreateRenderTargetView(backBuffer, 0, &mRenderTargetView));
	ReleaseCOM(backBuffer);

	/// Create the depth/stencil buffer and view.

	D3D11_TEXTURE2D_DESC depthStencilDesc;

	depthStencilDesc.Width = mClientWidth;
	depthStencilDesc.Height = mClientHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
//	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT ;
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;    //this type will make depth buffer editable.

	/// Use 4X MSAA? --must match swap chain MSAA values.
	if (mEnable4xMsaa)
	{
		depthStencilDesc.SampleDesc.Count = 4;
		depthStencilDesc.SampleDesc.Quality = m4xMsaaQuality - 1;
	}
	/// No MSAA
	else
	{
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
	}

	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	HR(md3dDevice->CreateTexture2D(&depthStencilDesc, 0, &mDepthStencilBuffer));
	
	D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc;
	ZeroMemory(&DSVDesc, sizeof(DSVDesc));
	DSVDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	DSVDesc.Texture2D.MipSlice = 0;
	HR(md3dDevice->CreateDepthStencilView(mDepthStencilBuffer, &DSVDesc, &mDepthStencilView));

	D3D11_SHADER_RESOURCE_VIEW_DESC depthStencilSRViewDesc;
	memset(&depthStencilSRViewDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	depthStencilSRViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	depthStencilSRViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	depthStencilSRViewDesc.Texture2D.MipLevels = 1;

	HR(md3dDevice->CreateShaderResourceView(mDepthStencilBuffer, &depthStencilSRViewDesc, &mDepthStencilSRView));
	/// Bind the render target view and depth/stencil view to the pipeline.

	md3dDeviceContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);


	/// Set the viewport transform.

	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	md3dDeviceContext->RSSetViewports(1, &mScreenViewport);

	///init imgui d3d impl
	ImGui_ImplDX11_Init(md3dDevice, md3dDeviceContext);
	
}

/**
	*@brief  Handle the massages of the applicaion.
	*@return[LRESULT] 
*/
LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
	{
		return true;
	}
	switch (msg)
	{
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			mAppPaused = true;
			mTimer.Stop();
		}
		else
		{
			mAppPaused = false;
			mTimer.Start();
		}
		return 0;
	case WM_SIZE:
		/// Save the new client area dimensions.
		mClientWidth = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);
		if (md3dDevice)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{

				/// Restoring from minimized state?
				if (mMinimized)
				{
					mAppPaused = false;
					mMinimized = false;
					OnResize();
				}

				/// Restoring from maximized state?
				else if (mMaximized)
				{
					mAppPaused = false;
					mMaximized = false;
					OnResize();
				}
				else if (mResizing)
				{
				}
				else /// API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;
		/// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResizing = true;
		mTimer.Stop();
		return 0;

		/// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		/// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing = false;
		mTimer.Start();
		OnResize();
		return 0;

		/// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
		/// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_MENUCHAR:
	/// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

	/// monitor the mouse event.
	case WM_INPUT:

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_XBUTTONDOWN:

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_XBUTTONUP:

	case WM_MOUSEWHEEL:
	case WM_MOUSEHOVER:
	case WM_MOUSEMOVE:
		m_pMouse->ProcessMessage(msg, wParam, lParam);
		return 0;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		m_pKeyboard->ProcessMessage(msg, wParam, lParam);
		return 0;

	case WM_ACTIVATEAPP:
		m_pMouse->ProcessMessage(msg, wParam, lParam);
		m_pKeyboard->ProcessMessage(msg, wParam, lParam);
		return 0;
	}

	
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
/**
	*@brief Init the window of App
	*@return[bool] If the initial succeed.
*/
bool D3DApp::InitMainWindow()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = mhAppInst;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"D3DWndClassName";

	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	/// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, mClientWidth, mClientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	mhMainWnd = CreateWindow(L"D3DWndClassName", mMainWndCaption.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);
	if (!mhMainWnd)
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(mhMainWnd, SW_SHOW);

	///Init ImGui Win32 Impl
	ImGui_ImplWin32_Init(mhMainWnd);

	UpdateWindow(mhMainWnd);

	return true;
}
/**
	*@brief Init the directX11 devices
*/
bool D3DApp::InitDirect3D()
{
	/// Create the device and device context.

	UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;  ///< directx2D need BGRA support
#if defined(DEBUG) || defined(_DEBUG)  
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	IDXGIFactory* pFactory;
	IDXGIAdapter* pAdapter;             ///<　for see about the equipment ability
	IDXGIOutput* adapterOutput;
	std::vector <IDXGIAdapter*> vAdapters;
	HR(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory));
	for (UINT i = 0;
		pFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND;
		++i)
	{
		vAdapters.push_back(pAdapter);
	}
	
	D3D_FEATURE_LEVEL featureLevel;
	HRESULT hr = D3D11CreateDevice(
		0,                 ///< default adapter
		md3dDriverType,
		0,                 ///< no software device
		createDeviceFlags,
		0, 0,              ///< default feature level array
		D3D11_SDK_VERSION,
		&md3dDevice,
		&featureLevel,
		&md3dDeviceContext);

	if (FAILED(hr))
	{
		MessageBox(0, L"D3D11CreateDevice Failed.", 0, 0);
		return false;
	}

	if (featureLevel != D3D_FEATURE_LEVEL_11_0)
	{
		MessageBox(0, L"Direct3D Feature Level 11 unsupported.", 0, 0);
		return false;
	}
	D3D11SetDebugObjectName(md3dDeviceContext, "DeviceContext");
	/** 
		Check 4X MSAA quality support for our back buffer format.
	 All Direct3D 11 capable devices support 4X MSAA for all render 
	 target formats, so we only need to check quality support.
	*/

	HR(md3dDevice->CheckMultisampleQualityLevels(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, &m4xMsaaQuality));
	assert(m4xMsaaQuality > 0);

	/// Fill out a DXGI_SWAP_CHAIN_DESC to describe our swap chain.

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = mClientWidth;
	sd.BufferDesc.Height = mClientHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	/// Use 4X MSAA? 
	if (mEnable4xMsaa)
	{
		sd.SampleDesc.Count = 4;
		sd.SampleDesc.Quality = m4xMsaaQuality - 1;
	}
	/// No MSAA
	else
	{
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
	}

	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 1;
	sd.OutputWindow = mhMainWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = 0;

	/** 
		To correctly create the swap chain, we must use the IDXGIFactory that was
	 used to create the device.  If we tried to use a different IDXGIFactory instance
	 (by calling CreateDXGIFactory), we get an error: "IDXGIFactory::CreateSwapChain: 
	 This function is being called with a device from a different IDXGIFactory."
	*/

	IDXGIDevice* dxgiDevice = 0;
	HR(md3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice));

	IDXGIAdapter* dxgiAdapter = 0;
	HR(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter));

	IDXGIFactory* dxgiFactory = 0;
	HR(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory));

	HR(dxgiFactory->CreateSwapChain(md3dDevice, &sd, &mSwapChain));

	ReleaseCOM(dxgiDevice);
	ReleaseCOM(dxgiAdapter);
	ReleaseCOM(dxgiFactory);

	/**
		The remaining steps that need to be carried out for d3d creation
	 also need to be executed every time the window is resized.  So
	 just call the OnResize method here to avoid code duplication.
	*/
	OnResize();
	m_orthoMatrix = XMMatrixOrthographicLH((float)mClientWidth, (float)mClientHeight, 0.1f, mScreenDepth);
	return true;
}
/**
	*@brief calculate the FPS and frame time(perframe/second) and print in the window caption bar.
*/
void D3DApp::CalculateFrameStats()
{
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	/// Compute averages over one second period.
	if ((mTimer.TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		std::wostringstream outs;
		outs.precision(6);
		outs << mMainWndCaption << L"    "
			<< L"FPS: " << fps << L"    "
			<< L"Frame Time: " << mspf << L" (ms)";
		SetWindowText(mhMainWnd, outs.str().c_str());

		/// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}