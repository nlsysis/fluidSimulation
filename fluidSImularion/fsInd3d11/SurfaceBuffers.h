#pragma once
#include "common/d3dUtil.h"

enum SurfaceBuffersIndex
{
	Diffuse = 0,
	Normal,
	Count
};
class SurfaceBuffers
{
public:
	SurfaceBuffers();
	~SurfaceBuffers();

	bool Init(ID3D11Device* device, UINT width, UINT height);
	void OnResize(ID3D11Device* device, UINT width, UINT height);

	void SetRenderTargets(ID3D11DeviceContext* dc, ID3D11DepthStencilView* depthStencilView);
	void ClearRenderTargets(ID3D11DeviceContext* dc, XMFLOAT4 RGBA, ID3D11DepthStencilView* depthStencilView);
	ID3D11RenderTargetView* GetRenderTarget(UINT bufferIndex);

	ID3D11RenderTargetView* GetLitSceneRTV();
	ID3D11ShaderResourceView* GetLitSceneSRV();
	ID3D11Texture2D* GetLitSceneTexture2D();

	ID3D11ShaderResourceView* GetSRV(int view);

	ID3D11RenderTargetView* GetBackgroundRTV();
	ID3D11ShaderResourceView* GetBackgroundSRV();
	ID3D11Texture2D* GetBackgroundTexture2D();

	void Shutdown();
private:
	ID3D11Texture2D* mRenderTargetTextureArray[SurfaceBuffersIndex::Count];
	ID3D11RenderTargetView* mRenderTargetViewArray[SurfaceBuffersIndex::Count];
	ID3D11ShaderResourceView* mShaderResourceViewArray[SurfaceBuffersIndex::Count];

	ID3D11Texture2D* mLitSceneRenderTargetTexture;
	ID3D11RenderTargetView* mLitSceneRenderTargetView;
	ID3D11ShaderResourceView* mLitSceneShaderResourceView;
};