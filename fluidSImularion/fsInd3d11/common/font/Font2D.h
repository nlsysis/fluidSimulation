#pragma once
#include "Textclass.h"
#include <xnamath.h>
namespace Font2D
{
	class FontPrint
	{
	public:
		static void InitFont(ID3D11Device* md3dDevice, HWND hWnd, int screenWidth, int screenHeight);
		static void DrawFont(ID3D11DeviceContext* md3dDeviceContext, XMMATRIX orthoMatrix);
		static void SetFont(ID3D11DeviceContext* md3dDeviceContext, const char*, float posX, float posY);
	private:
		static void TurnOnAlphaBlending(ID3D11DeviceContext* md3dDeviceContext);
		static void TurnOffAlphaBlending(ID3D11DeviceContext* md3dDeviceContext);

	};
}