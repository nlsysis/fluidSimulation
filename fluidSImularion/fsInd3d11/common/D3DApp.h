#pragma once

#include "d3dUtil.h"
#include "GameTimer.h"
#include <string>
#include "Mouse.h"
#include "Keyboard.h"
#include "ImguiManager.h"

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d3dx11d.lib")
#pragma comment(lib,"D3DCompiler.lib")
class D3DApp
{
public:
	D3DApp(HINSTANCE hInstance);
	virtual ~D3DApp();

	HINSTANCE GetAppInst()const;
	HWND      GetMainWnd()const;
	float     GetAspectRatio()const;   //the ratio of back buffer

	int Run();

	virtual bool Init();
	virtual void OnResize();
	virtual void UpdateScene(float dt) = 0;
	virtual void DrawScene() = 0;
	virtual void DrawScene(float dt) = 0;
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	//deal with mouse input
	virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
	virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

protected:
	bool InitMainWindow();
	bool InitDirect3D();

	void CalculateFrameStats();
protected:
	HINSTANCE mhAppInst;     // ?用程序?例句柄
	HWND      mhMainWnd;     // 主窗口句柄
	bool      mAppPaused;    // 程序是否?在?停状?
	bool      mMinimized;    // 程序是否最小化
	bool      mMaximized;    // 程序是否最大化
	bool      mResizing;     // 程序是否?在改?大小的状?
	UINT      m4xMsaaQuality;// 4X MSAA?量等?

	// 用于??"delta-time"和游???(§4.3)
	GameTimer mTimer;

	//  D3D11??(§4.2.1)，交??(§4.2.4)，用于深度/模板?存的2D?理(§4.2.6)，
	//  ?染目?(§4.2.5)和深度/模板??(§4.2.6)，和?口(§4.2.8)。
	ID3D11Device* md3dDevice;
	ID3D11DeviceContext* md3dDeviceContext;
	IDXGISwapChain* mSwapChain;
	ID3D11Texture2D* mDepthStencilBuffer;
	ID3D11RenderTargetView* mRenderTargetView;
	ID3D11DepthStencilView* mDepthStencilView;
	ID3D11DepthStencilState* m_depthStencilState;
	D3D11_VIEWPORT mScreenViewport;
	ID3D11RasterizerState* m_rasterState;
	XMMATRIX m_orthoMatrix;

	std::unique_ptr<DirectX::Mouse> m_pMouse;						// mouse
	DirectX::Mouse::ButtonStateTracker m_MouseTracker;				// mouse state tracker
	std::unique_ptr<DirectX::Keyboard> m_pKeyboard;					// keyboard
	DirectX::Keyboard::KeyboardStateTracker m_KeyboardTracker;		// keyboard state tracker

	//  下面的?量是在D3DApp?造函数中?置的。但是，?可以在派生?中重写?些?。

	//  窗口??。D3DApp的默???是"D3D11 Application"。
	std::wstring mMainWndCaption;

	//  Hardware device?是reference device？D3DApp默?使用D3D_DRIVER_TYPE_HARDWARE。
	D3D_DRIVER_TYPE md3dDriverType;
	// 窗口的初始大小。D3DApp默??800x600。注意，当窗口大小在?行?段改??，?些?也会随之改?。
	int mClientWidth;
	int mClientHeight;
	float mScreenNear;
	float mScreenDepth;
	//  ?置?true?使用4XMSAA(§4.1.8)，默??false。
	bool mEnable4xMsaa;

	ImguiManager imgui;

};