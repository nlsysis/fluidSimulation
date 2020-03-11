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

	
	///	deal with mouse input
	
	virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
	virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

	enum class CameraMode { FirstPerson, ThirdPerson, Free };
protected:
	bool InitMainWindow();
	bool InitDirect3D();

	void CalculateFrameStats();
protected:
	HINSTANCE mhAppInst;     // application instance
	HWND      mhMainWnd;     // main window handle
	bool      mAppPaused;    // whether the application stop
	bool      mMinimized;    // whether set the application minized
	bool      mMaximized;    // whether set the application max
	bool      mResizing;     // whether the application is changing the size
	UINT      m4xMsaaQuality;// 4X MSAA quality

	
	GameTimer mTimer;  //calculate the gameTime


	ID3D11Device* md3dDevice;
	ID3D11DeviceContext* md3dDeviceContext;
	IDXGISwapChain* mSwapChain;
	ID3D11Texture2D* mDepthStencilBuffer;
	ID3D11RenderTargetView* mRenderTargetView;
	ID3D11DepthStencilView* mDepthStencilView;
	ID3D11DepthStencilState* m_depthStencilState;
	ID3D11ShaderResourceView* mDepthStencilSRView;
	D3D11_VIEWPORT mScreenViewport;
	ID3D11RasterizerState* m_rasterState;
	XMMATRIX m_orthoMatrix;

	std::unique_ptr<DirectX::Mouse> m_pMouse;						// mouse
	DirectX::Mouse::ButtonStateTracker m_MouseTracker;				// mouse state tracker
	std::unique_ptr<DirectX::Keyboard> m_pKeyboard;					// keyboard
	DirectX::Keyboard::KeyboardStateTracker m_KeyboardTracker;		// keyboard state tracker

	
	///The parameters can be reset or overwrited

	std::wstring mMainWndCaption;

	
	D3D_DRIVER_TYPE md3dDriverType; //  Hardware device in D3DApp default setting is D3D_DRIVER_TYPE_HARDWARE。
 
	///	The initial size of window. 
	///	D3DApp the default size is `800x600`.
	
	int mClientWidth;
	int mClientHeight;
	float mScreenNear;
	float mScreenDepth;
	
	///if use 4XMSAA，define false.
	
	bool mEnable4xMsaa;

	ImguiManager imgui;

};