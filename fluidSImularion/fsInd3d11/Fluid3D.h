#pragma once
#include "d3dApp.h"
#include "d3dx11Effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"

class Fluid3D : public D3DApp
{	
public:
	Fluid3D(HINSTANCE hInstance);
	~Fluid3D();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();
	void DrawScene(float dt);
private:
	HRESULT CreateSimulationBuffers();
	void BuildShader();
	void SafeCompileShaderFromFile(const WCHAR* fileName, LPCSTR enterPoint, LPCSTR shaderModel, ID3DBlob **ppBlob);
	void GPUSort(ID3D11DeviceContext* pd3dImmediateContext,
		ID3D11UnorderedAccessView* inUAV, ID3D11ShaderResourceView* inSRV,
		ID3D11UnorderedAccessView* tempUAV, ID3D11ShaderResourceView* tempSRV);
	void SimulateFluid_Simple(ID3D11DeviceContext* pd3dImmediateContext);
	void SimulateFluid_Grid(ID3D11DeviceContext* pd3dImmediateContext);
	void SimulateFluid(ID3D11DeviceContext* pd3dImmediateContext, float fElapsedTime);
	void RenderFluid(ID3D11DeviceContext* pd3dImmediateContext, float fElapsedTime);
	void UpdateInput(float dt);

	//for draw box
	void BuildConstantBuffer();
	void BuildGeometryBuffers();
	void BuildFX();
	void SetShaderParameters();
	void DrawBox();
private:
	//Direct3D11 Grobal variables
	ID3D11ShaderResourceView* const   g_pNullSRV = NULL;       //helper to clear SRVS
	ID3D11UnorderedAccessView* const  g_pNullUAV = NULL;       //helper to clear UAVS
	ID3D11Buffer*  const              g_pNullBuffer = NULL;    //helper to clear buffers
	UINT                              g_iNullUINT = 0;         //helper to clear buffers

	//Shaders
	ID3D11VertexShader*               g_pParticlesVS = NULL;
	ID3D11GeometryShader*             g_pParticlesGS = NULL;
	ID3D11PixelShader*                g_pParticlesPS = NULL;

	ID3D11ComputeShader*              g_pBuildGridCS = NULL;
	ID3D11ComputeShader*              g_pClearGridIndicesCS = NULL;
	ID3D11ComputeShader*              g_pBuildGridIndicesCS = NULL;
	ID3D11ComputeShader*              g_pRearrangeParticlesCS = NULL;
	ID3D11ComputeShader*              g_pDensity_GridCS = NULL;
	ID3D11ComputeShader*              g_pForce_GridCS = NULL;
	ID3D11ComputeShader*              g_pIntegrateCS = NULL;
	ID3D11ComputeShader*              g_pDensity_SimpleCS = NULL;  //for test
	ID3D11ComputeShader*              g_pForce_SimpleCS = NULL;  //for test
	ID3D11ComputeShader*              g_pSortBitonic = NULL;
	ID3D11ComputeShader*              g_pSortTranspose = NULL;

	//structured buffers
	ID3D11Buffer*                    g_pParticles = NULL;
	ID3D11ShaderResourceView*        g_pParticlesSRV = NULL;
	ID3D11UnorderedAccessView*       g_pParticlesUAV = NULL;

	ID3D11Buffer*                       g_pSortedParticles = NULL;
	ID3D11ShaderResourceView*           g_pSortedParticlesSRV = NULL;
	ID3D11UnorderedAccessView*          g_pSortedParticlesUAV = NULL;

	ID3D11Buffer*                       g_pParticleDensity = NULL;
	ID3D11ShaderResourceView*           g_pParticleDensitySRV = NULL;
	ID3D11UnorderedAccessView*          g_pParticleDensityUAV = NULL;

	ID3D11Buffer*                       g_pParticleForces = NULL;
	ID3D11ShaderResourceView*           g_pParticleForcesSRV = NULL;
	ID3D11UnorderedAccessView*          g_pParticleForcesUAV = NULL;

	ID3D11Buffer*                       g_pGrid = NULL;
	ID3D11ShaderResourceView*           g_pGridSRV = NULL;
	ID3D11UnorderedAccessView*          g_pGridUAV = NULL;

	ID3D11Buffer*                       g_pGridPingPong = NULL;
	ID3D11ShaderResourceView*           g_pGridPingPongSRV = NULL;
	ID3D11UnorderedAccessView*          g_pGridPingPongUAV = NULL;

	ID3D11Buffer*                       g_pGridIndices = NULL;
	ID3D11ShaderResourceView*           g_pGridIndicesSRV = NULL;
	ID3D11UnorderedAccessView*          g_pGridIndicesUAV = NULL;

	//Constant Buffers
	ID3D11Buffer*			g_pcbSimulationConstants = NULL;
	ID3D11Buffer*			g_pcbRenderConstants = NULL;
	ID3D11Buffer*			g_pSortCB = NULL;
	ID3D11Buffer*           g_pcbPlanes = NULL;


	float mParticleRadius;
	float mTheta;
	float mPhi;
	float mRadius;

	XMFLOAT4X4 mWorld;
	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	//for draw cube
	ID3D11Buffer* mBoxVB;
	ID3D11Buffer* mBoxIB;
	ID3D11Buffer* mMatrixBuffer;
	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;
	ID3D11InputLayout* mInputLayout;
};