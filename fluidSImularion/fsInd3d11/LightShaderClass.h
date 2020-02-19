#pragma once
#include <wrl/client.h>
#include "common/d3dUtil.h"
#include "reference/LightHelper.h"
#include "cbufferStruct.h"

class LightShaderClass
{
public:
	LightShaderClass();
	~LightShaderClass();

	bool Init(ID3D11Device* device);
	void OnResize();
	void UpdateScene(float dt);
	void RenderLight(ID3D11DeviceContext* deviceContext);
	void SetCBLightBufferPara(XMMATRIX mvp, XMFLOAT3 eyePosW, bool skipLighting, XMMATRIX camViewProjInv);
	void SetLight(ID3D11DeviceContext* deviceContext);
private:
	void BuildShader(ID3D11Device* device);
	void SetLightParameters(ID3D11DeviceContext* deviceContext);
	void SafeCompileShaderFromFile(const WCHAR* fileName, LPCSTR enterPoint, LPCSTR shaderModel, ID3DBlob **ppBlob);

private:
	Microsoft::WRL::ComPtr <ID3D11Buffer>               mDirLightBuffer;
	Microsoft::WRL::ComPtr < ID3D11ShaderResourceView>  DirLightResourceView;
	Microsoft::WRL::ComPtr < ID3D11Buffer>              mPointLightBuffer;
	Microsoft::WRL::ComPtr < ID3D11ShaderResourceView>  PointLightResourceView;
	Microsoft::WRL::ComPtr < ID3D11Buffer>              mSpotLightBuffer;
	Microsoft::WRL::ComPtr < ID3D11ShaderResourceView>  SpotLightResourceView;

	Microsoft::WRL::ComPtr < ID3D11VertexShader>  mVertexShader;
	Microsoft::WRL::ComPtr < ID3D11PixelShader>  mPixelShader;
	Microsoft::WRL::ComPtr < ID3D11InputLayout>  mInputLayout;

	//constant buffer
	Microsoft::WRL::ComPtr < ID3D11Buffer> g_pcbMatrixBufferType;
	Microsoft::WRL::ComPtr < ID3D11Buffer> g_pcbLightBuffer;

	XMFLOAT4X4 g_mViewProjection;

	std::vector<PointLight> mPointLights;
	std::vector<DirectionalLight> mDirLights;
	std::vector<SpotLight> mSpotLights;

	cbLightBuffer m_lightBuffer;
	MatrixBufferType m_matrixBufferType;
};
