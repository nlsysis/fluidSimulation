#pragma once
#include "common/d3dUtil.h"

class OrthoWindow
{
private:
	struct PosTex
	{
		XMFLOAT3 position;
		XMFLOAT2 texCoord;
	};
public:
	OrthoWindow();
	~OrthoWindow();

	bool Initialize(ID3D11Device* device, int windowWidth, int windowHeight);
	void Shutdown();
	void Render(ID3D11DeviceContext* dc);

	void OnResize(ID3D11Device* device, int width, int height);

	int GetIndexCount();

private:
	bool InitializeBuffers(ID3D11Device* device, int windowWidth, int windowHeight);
	void ShutdownBuffers();
	void PrepareBuffers(ID3D11DeviceContext* dc);

private:
	ID3D11Buffer* mVertexBuffer;
	ID3D11Buffer* mIndexBuffer;
	int mVertexCount, mIndexCount;
};