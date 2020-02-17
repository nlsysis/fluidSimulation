#pragma once
#include "d3dApp.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include <wrl/client.h>
#include "common/Camera.h"
#include "reference/Texture.h"
#include "SurfaceBuffers.h"
#include "common/objLoad/OBJLoadClass.h"
#include "OrthoWindow.h"
#include "LightShaderClass.h"

enum RenderModel
{
	SIM_MODEL_INITIAL = 0,
	SIM_MODEL_SPHERE,
	SIM_MODEL_LIGHT,
	SIM_MODEL_WATER
};

class FluidPBF : public D3DApp
{
public:
	FluidPBF(HINSTANCE hInstance);
	~FluidPBF();

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
	void SimulateFluid_Grid();
	void SimulateFluid(float fElapsedTime);
	void RenderFluid(float fElapsedTime);
	void InitCamera();
	void UpdateCamera(float dt);
	void RenderFluidInSphere(float fElapsedTime);

	void BuildRenderShader(const WCHAR* fileName);
	
	bool InitOBJModels();
	HRESULT CreateOBJBuffers();
	void BuildOBJShader();
	bool RenderOBJModel();
private:
	//Direct3D11 Grobal variables
	ID3D11ShaderResourceView* const   g_pNullSRV = NULL;       //helper to clear SRVS
	ID3D11UnorderedAccessView* const  g_pNullUAV = NULL;       //helper to clear UAVS
	ID3D11Buffer*  const              g_pNullBuffer = NULL;    //helper to clear buffers
	UINT                              g_iNullUINT = 0;         //helper to clear buffers

	//shader
	Microsoft::WRL::ComPtr <ID3D11VertexShader>       g_pParticlesVS;
	Microsoft::WRL::ComPtr <ID3D11GeometryShader>     g_pParticlesGS;
	Microsoft::WRL::ComPtr <ID3D11PixelShader>        g_pParticlesPS;

	Microsoft::WRL::ComPtr < ID3D11ComputeShader >              g_pBuildGridCS ;
	Microsoft::WRL::ComPtr < ID3D11ComputeShader >              g_pClearGridIndicesCS ;
	Microsoft::WRL::ComPtr < ID3D11ComputeShader >              g_pBuildGridIndicesCS ;
	Microsoft::WRL::ComPtr < ID3D11ComputeShader >              g_pRearrangeParticlesCS ;
	Microsoft::WRL::ComPtr < ID3D11ComputeShader >              g_pDensity_GridCS ;
	Microsoft::WRL::ComPtr < ID3D11ComputeShader >              g_pForce_GridCS ;
	Microsoft::WRL::ComPtr < ID3D11ComputeShader >              g_pIntegrateCS ;
	Microsoft::WRL::ComPtr < ID3D11ComputeShader >              g_pDensity_SimpleCS ;  //for test
	Microsoft::WRL::ComPtr < ID3D11ComputeShader >              g_pForce_SimpleCS ;  //for test
	Microsoft::WRL::ComPtr < ID3D11ComputeShader >              g_pSortBitonic ;
	Microsoft::WRL::ComPtr < ID3D11ComputeShader >              g_pSortTranspose ;

	//structured buffers
	Microsoft::WRL::ComPtr < ID3D11Buffer >                    g_pParticles ;
	Microsoft::WRL::ComPtr < ID3D11ShaderResourceView >        g_pParticlesSRV ;
	Microsoft::WRL::ComPtr < ID3D11UnorderedAccessView >       g_pParticlesUAV ;

	Microsoft::WRL::ComPtr < ID3D11Buffer >                      g_pSortedParticles ;
	Microsoft::WRL::ComPtr < ID3D11ShaderResourceView >          g_pSortedParticlesSRV ;
	Microsoft::WRL::ComPtr < ID3D11UnorderedAccessView >         g_pSortedParticlesUAV ;

	Microsoft::WRL::ComPtr < ID3D11Buffer >                      g_pParticleDensity ;
	Microsoft::WRL::ComPtr < ID3D11ShaderResourceView >          g_pParticleDensitySRV ;
	Microsoft::WRL::ComPtr < ID3D11UnorderedAccessView >         g_pParticleDensityUAV ;

	Microsoft::WRL::ComPtr < ID3D11Buffer >                       g_pParticleForces ;
	Microsoft::WRL::ComPtr < ID3D11ShaderResourceView >           g_pParticleForcesSRV ;
	Microsoft::WRL::ComPtr < ID3D11UnorderedAccessView >          g_pParticleForcesUAV ;

	Microsoft::WRL::ComPtr < ID3D11Buffer >                       g_pGrid ;
	Microsoft::WRL::ComPtr < ID3D11ShaderResourceView >           g_pGridSRV ;
	Microsoft::WRL::ComPtr < ID3D11UnorderedAccessView >          g_pGridUAV ;

	Microsoft::WRL::ComPtr < ID3D11Buffer >                       g_pGridPingPong ;
	Microsoft::WRL::ComPtr < ID3D11ShaderResourceView >           g_pGridPingPongSRV ;
	Microsoft::WRL::ComPtr < ID3D11UnorderedAccessView >          g_pGridPingPongUAV ;

	Microsoft::WRL::ComPtr < ID3D11Buffer>                       g_pGridIndices ;
	Microsoft::WRL::ComPtr < ID3D11ShaderResourceView>           g_pGridIndicesSRV ;
	Microsoft::WRL::ComPtr < ID3D11UnorderedAccessView>          g_pGridIndicesUAV ;

	//Constant Buffers
	Microsoft::WRL::ComPtr < ID3D11Buffer>			g_pcbSimulationConstants ;
	Microsoft::WRL::ComPtr < ID3D11Buffer>			g_pcbRenderConstants ;
	Microsoft::WRL::ComPtr < ID3D11Buffer>			g_pSortCB ;
	Microsoft::WRL::ComPtr < ID3D11Buffer>          g_pcbPlanes ;
	Microsoft::WRL::ComPtr < ID3D11Buffer>			g_pcbEyePos;


	//camera
	std::shared_ptr<Camera> m_pCamera;						  
	CameraMode m_CameraMode;

	//TextureManager
	TextureManager* m_TextureMgr;

	//depth/normal texture 
	SurfaceBuffers* m_SurfaceBuffers;

	RenderModel nRenderModel;

	//for obj models
	Microsoft::WRL::ComPtr <ID3D11VertexShader>       g_pObjLoadVS;
	Microsoft::WRL::ComPtr <ID3D11PixelShader>        g_pObjLoadPS;

	Microsoft::WRL::ComPtr <ID3D11Buffer>                    g_pLight;
	Microsoft::WRL::ComPtr <ID3D11ShaderResourceView>        g_pLightSRV;

	//Constant Buffers
	Microsoft::WRL::ComPtr <ID3D11Buffer>   g_pcbMatrix;
	Microsoft::WRL::ComPtr <ID3D11Buffer>   g_pcbPerFrame;
	Microsoft::WRL::ComPtr <ID3D11Buffer>   g_pcbMaterial;
	OBJLoadClass* mObjLoadClass;

	LightShaderClass* m_LightShaderClass;
	OrthoWindow* m_OrthoWindow;
};