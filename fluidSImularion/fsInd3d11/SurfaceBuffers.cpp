#include "SurfaceBuffers.h"

SurfaceBuffers::SurfaceBuffers()
{
	for (UINT i = 0; i < SurfaceBuffersIndex::Count; i++)
	{
		mRenderTargetTextureArray[i] = nullptr;
		mRenderTargetViewArray[i] = nullptr;
		mShaderResourceViewArray[i] = nullptr;
	}
}

SurfaceBuffers::~SurfaceBuffers()
{
	Shutdown();
}

//init the texture as the setting size
bool SurfaceBuffers::Init(ID3D11Device * device, UINT width, UINT height)
{
	HRESULT hr = S_OK;

	D3D11_TEXTURE2D_DESC textureDesc;
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;

	DXGI_FORMAT formats[SurfaceBuffersIndex::Count];
	formats[SurfaceBuffersIndex::Diffuse] = DXGI_FORMAT_R8G8B8A8_UNORM;     //for the color- target0 in PSFluidRenderSphere 
	formats[SurfaceBuffersIndex::Normal] = DXGI_FORMAT_R16G16B16A16_FLOAT;  //for the normal- target1 in PSFluidRenderSphere

	//setup render target texture description
	ZeroMemory(&textureDesc, sizeof(D3D11_TEXTURE2D_DESC));
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	//create the render target texture
	for (UINT i = 0; i < SurfaceBuffersIndex::Count; i++)
	{
		textureDesc.Format = formats[i];
		hr = device->CreateTexture2D(&textureDesc, NULL, &mRenderTargetTextureArray[i]);
		D3D11SetDebugObjectName(mRenderTargetTextureArray[i], "DeferredTTA");
		if (FAILED(hr))
			return false;
	}

	//renderTargetView description
	ZeroMemory(&renderTargetViewDesc, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	//create renderTargetView
	for (UINT i = 0; i < SurfaceBuffersIndex::Count; i++)
	{
		renderTargetViewDesc.Format = formats[i];
		hr = device->CreateRenderTargetView(mRenderTargetTextureArray[i], &renderTargetViewDesc, &mRenderTargetViewArray[i]);
		D3D11SetDebugObjectName(mRenderTargetViewArray[i], "DeferredRTVA");
		if (FAILED(hr))
			return false;
	}
	
	//shaderResourceView description
	ZeroMemory(&shaderResourceViewDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	for (UINT i = 0; i < SurfaceBuffersIndex::Count; i++)
	{
		shaderResourceViewDesc.Format = formats[i];
		hr = device->CreateShaderResourceView(mRenderTargetTextureArray[i], &shaderResourceViewDesc, &mShaderResourceViewArray[i]);
		D3D11SetDebugObjectName(mRenderTargetViewArray[i], "DeferredSRVA");
		if (FAILED(hr))
			return false;
	}

	DXGI_FORMAT format = DXGI_FORMAT_R16G16B16A16_FLOAT;

	textureDesc.Format = format;
	hr = device->CreateTexture2D(&textureDesc, NULL, &mLitSceneRenderTargetTexture);
	if (FAILED(hr))
		return false;

	renderTargetViewDesc.Format = format;
	hr = device->CreateRenderTargetView(mLitSceneRenderTargetTexture, &renderTargetViewDesc, &mLitSceneRenderTargetView);
	if (FAILED(hr))
		return false;

	shaderResourceViewDesc.Format = format;
	hr = device->CreateShaderResourceView(mLitSceneRenderTargetTexture, &shaderResourceViewDesc, &mLitSceneShaderResourceView);
	if (FAILED(hr))
		return false;

	return true;
}

void SurfaceBuffers::OnResize(ID3D11Device * device, UINT width, UINT height)
{
	Shutdown();
	Init(device, width, height);
}

void SurfaceBuffers::SetRenderTargets(ID3D11DeviceContext * dc, ID3D11DepthStencilView * depthStencilView)
{
	dc->OMSetRenderTargets(SurfaceBuffersIndex::Count,mRenderTargetViewArray,depthStencilView);
}
//clear two renderTargetView
void SurfaceBuffers::ClearRenderTargets(ID3D11DeviceContext * dc, XMFLOAT4 RGBA, ID3D11DepthStencilView * depthStencilView)
{
	float color[4];

	color[0] = RGBA.x;
	color[1] = RGBA.y;
	color[2] = RGBA.z;
	color[3] = RGBA.w;

	for (UINT i = 0; i < SurfaceBuffersIndex::Count; i++)
	{
		dc->ClearRenderTargetView(mRenderTargetViewArray[i], color);
	}
	dc->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

ID3D11RenderTargetView * SurfaceBuffers::GetRenderTarget(UINT bufferIndex)
{
	return mRenderTargetViewArray[bufferIndex];
}

ID3D11RenderTargetView * SurfaceBuffers::GetLitSceneRTV()
{
	return mLitSceneRenderTargetView;
}

ID3D11ShaderResourceView * SurfaceBuffers::GetLitSceneSRV()
{
	return mLitSceneShaderResourceView;
}

ID3D11Texture2D * SurfaceBuffers::GetLitSceneTexture2D()
{
	return mLitSceneRenderTargetTexture;
}

ID3D11ShaderResourceView * SurfaceBuffers::GetSRV(int view)
{
	return mShaderResourceViewArray[view];
}

ID3D11RenderTargetView * SurfaceBuffers::GetBackgroundRTV()
{
	return mLitSceneRenderTargetView;
}

ID3D11ShaderResourceView * SurfaceBuffers::GetBackgroundSRV()
{
	return mLitSceneShaderResourceView;
}

ID3D11Texture2D * SurfaceBuffers::GetBackgroundTexture2D()
{
	return mLitSceneRenderTargetTexture;
}

void SurfaceBuffers::Shutdown()
{
	for (UINT i = 0; i < SurfaceBuffersIndex::Count; i++)
	{
		
		ReleaseCOM(mShaderResourceViewArray[i]);
		ReleaseCOM(mRenderTargetViewArray[i]);
		ReleaseCOM(mRenderTargetTextureArray[i]);
	
	}

	ReleaseCOM(mLitSceneRenderTargetTexture);
	ReleaseCOM(mLitSceneRenderTargetView);
	ReleaseCOM(mLitSceneShaderResourceView);
}
