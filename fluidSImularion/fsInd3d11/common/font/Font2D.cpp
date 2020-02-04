#include "Font2D.h"
#include <wrl/client.h>
#include <memory>

using namespace Microsoft::WRL;
using namespace Font2D;

std::unique_ptr<class TextClass>  m_Text;
XMMATRIX m_worldMatrix;
ComPtr <ID3D11BlendState> m_alphaEnableBlendingState = nullptr;
ComPtr <ID3D11BlendState> m_alphaDisableBlendingState = nullptr;
ComPtr <ID3D11DepthStencilState> m_depthStencilState = nullptr;
ComPtr <ID3D11DepthStencilState> m_depthDisabledStencilState = nullptr;
void FontPrint::InitFont(ID3D11Device* md3dDevice,HWND hWnd,int screenWidth, int screenHeight)
{
	XMMATRIX baseViewMatrix;
	FXMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f,1.0);
	FXMVECTOR position = XMVectorSet(0.0f, 0.0f, -1.0f,1.0f);
	FXMVECTOR lookAt = XMVectorSet(0.0f, 0.0f, 1.0f,1.0f);
	baseViewMatrix = XMMatrixLookAtLH(position, lookAt, up);
	
	m_Text = std::make_unique<TextClass>();
	m_Text->Initialize(md3dDevice,hWnd, screenWidth, screenHeight, baseViewMatrix);

	D3D11_BLEND_DESC blendStateDescription;
	ZeroMemory(&blendStateDescription, sizeof(D3D11_BLEND_DESC));
	// Create an alpha enabled blend state description.
	blendStateDescription.RenderTarget[0].BlendEnable = TRUE;
	blendStateDescription.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendStateDescription.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDescription.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDescription.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendStateDescription.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDescription.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDescription.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	md3dDevice->CreateBlendState(&blendStateDescription, &m_alphaEnableBlendingState);

	blendStateDescription.RenderTarget[0].BlendEnable = FALSE;
	md3dDevice->CreateBlendState(&blendStateDescription, &m_alphaDisableBlendingState);

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	// Initialize the description of the stencil state.
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	// Set up the description of the stencil state.
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the depth stencil state.
	md3dDevice->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilState);
	// Set up the description of the stencil state.
	depthStencilDesc.DepthEnable = false;
	md3dDevice->CreateDepthStencilState(&depthStencilDesc, &m_depthDisabledStencilState);

	m_worldMatrix = XMMatrixIdentity();
}
void FontPrint::SetFont(ID3D11DeviceContext* md3dDeviceContext,const char*text,float posX, float posY)
{
	m_worldMatrix = XMMatrixTranslation(posX, posY, 0.0f);
	m_Text->UpdateText(text,posX,posY,0.0f,0.0f,0.0f, md3dDeviceContext);
}
void FontPrint::DrawFont(ID3D11DeviceContext* md3dDeviceContext,XMMATRIX orthoMatrix)
{
	md3dDeviceContext->OMSetDepthStencilState(m_depthDisabledStencilState.Get(), 1);  //turnoff Z buffer
	TurnOnAlphaBlending(md3dDeviceContext);

	m_Text->Render(md3dDeviceContext, m_worldMatrix, orthoMatrix);

	TurnOffAlphaBlending(md3dDeviceContext);
	md3dDeviceContext->OMSetDepthStencilState(m_depthStencilState.Get(), 1);  //turnoff Z buffer
	
}

void FontPrint::TurnOnAlphaBlending(ID3D11DeviceContext* md3dDeviceContext)
{
	float blendFactor[4];
	// Setup the blend factor.
	blendFactor[0] = 0.0f;
	blendFactor[1] = 0.0f;
	blendFactor[2] = 0.0f;
	blendFactor[3] = 0.0f;

	md3dDeviceContext->OMSetBlendState(m_alphaEnableBlendingState.Get(), blendFactor, 0xffffffff);
}
void FontPrint::TurnOffAlphaBlending(ID3D11DeviceContext* md3dDeviceContext)
{
	float blendFactor[4];


	// Setup the blend factor.
	blendFactor[0] = 0.0f;
	blendFactor[1] = 0.0f;
	blendFactor[2] = 0.0f;
	blendFactor[3] = 0.0f;

	// Turn off the alpha blending.
	md3dDeviceContext->OMSetBlendState(m_alphaDisableBlendingState.Get(), blendFactor, 0xffffffff);
}