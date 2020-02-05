#include "Fluid3D.h"
#include "cbufferStruct.h"
#include <random>
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
//global variables

//Compute Shader Constants
//Grid cell key size for sorting.8-bit for x and y
//const UINT NUM_GRID_INDICES = 65536;
const UINT GRID_SIZE = 64;    //the number of grid in 3 dim
const UINT NUM_GRID_INDICES = (GRID_SIZE * GRID_SIZE * GRID_SIZE);

//Numthreads size for the simulation
const UINT SIMULATION_BLOCK_SIZE = 256;

//Numthreads size for the sort
const UINT BITONIC_BLOCK_SIZE = 512;
const UINT TRANSPOSE_BLOCK_SIZE = 16;

const UINT NUM_PARTICLES_8K = 8 * 1024;
const UINT NUM_PARTICLES_16K = 16 * 1024;
UINT g_iNumParticles3D = NUM_PARTICLES_8K;   //default use particle number

//Particle Properties
//float g_fInitialParticleSpacing3D = 0.0045f;      //initial particle space
//float g_fSmoothlen3D = 0.012f;                   //光滑核函数作用范围长度
//float g_fPressureStiffness3D = 2000.0f;          //
//float g_fRestDensity3D = 1000.0f;                //p0静态流体密度
//float g_fParticleMass3D = 0.0002f;               //the mass of particles
//float g_fViscosity3D = 1.0f;                     //
//float g_fParticleRenderSize3D = 0.0012f;
//float g_fMaxAllowableTimeStep3D = 0.0075f;
//float g_fParticleAspectRatio = 1.0f;
//float g_fDelta = 10.0f;
//float g_fParticleRadius = 0.0012f;

float g_fSmoothlen3D = 2.0f;                  //for test
float g_fPressureStiffness3D = 1000.25f;
float g_fRestDensity3D = 1.5f;
float g_fParticleMass3D = 2.5f;
float g_fViscosity3D = 1.0f;                     //

float g_fMaxAllowableTimeStep3D = 0.005f;        //
float g_fParticleRenderSize3D = 0.4f;          //
float g_fParticleRadius = 1.0f;
//Gravity Directions
const XMFLOAT3 GRAVITY_DOWN(0, -9.8f, 0.0f);
const XMFLOAT3 GRAVITY_UP(0.0f, 9.8f, 0.0f);
const XMFLOAT3 GRAVITY_LEFT(-9.8f, 0.0f, 0.0f);
const XMFLOAT3 GRAVITY_RIGHT(9.8f, 0.0f, 0.0f);
XMFLOAT3 g_vGravity3D = GRAVITY_DOWN;  //default gracity direction



//Map Wall Collision Planes
float g_fWallStiffness3D = 3000.0f;
float g_boundaryDampening = 256.0f;
float g_speedLimiting = 200.0f;
//float g_fWallStiffness3D = 0.7f;


//Map Size
//These values should not be larger than GRID_SIZE * fSmothlen[the volumn of simulation space]
//Since the map must be divided up into fSmoothlen sized grid cells
//And the grid cell is used as a 32-bit sort key,32-bits for x ,y and z
//float g_fMapHeight3D = 0.3f;
//float g_fMapWidth3D = 0.5f;
//float g_fMapLength3D = 0.5f;

float g_fMapHeight3D = GRID_SIZE;
float g_fMapWidth3D = GRID_SIZE;
float g_fMapLength3D = GRID_SIZE;
XMFLOAT4 g_vPlanes[6] = {
	XMFLOAT4(1,0,0,-g_fParticleRadius),
	XMFLOAT4(0,1,0,-g_fParticleRadius),
	XMFLOAT4(0,0,1,-g_fParticleRadius),
	XMFLOAT4(-1,0,0,g_fMapWidth3D + g_fParticleRadius),
	XMFLOAT4(0,-1,0,g_fMapHeight3D + g_fParticleRadius),
	XMFLOAT4(0,0,-1,g_fMapLength3D + g_fParticleRadius)
};


//Simulation Algorithm
enum eSimulationMode3D
{
	SIM_MODE_SIMPLE,
	SIM_MODE_SHARED,
	SIM_MODE_GRID
};

eSimulationMode3D g_eSimMode = SIM_MODE_GRID;


Fluid3D::Fluid3D(HINSTANCE hInstance)
	:D3DApp(hInstance), mTheta(1.5f*MathHelper::Pi), mPhi(0.25f*MathHelper::Pi),mRadius(200.0f),
	mBoxVB(0), mBoxIB(0), mVertexShader(0), mPixelShader(0), mInputLayout(0)
{
	mMainWndCaption = L"Fluid 3D Demo";
	mEnable4xMsaa = false;
	g_iNullUINT = 0;         //helper to clear buffers

	//Shaders
	g_pParticlesVS = NULL;
	g_pParticlesGS = NULL;
	g_pParticlesPS = NULL;

	g_pBuildGridCS = NULL;
	g_pClearGridIndicesCS = NULL;
	g_pBuildGridIndicesCS = NULL;
	g_pRearrangeParticlesCS = NULL;

	g_pDensity_GridCS = NULL;
	g_pForce_GridCS = NULL;
	g_pIntegrateCS = NULL;
	g_pDensity_SimpleCS = NULL;
	g_pForce_SimpleCS = NULL;
	g_pSortBitonic = NULL;
	g_pSortTranspose = NULL;

	//structured buffers
	g_pParticles = NULL;
	g_pParticlesSRV = NULL;
	g_pParticlesUAV = NULL;

	g_pSortedParticles = NULL;
	g_pSortedParticlesSRV = NULL;
	g_pSortedParticlesUAV = NULL;

	g_pParticleDensity = NULL;
	g_pParticleDensitySRV = NULL;
	g_pParticleDensityUAV = NULL;

	g_pParticleForces = NULL;
	g_pParticleForcesSRV = NULL;
	g_pParticleForcesUAV = NULL;

	g_pGrid = NULL;
	g_pGridSRV = NULL;
	g_pGridUAV = NULL;

	g_pGridPingPong = NULL;
	g_pGridPingPongSRV = NULL;
	g_pGridPingPongUAV = NULL;

	g_pGridIndices = NULL;
	g_pGridIndicesSRV = NULL;
	g_pGridIndicesUAV = NULL;

	//Constant Buffers
	g_pcbSimulationConstants = NULL;
	g_pcbRenderConstants = NULL;
	g_pSortCB = NULL;

	ReleaseCOM(mBoxVB);
	ReleaseCOM(mBoxIB);
	ReleaseCOM(mPixelShader);
	ReleaseCOM(mVertexShader);
	ReleaseCOM(mInputLayout);
}

Fluid3D::~Fluid3D()
{
	//release constant shader
	SAFE_RELEASE(g_pcbSimulationConstants);
	SAFE_RELEASE(g_pcbRenderConstants);
	SAFE_RELEASE(g_pSortCB);

	SAFE_RELEASE(g_pParticlesVS);
	SAFE_RELEASE(g_pParticlesGS);
	SAFE_RELEASE(g_pParticlesPS);

	SAFE_RELEASE(g_pIntegrateCS);
	
	SAFE_RELEASE(g_pDensity_GridCS);
	SAFE_RELEASE(g_pForce_GridCS);
	SAFE_RELEASE(g_pBuildGridCS);
	SAFE_RELEASE(g_pDensity_SimpleCS);
	SAFE_RELEASE(g_pForce_SimpleCS);
	SAFE_RELEASE(g_pClearGridIndicesCS);
	SAFE_RELEASE(g_pBuildGridIndicesCS);
	SAFE_RELEASE(g_pRearrangeParticlesCS);
	SAFE_RELEASE(g_pSortBitonic);
	SAFE_RELEASE(g_pSortTranspose);

	SAFE_RELEASE(g_pParticles);
	SAFE_RELEASE(g_pParticlesSRV);
	SAFE_RELEASE(g_pParticlesUAV);

	SAFE_RELEASE(g_pSortedParticles);
	SAFE_RELEASE(g_pSortedParticlesSRV);
	SAFE_RELEASE(g_pSortedParticlesUAV);

	SAFE_RELEASE(g_pParticleForces);
	SAFE_RELEASE(g_pParticleForcesSRV);
	SAFE_RELEASE(g_pParticleForcesUAV);

	SAFE_RELEASE(g_pParticleDensity);
	SAFE_RELEASE(g_pParticleDensitySRV);
	SAFE_RELEASE(g_pParticleDensityUAV);

	SAFE_RELEASE(g_pGridSRV);
	SAFE_RELEASE(g_pGridUAV);
	SAFE_RELEASE(g_pGrid);

	SAFE_RELEASE(g_pGridPingPongSRV);
	SAFE_RELEASE(g_pGridPingPongUAV);
	SAFE_RELEASE(g_pGridPingPong);

	SAFE_RELEASE(g_pGridIndicesSRV);
	SAFE_RELEASE(g_pGridIndicesUAV);
	SAFE_RELEASE(g_pGridIndices);
}

bool Fluid3D::Init()
{
	if (!D3DApp::Init())
		return false;

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);

	/*XMMATRIX P = XMMatrixOrthographicLH(64, 64 * 4/3, 0, 1000);
	XMStoreFloat4x4(&mProj, P);*/

	mParticleRadius = 1.0f;
	CreateSimulationBuffers();
	BuildShader();


	BuildGeometryBuffers();
	BuildFX();
	BuildConstantBuffer();

	return true;
}

void Fluid3D::OnResize()
{
	D3DApp::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);

	/*XMMATRIX P = XMMatrixOrthographicLH(64, 64 * 4 / 3, 0, 1000);
	XMStoreFloat4x4(&mProj, P);*/
}

void Fluid3D::UpdateScene(float dt)
{
	// Convert Spherical to Cartesian coordinates.
	float x = mRadius * sinf(mPhi)*cosf(mTheta);
	float z = mRadius * sinf(mPhi)*sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, V);

	UpdateInput(dt);
}

void Fluid3D::DrawScene()
{
}

void Fluid3D::DrawScene(float fElapsedTime)
{
	md3dDeviceContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Silver));
	md3dDeviceContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	SimulateFluid(md3dDeviceContext, fElapsedTime);

	RenderFluid(md3dDeviceContext, fElapsedTime);

	//DrawBox();
	
	//imgui stuff
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	
	//Imgui setting
	if (ImGui::Begin("Fluid Coff"))
	{
		ImGui::SliderFloat("Gravity", &g_vGravity3D.y, -100.0f, 10.0f);
		ImGui::SliderFloat("SmoothLen", &g_fSmoothlen3D,0.0f, 10.0f);
		ImGui::SliderFloat("RestDensity", &g_fRestDensity3D, 800.0f, 1200.0f);
		ImGui::SliderFloat("ParticleMass", &g_fParticleMass3D,0.0f, 1.0f);
		ImGui::SliderFloat("Viscosity", &g_fViscosity3D, 0.0f, 1.0f);
		ImGui::SliderFloat("ForceStiff", &g_fWallStiffness3D, 3000.0f, 5000.0f);

		float  fDensityCoef = g_fParticleMass3D * 315.0f / (64.0f * XM_PI * pow(g_fSmoothlen3D, 9));;
		ImGui::Text("ParticleRenderSize:%0.10f", &fDensityCoef, 0.0f, 10.0f);

		float fGradPressureCoef = g_fParticleMass3D * -45.0f / (XM_PI * pow(g_fSmoothlen3D, 6));
		ImGui::Text("GradPressureCoef %0.10f", &fGradPressureCoef);

		float fLapViscosityCoef = g_fParticleMass3D * g_fViscosity3D * 45.0f / (XM_PI * pow(g_fSmoothlen3D, 6));
		ImGui::Text("LapViscosityCoef  %0.10f", &fLapViscosityCoef);
	}
	ImGui::End();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	HR(mSwapChain->Present(0, 0));
	
}

HRESULT Fluid3D::CreateSimulationBuffers()
{
	HRESULT hr = S_OK;
	// Destroy the old buffers in case the number of particles has changed
	SAFE_RELEASE(g_pParticles);
	SAFE_RELEASE(g_pParticlesSRV);
	SAFE_RELEASE(g_pParticlesUAV);

	SAFE_RELEASE(g_pSortedParticles);
	SAFE_RELEASE(g_pSortedParticlesSRV);
	SAFE_RELEASE(g_pSortedParticlesUAV);

	SAFE_RELEASE(g_pParticleForces);
	SAFE_RELEASE(g_pParticleForcesSRV);
	SAFE_RELEASE(g_pParticleForcesUAV);

	SAFE_RELEASE(g_pParticleDensity);
	SAFE_RELEASE(g_pParticleDensitySRV);
	SAFE_RELEASE(g_pParticleDensityUAV);

	SAFE_RELEASE(g_pGridSRV);
	SAFE_RELEASE(g_pGridUAV);
	SAFE_RELEASE(g_pGrid);

	SAFE_RELEASE(g_pGridPingPongSRV);
	SAFE_RELEASE(g_pGridPingPongUAV);
	SAFE_RELEASE(g_pGridPingPong);

	SAFE_RELEASE(g_pGridIndicesSRV);
	SAFE_RELEASE(g_pGridIndicesUAV);
	SAFE_RELEASE(g_pGridIndices);

	// Create the initial particle positions
	// This is only used to populate the GPU buffers on creation
	//const UINT iStartingWidth = (UINT)sqrt((FLOAT)g_iNumParticles3D);
	//Particle* particles = new Particle[g_iNumParticles3D];
	//ZeroMemory(particles, sizeof(Particle) * g_iNumParticles3D);
	//for (UINT i = 0; i < g_iNumParticles3D; i++)
	//{
	//	// Arrange the particles in a nice square
	//	UINT x = i % iStartingWidth;
	//	UINT y = i / iStartingWidth;
	//	particles[i].vPosition = XMFLOAT3(g_fInitialParticleSpacing3D * (FLOAT)x, g_fInitialParticleSpacing3D * (FLOAT)y, g_fInitialParticleSpacing3D * (FLOAT)x);
	//	particles[i].vVelocity = XMFLOAT3(0.0f, 0.0f,0.0f);
	//}

	Particle* particles = new Particle[g_iNumParticles3D];
	ZeroMemory(particles, sizeof(Particle) * g_iNumParticles3D);
	float rndX, rndY, rndZ;
	std::mt19937 eng(std::random_device{}());
	float velRange = GRID_SIZE * 0.5f;
	std::uniform_real_distribution<float> dist(0.0, 30);   //intial fluid position

	for (UINT i = 0; i < g_iNumParticles3D; i++)
	{
		rndX = dist(eng) ;
		rndY = dist(eng);
		rndZ = dist(eng) ;

		particles[i].vPosition = XMFLOAT3(rndX, rndY, rndZ);
		particles[i].vVelocity = XMFLOAT3(0.0f, 0.0f, 0.0f);

	}

	// Create Structured Buffers
	HR(CreateStructuredBuffer< Particle >(md3dDevice, g_iNumParticles3D, &g_pParticles, &g_pParticlesSRV, &g_pParticlesUAV, particles));
	D3D11SetDebugObjectName(g_pParticles, "Particles");
	D3D11SetDebugObjectName(g_pParticlesSRV, "Particles SRV");
	D3D11SetDebugObjectName(g_pParticlesUAV, "Particles UAV");
	HR(CreateStructuredBuffer< Particle >(md3dDevice, g_iNumParticles3D, &g_pSortedParticles, &g_pSortedParticlesSRV, &g_pSortedParticlesUAV, particles));
	D3D11SetDebugObjectName(g_pSortedParticles, "Sorted");
	D3D11SetDebugObjectName(g_pSortedParticlesSRV, "Sorted SRV");
	D3D11SetDebugObjectName(g_pSortedParticlesUAV, "Sorted UAV");
	HR(CreateStructuredBuffer< ParticleForces >(md3dDevice, g_iNumParticles3D, &g_pParticleForces, &g_pParticleForcesSRV, &g_pParticleForcesUAV));
	D3D11SetDebugObjectName(g_pParticleForces, "Forces");
	D3D11SetDebugObjectName(g_pParticleForcesSRV, "Forces SRV");
	D3D11SetDebugObjectName(g_pParticleForcesUAV, "Forces UAV");
	HR(CreateStructuredBuffer< ParticleDensity >(md3dDevice, g_iNumParticles3D, &g_pParticleDensity, &g_pParticleDensitySRV, &g_pParticleDensityUAV));
	D3D11SetDebugObjectName(g_pParticleDensity, "Density");
	D3D11SetDebugObjectName(g_pParticleDensitySRV, "Density SRV");
	D3D11SetDebugObjectName(g_pParticleDensityUAV, "Density UAV");
	HR(CreateStructuredBuffer< UINT2 >(md3dDevice, g_iNumParticles3D, &g_pGrid, &g_pGridSRV, &g_pGridUAV));
	D3D11SetDebugObjectName(g_pGrid, "Grid");
	D3D11SetDebugObjectName(g_pGridSRV, "Grid SRV");
	D3D11SetDebugObjectName(g_pGridUAV, "Grid UAV");
	HR(CreateStructuredBuffer< UINT2 >(md3dDevice, g_iNumParticles3D, &g_pGridPingPong, &g_pGridPingPongSRV, &g_pGridPingPongUAV));
	D3D11SetDebugObjectName(g_pGridPingPong, "PingPong");
	D3D11SetDebugObjectName(g_pGridPingPongSRV, "PingPong SRV");
	D3D11SetDebugObjectName(g_pGridPingPongUAV, "PingPong UAV");
	HR(CreateStructuredBuffer< UINT2 >(md3dDevice, NUM_GRID_INDICES, &g_pGridIndices, &g_pGridIndicesSRV, &g_pGridIndicesUAV));
	D3D11SetDebugObjectName(g_pGridIndices, "Indices");
	D3D11SetDebugObjectName(g_pGridIndicesSRV, "Indices SRV");
	D3D11SetDebugObjectName(g_pGridIndicesUAV, "Indices UAV");
	delete[] particles;

	// Create Constant Buffers
	HR(CreateConstantBuffer< CBTest >(md3dDevice, &g_pcbSimulationConstants));
	HR(CreateConstantBuffer< CBPlanes >(md3dDevice, &g_pcbPlanes));
	HR(CreateConstantBuffer< CBRenderConstants >(md3dDevice, &g_pcbRenderConstants));
	HR(CreateConstantBuffer< SortCB >(md3dDevice, &g_pSortCB));

	D3D11SetDebugObjectName(g_pcbSimulationConstants, "SimluationCB");
	D3D11SetDebugObjectName(g_pcbPlanes, "Planes");
	D3D11SetDebugObjectName(g_pcbRenderConstants, "Render");
	D3D11SetDebugObjectName(g_pSortCB, "Sort");
	return S_OK;
}

//Compile the Shaders
void Fluid3D::BuildShader()
{
	ID3DBlob* pBlob = NULL;
	//rendering shaders
	SafeCompileShaderFromFile(L".\\shader\\Fluid3DRender.hlsl", "ParticleVS", "vs_5_0", &pBlob);
	HR(md3dDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pParticlesVS));
	D3D11SetDebugObjectName(g_pParticlesVS, "ParticlesVS");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\Fluid3DRender.hlsl", "ParticleGS", "gs_5_0", &pBlob);
	HR(md3dDevice->CreateGeometryShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pParticlesGS));
	D3D11SetDebugObjectName(g_pParticlesGS, "ParticlesGS");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\Fluid3DRender.hlsl", "ParticlePS", "ps_5_0", &pBlob);
	HR(md3dDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pParticlesPS));
	D3D11SetDebugObjectName(g_pParticlesPS, "ParticlesPS");
	SAFE_RELEASE(pBlob);

	// Compute Shaders
	const char* CSTarget = (md3dDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0) ? "cs_5_0" : "cs_4_0";
	SafeCompileShaderFromFile(L".\\shader\\Fluid3DTest.hlsl", "IntegrateCS", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pIntegrateCS));
	D3D11SetDebugObjectName(g_pIntegrateCS, "IntegrateCS");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\Fluid3DTest.hlsl", "DensityCS_Simple", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pDensity_SimpleCS));
	D3D11SetDebugObjectName(g_pDensity_SimpleCS, "DensityCS_Simple");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\Fluid3DTest.hlsl", "ForceCS_Simple", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pForce_SimpleCS));
	D3D11SetDebugObjectName(g_pForce_SimpleCS, "ForceCS_Simple");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\Fluid3DTest.hlsl", "DensityCS_Grid", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pDensity_GridCS));
	D3D11SetDebugObjectName(g_pDensity_GridCS, "DensityCS_Grid");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\Fluid3DTest.hlsl", "ForceCS_Grid", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pForce_GridCS));
	D3D11SetDebugObjectName(g_pForce_GridCS, "ForceCS_Grid");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\Fluid3DTest.hlsl", "BuildGridCS", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pBuildGridCS));
	D3D11SetDebugObjectName(g_pBuildGridCS, "BuildGridCS");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\Fluid3DTest.hlsl", "ClearGridIndicesCS", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pClearGridIndicesCS));
	D3D11SetDebugObjectName(g_pClearGridIndicesCS, "ClearGridIndicesCS");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\Fluid3DTest.hlsl", "BuildGridIndicesCS", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pBuildGridIndicesCS));
	D3D11SetDebugObjectName(g_pBuildGridIndicesCS, "BuildGridIndicesCS");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\Fluid3DTest.hlsl", "RearrangeParticlesCS", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pRearrangeParticlesCS));
	D3D11SetDebugObjectName(g_pRearrangeParticlesCS, "RearrangeParticlesCS");
	SAFE_RELEASE(pBlob);

	// Sort Shaders
	SafeCompileShaderFromFile(L".\\shader\\BitonicSortCS.hlsl", "BitonicSort", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pSortBitonic));
	D3D11SetDebugObjectName(g_pSortBitonic, "BitonicSort");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\BitonicSortCS.hlsl", "MatrixTranspose", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pSortTranspose));
	D3D11SetDebugObjectName(g_pSortTranspose, "MatrixTranspose");
	SAFE_RELEASE(pBlob);
}


void Fluid3D::SafeCompileShaderFromFile(const WCHAR * fileName, LPCSTR enterPoint, LPCSTR shaderModel, ID3DBlob **ppBlob)
{
	ID3D10Blob* compilationMsgs = NULL;
	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
#ifdef _DEBUG
	dwShaderFlags |= D3D10_SHADER_DEBUG;
	dwShaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif
	HRESULT hr = D3DX11CompileFromFile(fileName, 0, 0, enterPoint, shaderModel, D3D10_SHADER_ENABLE_STRICTNESS,
		0, 0, ppBlob, &compilationMsgs, 0);
	// compilationMsgs can store errors or warnings.
	if (compilationMsgs != 0)
	{
		MessageBoxA(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
		ReleaseCOM(compilationMsgs);
	}

	// Even if there are no compilationMsgs, check to make sure there were no other errors.
	if (FAILED(hr))
	{
		DXTrace(__FILE__, (DWORD)__LINE__, hr, L"D3DX11CompileFromFile", true);
	}
}

//--------------------------------------------------------------------------------------
// GPU Bitonic Sort
// For more information, please see the ComputeShaderSort11 sample
//--------------------------------------------------------------------------------------
void Fluid3D::GPUSort(ID3D11DeviceContext * pd3dImmediateContext,
	ID3D11UnorderedAccessView * inUAV,
	ID3D11ShaderResourceView * inSRV,
	ID3D11UnorderedAccessView * tempUAV,
	ID3D11ShaderResourceView * tempSRV)
{
	pd3dImmediateContext->CSSetConstantBuffers(0, 1, &g_pSortCB);

	const UINT NUM_ELEMENTS = g_iNumParticles3D;
	const UINT MATRIX_WIDTH = BITONIC_BLOCK_SIZE;
	const UINT MATRIX_HEIGHT = NUM_ELEMENTS / BITONIC_BLOCK_SIZE;

	// Sort the data
   // First sort the rows for the levels <= to the block size
	for (UINT level = 2; level <= BITONIC_BLOCK_SIZE; level <<= 1)
	{
		SortCB constants = { level, level, MATRIX_HEIGHT, MATRIX_WIDTH };
		pd3dImmediateContext->UpdateSubresource(g_pSortCB, 0, NULL, &constants, 0, 0);

		// Sort the row data
		UINT UAVInitialCounts = 0;
		pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &inUAV, &UAVInitialCounts);
		pd3dImmediateContext->CSSetShader(g_pSortBitonic, NULL, 0);
		pd3dImmediateContext->Dispatch(NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1);
	}
	// Then sort the rows and columns for the levels > than the block size
  // Transpose. Sort the Columns. Transpose. Sort the Rows.
	for (UINT level = (BITONIC_BLOCK_SIZE << 1); level <= NUM_ELEMENTS; level <<= 1)
	{
		SortCB constants1 = { (level / BITONIC_BLOCK_SIZE), (level & ~NUM_ELEMENTS) / BITONIC_BLOCK_SIZE, MATRIX_WIDTH, MATRIX_HEIGHT };
		pd3dImmediateContext->UpdateSubresource(g_pSortCB, 0, NULL, &constants1, 0, 0);

		// Transpose the data from buffer 1 into buffer 2
		ID3D11ShaderResourceView* pViewNULL = NULL;
		UINT UAVInitialCounts = 0;
		pd3dImmediateContext->CSSetShaderResources(0, 1, &pViewNULL);
		pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &tempUAV, &UAVInitialCounts);
		pd3dImmediateContext->CSSetShaderResources(0, 1, &inSRV);
		pd3dImmediateContext->CSSetShader(g_pSortTranspose, NULL, 0);
		pd3dImmediateContext->Dispatch(MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE, MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE, 1);

		// Sort the transposed column data
		pd3dImmediateContext->CSSetShader(g_pSortBitonic, NULL, 0);
		pd3dImmediateContext->Dispatch(NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1);

		SortCB constants2 = { BITONIC_BLOCK_SIZE, level, MATRIX_HEIGHT, MATRIX_WIDTH };
		pd3dImmediateContext->UpdateSubresource(g_pSortCB, 0, NULL, &constants2, 0, 0);

		// Transpose the data from buffer 2 back into buffer 1
		pd3dImmediateContext->CSSetShaderResources(0, 1, &pViewNULL);
		pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &inUAV, &UAVInitialCounts);
		pd3dImmediateContext->CSSetShaderResources(0, 1, &tempSRV);
		pd3dImmediateContext->CSSetShader(g_pSortTranspose, NULL, 0);
		pd3dImmediateContext->Dispatch(MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE, MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE, 1);

		// Sort the row data
		pd3dImmediateContext->CSSetShader(g_pSortBitonic, NULL, 0);
		pd3dImmediateContext->Dispatch(NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1);
	}
}

//--------------------------------------------------------------------------------------
// GPU Fluid Simulation - Optimized Algorithm using a Grid + Sort
// Algorithm Overview:
//    Build Grid: For every particle, calculate a hash based on the grid cell it is in
//    Sort Grid: Sort all of the particles based on the grid ID hash
//        Particles in the same cell will now be adjacent in memory
//    Build Grid Indices: Located the start and end offsets for each cell
//    Rearrange: Rearrange the particles into the same order as the grid for easy lookup
//    Density, Force, Integrate: Perform the normal fluid simulation algorithm
//        Except now, only calculate particles from the 8 adjacent cells + current cell
//--------------------------------------------------------------------------------------
void Fluid3D::SimulateFluid_Grid(ID3D11DeviceContext* pd3dImmediateContext)
{
	UINT UAVInitialCounts = 0;

	// Setup
	pd3dImmediateContext->CSSetConstantBuffers(0, 1, &g_pcbSimulationConstants);
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pGridUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(0, 1, &g_pParticlesSRV);

	// Build Grid
	pd3dImmediateContext->CSSetShader(g_pBuildGridCS, NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles3D / SIMULATION_BLOCK_SIZE, 1, 1);

	// Sort Grid
	GPUSort(pd3dImmediateContext, g_pGridUAV, g_pGridSRV, g_pGridPingPongUAV, g_pGridPingPongSRV);

	// Setup
	pd3dImmediateContext->CSSetConstantBuffers(0, 1, &g_pcbSimulationConstants);
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pGridIndicesUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(3, 1, &g_pGridSRV);

	// Build Grid Indices
	pd3dImmediateContext->CSSetShader(g_pClearGridIndicesCS, NULL, 0);
	pd3dImmediateContext->Dispatch(NUM_GRID_INDICES / SIMULATION_BLOCK_SIZE, 1, 1);
	pd3dImmediateContext->CSSetShader(g_pBuildGridIndicesCS, NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles3D / SIMULATION_BLOCK_SIZE, 1, 1);

	// Setup
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pSortedParticlesUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(0, 1, &g_pParticlesSRV);
	pd3dImmediateContext->CSSetShaderResources(3, 1, &g_pGridSRV);

	// Rearrange
	pd3dImmediateContext->CSSetShader(g_pRearrangeParticlesCS, NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles3D / SIMULATION_BLOCK_SIZE, 1, 1);

	// Setup
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pNullUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(0, 1, &g_pNullSRV);
	pd3dImmediateContext->CSSetShaderResources(0, 1, &g_pSortedParticlesSRV);
	pd3dImmediateContext->CSSetShaderResources(3, 1, &g_pGridSRV);
	pd3dImmediateContext->CSSetShaderResources(4, 1, &g_pGridIndicesSRV);

	// Density
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pParticleDensityUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShader(g_pDensity_GridCS, NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles3D / SIMULATION_BLOCK_SIZE, 1, 1);

	// Force
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pParticleForcesUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(1, 1, &g_pParticleDensitySRV);
	pd3dImmediateContext->CSSetShader(g_pForce_GridCS, NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles3D / SIMULATION_BLOCK_SIZE, 1, 1);

	// Integrate
	CBPlanes pData2 = {};
	pData2.vPlanes[0] = g_vPlanes[0];
	pData2.vPlanes[1] = g_vPlanes[1];
	pData2.vPlanes[2] = g_vPlanes[2];
	pData2.vPlanes[3] = g_vPlanes[3];
	pData2.vPlanes[4] = g_vPlanes[4];
	pData2.vPlanes[5] = g_vPlanes[5];
	pd3dImmediateContext->UpdateSubresource(g_pcbPlanes, 0, NULL, &pData2, 0, 0);
	pd3dImmediateContext->CSSetConstantBuffers(1, 1, &g_pcbPlanes);

	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pParticlesUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(2, 1, &g_pParticleForcesSRV);
	pd3dImmediateContext->CSSetShader(g_pIntegrateCS, NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles3D / SIMULATION_BLOCK_SIZE, 1, 1);

}

//--------------------------------------------------------------------------------------
// GPU Fluid Simulation
//--------------------------------------------------------------------------------------
void Fluid3D::SimulateFluid(ID3D11DeviceContext* pd3dImmediateContext, float fElapsedTime)
{
	UINT UAVInitialCounts = 0;

	// Update per-frame variables
	CBTest pData = {};

	// Simulation Constants
	pData.iNumParticles = g_iNumParticles3D;
	// Clamp the time step when the simulation runs slowly to prevent numerical explosion
	pData.fTimeStep = min(g_fMaxAllowableTimeStep3D, fElapsedTime);
	pData.fSmoothlen = g_fSmoothlen3D;
	pData.fPressureStiffness = g_fPressureStiffness3D;
	pData.fRestDensity = g_fRestDensity3D;
	pData.fDensityCoef = g_fParticleMass3D * 315.0f / (64.0f * XM_PI * pow(g_fSmoothlen3D, 9));
	pData.fGradPressureCoef = g_fParticleMass3D * -45.0f / (XM_PI * pow(g_fSmoothlen3D, 6));
	pData.fLapViscosityCoef = g_fParticleMass3D * g_fViscosity3D * 45.0f / (XM_PI * pow(g_fSmoothlen3D, 6));

	pData.vGravity = g_vGravity3D;
	pData.originPos = XMFLOAT4(0.0f, 0.0f, 0.0f,0.0f);
	//pData.padding = XMFLOAT3(0.0f, 0.0f, 0.0f);
	// Cells are spaced the size of the smoothing length search radius
	// That way we only need to search the 8 adjacent cells + current cell
	//-------------------
	//|     |     |     |    
	//|     |     |     |       
	//-------------------
	//|     |     |     | 
	//|     |     |     |       
	//-------------------
	//|     |     |     | 
	//|     |     |     |       
	//-------------------
	//float cellSize = 2 * g_fSmoothlen3D / g_fInitialParticleSpacing3D;
	//float unitSize = 2 * g_fSmoothlen3D;
	pData.vGridDim.x = pData.vGridDim.y = pData.vGridDim.z = g_fSmoothlen3D;

	// Collision information for the map
	pData.fWallStiffness = g_fWallStiffness3D;
	pData.vGridSize.x = pData.vGridSize.y = pData.vGridSize.z = GRID_SIZE;
	pData.fParticleRadius = g_fParticleRadius * 2;

	pd3dImmediateContext->UpdateSubresource(g_pcbSimulationConstants, 0, NULL, &pData, 0, 0);


	SimulateFluid_Grid(pd3dImmediateContext);

	//SimulateFluid_Simple(pd3dImmediateContext);
	//switch (g_eSimMode) {
	//	// Simple N^2 Algorithm
	//case SIM_MODE_SIMPLE:
	//	SimulateFluid_Simple(pd3dImmediateContext);
	//	break;

	//	// Optimized N^2 Algorithm using Shared Memory
	//case SIM_MODE_SHARED:
	//	SimulateFluid_Shared(pd3dImmediateContext);
	//	break;

	//	// Optimized Grid + Sort Algorithm
	//case SIM_MODE_GRID:
	//	SimulateFluid_Grid(pd3dImmediateContext);
	//	break;
	//}

	// Unset
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pNullUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(0, 1, &g_pNullSRV);
	pd3dImmediateContext->CSSetShaderResources(1, 1, &g_pNullSRV);
	pd3dImmediateContext->CSSetShaderResources(2, 1, &g_pNullSRV);
	pd3dImmediateContext->CSSetShaderResources(3, 1, &g_pNullSRV);
	pd3dImmediateContext->CSSetShaderResources(4, 1, &g_pNullSRV);


}

//--------------------------------------------------------------------------------------
// GPU Fluid Simulation - Simple N^2 Algorithm
//--------------------------------------------------------------------------------------
void Fluid3D::SimulateFluid_Simple(ID3D11DeviceContext* pd3dImmediateContext)
{
	UINT UAVInitialCounts = 0;

	// Setup
	pd3dImmediateContext->CSSetConstantBuffers(0, 1, &g_pcbSimulationConstants);
	pd3dImmediateContext->CSSetShaderResources(0, 1, &g_pParticlesSRV);

	// Density
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pParticleDensityUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShader(g_pDensity_SimpleCS, NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles3D / SIMULATION_BLOCK_SIZE, 1, 1);

	// Force
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pParticleForcesUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(1, 1, &g_pParticleDensitySRV);
	pd3dImmediateContext->CSSetShader(g_pForce_SimpleCS, NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles3D / SIMULATION_BLOCK_SIZE, 1, 1);

	// Integrate
	CBPlanes pData2 = {};
	pData2.vPlanes[0] = g_vPlanes[0];
	pData2.vPlanes[1] = g_vPlanes[1];
	pData2.vPlanes[2] = g_vPlanes[2];
	pData2.vPlanes[3] = g_vPlanes[3];
	pData2.vPlanes[4] = g_vPlanes[4];
	pData2.vPlanes[5] = g_vPlanes[5];
	pd3dImmediateContext->UpdateSubresource(g_pcbPlanes, 0, NULL, &pData2, 0, 0);
	pd3dImmediateContext->CSSetConstantBuffers(1, 1, &g_pcbPlanes);
	pd3dImmediateContext->CSSetConstantBuffers(0, 1, &g_pcbSimulationConstants);

	pd3dImmediateContext->CopyResource(g_pSortedParticles, g_pParticles);
	pd3dImmediateContext->CSSetShaderResources(0, 1, &g_pSortedParticlesSRV);
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pParticlesUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(2, 1, &g_pParticleForcesSRV);
	pd3dImmediateContext->CSSetShader(g_pIntegrateCS, NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles3D / SIMULATION_BLOCK_SIZE, 1, 1);

}


//--------------------------------------------------------------------------------------
// GPU Fluid Rendering
//--------------------------------------------------------------------------------------
void  Fluid3D::RenderFluid(ID3D11DeviceContext* pd3dImmediateContext, float fElapsedTime)
{
	// Simple orthographic projection to display the entire map
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX world = XMMatrixIdentity();
	XMMATRIX mViewProjection = world * view * proj;

	// Update Constants
	CBRenderConstants pData = {};

	XMStoreFloat4x4(&pData.mViewProjection, XMMatrixTranspose(mViewProjection));
	pData.fParticleSize = g_fParticleRenderSize3D;

	pd3dImmediateContext->UpdateSubresource(g_pcbRenderConstants, 0, NULL, &pData, 0, 0);

	// Set the shaders
	pd3dImmediateContext->VSSetShader(g_pParticlesVS, NULL, 0);
	pd3dImmediateContext->GSSetShader(g_pParticlesGS, NULL, 0);
	pd3dImmediateContext->PSSetShader(g_pParticlesPS, NULL, 0);

	// Set the constant buffers
	pd3dImmediateContext->VSSetConstantBuffers(0, 1, &g_pcbRenderConstants);
	pd3dImmediateContext->GSSetConstantBuffers(0, 1, &g_pcbRenderConstants);

	// Setup the particles buffer and IA
	pd3dImmediateContext->VSSetShaderResources(0, 1, &g_pParticlesSRV);
	pd3dImmediateContext->VSSetShaderResources(1, 1, &g_pParticleDensitySRV);
	pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_pNullBuffer, &g_iNullUINT, &g_iNullUINT);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	// Draw the mesh
	pd3dImmediateContext->Draw(g_iNumParticles3D, 0);

	// Unset the particles buffer
	pd3dImmediateContext->VSSetShaderResources(0, 1, &g_pNullSRV);
	pd3dImmediateContext->VSSetShaderResources(1, 1, &g_pNullSRV);
}

void Fluid3D::UpdateInput(float dt)
{
	//get mouse state
	DirectX::Mouse::State mouseState = m_pMouse->GetState();
	DirectX::Mouse::State lastMouseState = m_MouseTracker.GetLastState();
	//get keyboard state
	DirectX::Keyboard::State keyState = m_pKeyboard->GetState();
	DirectX::Keyboard::State lastKeyState = m_KeyboardTracker.GetLastState();

	//update mousestate
	m_MouseTracker.Update(mouseState);
	m_KeyboardTracker.Update(keyState);
	m_KeyboardTracker.Update(keyState);
	if (mouseState.leftButton == true && m_MouseTracker.leftButton == m_MouseTracker.HELD)
	{
		mTheta -= (mouseState.x - lastMouseState.x) * 0.01f;
		mPhi -= (mouseState.y - lastMouseState.y) * 0.01f;
	}
	if (keyState.IsKeyDown(DirectX::Keyboard::W))
		mPhi += dt * 2;
	if (keyState.IsKeyDown(DirectX::Keyboard::S))
		mPhi -= dt * 2;
	if (keyState.IsKeyDown(DirectX::Keyboard::A))
		mTheta += dt * 2;
	if (keyState.IsKeyDown(DirectX::Keyboard::D))
		mTheta -= dt * 2;

}


void Fluid3D::BuildGeometryBuffers()
{
	// Create vertex buffer
	Vertex vertices[] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), (const float*)&Colors::White   },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), (const float*)&Colors::Black   },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), (const float*)&Colors::Red     },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), (const float*)&Colors::Green   },
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), (const float*)&Colors::Blue    },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), (const float*)&Colors::Yellow  },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), (const float*)&Colors::Cyan    },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), (const float*)&Colors::Magenta }
	};

	for (int i = 0; i <= 7; i++)
	{
		vertices[i].Pos.x *= 0.5;
		vertices[i].Pos.y *= 0.5;
		vertices[i].Pos.z *= 0.5;

	}
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof(Vertex) * 8;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = vertices;
	HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB));


	// Create the index buffer

	UINT indices[] = {
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.ByteWidth = sizeof(UINT) * 36;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = indices;
	HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));
}

void Fluid3D::BuildFX()
{
	DWORD shaderFlags = 0;
#if defined( DEBUG ) || defined( _DEBUG )
	shaderFlags |= D3D10_SHADER_DEBUG;
	shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif

	ID3D10Blob* vertexShaderBuffer = 0;
	ID3D10Blob* pixelShaderBuffer = 0;
	ID3D10Blob* compilationMsgs = 0;
	HRESULT hr = D3DX11CompileFromFile(L".\\shader\\color.hlsl", 0, 0, "VS", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS,
		0, 0, &vertexShaderBuffer, &compilationMsgs, 0);

	// compilationMsgs can store errors or warnings.
	if (compilationMsgs != 0)
	{
		MessageBoxA(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
		ReleaseCOM(compilationMsgs);
	}

	// Even if there are no compilationMsgs, check to make sure there were no other errors.
	if (FAILED(hr))
	{
		DXTrace(__FILE__, (DWORD)__LINE__, hr, L"D3DX11CompileFromFile", true);
	}
	compilationMsgs = 0;
	hr = D3DX11CompileFromFile(L".\\shader\\color.hlsl", 0, 0, "PS", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS,
		0, 0, &pixelShaderBuffer, &compilationMsgs, 0);

	// compilationMsgs can store errors or warnings.
	if (compilationMsgs != 0)
	{
		MessageBoxA(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
		ReleaseCOM(compilationMsgs);
	}

	// Even if there are no compilationMsgs, check to make sure there were no other errors.
	if (FAILED(hr))
	{
		DXTrace(__FILE__, (DWORD)__LINE__, hr, L"D3DX11CompileFromFile", true);
	}

	HR(md3dDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(),
		0, &mVertexShader));

	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	// Create the input layout
	unsigned int numElements = sizeof(vertexDesc) / sizeof(vertexDesc[0]);
	HR(md3dDevice->CreateInputLayout(vertexDesc, numElements, vertexShaderBuffer->GetBufferPointer(),
		vertexShaderBuffer->GetBufferSize(), &mInputLayout));

	// Done with compiled shader.
	ReleaseCOM(vertexShaderBuffer);

	HR(md3dDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(),
		0, &mPixelShader));

	// Done with compiled shader.
	ReleaseCOM(pixelShaderBuffer);
}

void Fluid3D::BuildConstantBuffer()
{
	D3D11_BUFFER_DESC matrixBufferDesc;

	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;

	HR(md3dDevice->CreateBuffer(&matrixBufferDesc, NULL, &mMatrixBuffer));
}

void Fluid3D::SetShaderParameters()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* dataPtr;
	// Set constants
	//XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj =  view * proj;

	HR(md3dDeviceContext->Map(mMatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));

	dataPtr = (MatrixBufferType*)mappedResource.pData;
	dataPtr->mvp = worldViewProj;

	md3dDeviceContext->Unmap(mMatrixBuffer, 0);

	md3dDeviceContext->VSSetConstantBuffers(0, 1, &mMatrixBuffer);

}

void Fluid3D::DrawBox()
{
	SetShaderParameters();
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	md3dDeviceContext->IASetVertexBuffers(0, 1, &mBoxVB, &stride, &offset);
	md3dDeviceContext->IASetIndexBuffer(mBoxIB, DXGI_FORMAT_R32_UINT, 0);

	md3dDeviceContext->IASetInputLayout(mInputLayout);
	md3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	md3dDeviceContext->VSSetShader(mVertexShader, NULL, 0);
	md3dDeviceContext->PSSetShader(mPixelShader, NULL, 0);
	md3dDeviceContext->DrawIndexed(36, 0, 0);
}