﻿#include "Fluid3DInPBF.h"
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
UINT g_iNumParticles = NUM_PARTICLES_16K;   //default use particle number

//Particle Properties
float g_fSmoothlen = 2.0f;                  //for test
float g_fPressureStiffness = 100.25f;
float g_fRestDensity = 1.5f;
float g_fParticleMass = 0.75f;
float g_fViscosity = 1.0f;                     //

float g_fMaxAllowableTimeStep = 0.005f;        //
float g_fParticleRenderSize = 0.4f;          //the render size of particle
float g_fParticleRadius2 = 1.0f;            //the size of partucle

//Gravity Directions
const XMFLOAT3 GRAVITY_DOWN(0, -9.8f, 0.0f);
const XMFLOAT3 GRAVITY_UP(0.0f, 9.8f, 0.0f);
const XMFLOAT3 GRAVITY_LEFT(-9.8f, 0.0f, 0.0f);
const XMFLOAT3 GRAVITY_RIGHT(9.8f, 0.0f, 0.0f);
XMFLOAT3 g_vGravity = GRAVITY_DOWN;  //default gracity direction



//Map Wall Collision Planes
float g_fWallStiffness = 3000.0f;
//float g_boundaryDampening = 256.0f;
//float g_speedLimiting = 200.0f;
//float g_fWallStiffness3D = 0.7f;


//Map Size
//These values should not be larger than GRID_SIZE * fSmothlen[the volumn of simulation space]
//Since the map must be divided up into fSmoothlen sized grid cells
//And the grid cell is used as a 32-bit sort key,32-bits for x ,y and z
float g_fMapHeight = GRID_SIZE;
float g_fMapWidth = GRID_SIZE;
float g_fMapLength = GRID_SIZE;
XMFLOAT4 g_vPlanes2[6] = {
	XMFLOAT4(1,0,0,-g_fParticleRadius2),
	XMFLOAT4(0,1,0,-g_fParticleRadius2),
	XMFLOAT4(0,0,1,-g_fParticleRadius2),
	XMFLOAT4(-1,0,0,g_fMapWidth + g_fParticleRadius2),
	XMFLOAT4(0,-1,0,g_fMapHeight + g_fParticleRadius2),
	XMFLOAT4(0,0,-1,g_fMapLength + g_fParticleRadius2)
};


 FluidPBF:: FluidPBF(HINSTANCE hInstance)
	:
	 D3DApp(hInstance), 
	 m_CameraMode(CameraMode::ThirdPerson)

{
	mMainWndCaption = L"Fluid 3D Demo";
	mEnable4xMsaa = false;
	g_iNullUINT = 0;         //helper to clear buffers

}

 FluidPBF::~FluidPBF()
{
	
}

bool  FluidPBF::Init()
{
	if (!D3DApp::Init())
		return false;

	InitCamera();
	
	CreateSimulationBuffers();
	BuildShader();
	return true;
}

void  FluidPBF::OnResize()
{
	D3DApp::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	if (m_pCamera != nullptr)
	{
		m_pCamera->SetFrustum(XM_PI / 4, GetAspectRatio(), 1.0f, 1000.0f);
		m_pCamera->SetViewPort(0.0f, 0.0f, (float)mClientWidth, (float)mClientHeight);
	}
}

void  FluidPBF::UpdateScene(float dt)
{
	UpdateCamera(dt);
	
}

void  FluidPBF::DrawScene()
{
}

void  FluidPBF::DrawScene(float fElapsedTime)
{
	
	md3dDeviceContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Silver));
	md3dDeviceContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	SimulateFluid(md3dDeviceContext, fElapsedTime);

	RenderFluid(md3dDeviceContext, fElapsedTime);

	//imgui stuff
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//Imgui setting

	float  fDensityCoef = g_fParticleMass * 315.0f / (64.0f * XM_PI * pow(g_fSmoothlen, 9));
	float fGradPressureCoef = g_fParticleMass * -45.0f / (XM_PI * pow(g_fSmoothlen, 6));
	float fLapViscosityCoef = g_fParticleMass * g_fViscosity * 45.0f / (XM_PI * pow(g_fSmoothlen, 6));
	if (ImGui::Begin("Fluid Coff"))
	{
		ImGui::SliderFloat("Gravity", &g_vGravity.y, -100.0f, 10.0f);
		ImGui::SliderFloat("SmoothLen", &g_fSmoothlen, 0.0f, 10.0f);
		ImGui::SliderFloat("RestDensity", &g_fRestDensity, 800.0f, 1200.0f);
		ImGui::SliderFloat("ParticleMass", &g_fParticleMass, 0.0f, 10.0f);
		ImGui::SliderFloat("Viscosity", &g_fViscosity, 0.0f, 1.0f);
		ImGui::SliderFloat("ForceStiff", &g_fWallStiffness, 3000.0f, 5000.0f);
		ImGui::SliderFloat("sphereRadius", &g_fParticleRadius2, 0.5f, 1.0f);

		ImGui::Text("ParticleRenderSize:%0.10f", &fDensityCoef);
		ImGui::Text("GradPressureCoef %0.10f", &fGradPressureCoef); 		
		ImGui::Text("LapViscosityCoef  %0.10f", &fLapViscosityCoef);
	}
	ImGui::End();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	HR(mSwapChain->Present(0, 0));

}

HRESULT  FluidPBF::CreateSimulationBuffers()
{
	HRESULT hr = S_OK;

	// Destroy the old buffers in case the number of particles has changed
	SAFE_RELEASEComPtr(g_pParticles);
	SAFE_RELEASEComPtr(g_pParticlesSRV);
	SAFE_RELEASEComPtr(g_pParticlesUAV);

	SAFE_RELEASEComPtr(g_pSortedParticles);
	SAFE_RELEASEComPtr(g_pSortedParticlesSRV);
	SAFE_RELEASEComPtr(g_pSortedParticlesUAV);

	SAFE_RELEASEComPtr(g_pParticleForces);
	SAFE_RELEASEComPtr(g_pParticleForcesSRV);
	SAFE_RELEASEComPtr(g_pParticleForcesUAV);

	SAFE_RELEASEComPtr(g_pParticleDensity);
	SAFE_RELEASEComPtr(g_pParticleDensitySRV);
	SAFE_RELEASEComPtr(g_pParticleDensityUAV);

	SAFE_RELEASEComPtr(g_pGridSRV);
	SAFE_RELEASEComPtr(g_pGridUAV);
	SAFE_RELEASEComPtr(g_pGrid);

	SAFE_RELEASEComPtr(g_pGridPingPongSRV);
	SAFE_RELEASEComPtr(g_pGridPingPongUAV);
	SAFE_RELEASEComPtr(g_pGridPingPong);

	SAFE_RELEASEComPtr(g_pGridIndicesSRV);
	SAFE_RELEASEComPtr(g_pGridIndicesUAV);
	SAFE_RELEASEComPtr(g_pGridIndices);
	
	// Create the initial particle positions
	// This is only used to populate the GPU buffers on creation
	Particle* particles = new Particle[g_iNumParticles];
	ZeroMemory(particles, sizeof(Particle) * g_iNumParticles);
	float rndX, rndY, rndZ;
	std::mt19937 eng(std::random_device{}());
	float velRange = GRID_SIZE * 0.5f;
	std::uniform_real_distribution<float> dist(0.0, 30);   //intial fluid position

	for (UINT i = 0; i < g_iNumParticles; i++)
	{
		rndX = dist(eng);
		rndY = dist(eng);
		rndZ = dist(eng);

		particles[i].vPosition = XMFLOAT3(rndX, rndY, rndZ);
		particles[i].vVelocity = XMFLOAT3(0.0f, 0.0f, 0.0f);

	}

	// Create Structured Buffers
	HR(CreateStructuredBuffer< Particle >(md3dDevice, g_iNumParticles, g_pParticles.GetAddressOf(), g_pParticlesSRV.GetAddressOf(), g_pParticlesUAV.GetAddressOf(), particles));
	D3D11SetDebugObjectName(g_pParticles.Get(), "Particles");
	D3D11SetDebugObjectName(g_pParticlesSRV.Get(), "Particles SRV");
	D3D11SetDebugObjectName(g_pParticlesUAV.Get(), "Particles UAV");
	HR(CreateStructuredBuffer< Particle >(md3dDevice, g_iNumParticles, g_pSortedParticles.GetAddressOf(), g_pSortedParticlesSRV.GetAddressOf(), g_pSortedParticlesUAV.GetAddressOf(), particles));
	D3D11SetDebugObjectName(g_pSortedParticles.Get(), "Sorted");
	D3D11SetDebugObjectName(g_pSortedParticlesSRV.Get(), "Sorted SRV");
	D3D11SetDebugObjectName(g_pSortedParticlesUAV.Get(), "Sorted UAV");
	HR(CreateStructuredBuffer< ParticleForces >(md3dDevice, g_iNumParticles, g_pParticleForces.GetAddressOf(), g_pParticleForcesSRV.GetAddressOf(), g_pParticleForcesUAV.GetAddressOf()));
	D3D11SetDebugObjectName(g_pParticleForces.Get(), "Forces");
	D3D11SetDebugObjectName(g_pParticleForcesSRV.Get(), "Forces SRV");
	D3D11SetDebugObjectName(g_pParticleForcesUAV.Get(), "Forces UAV");
	HR(CreateStructuredBuffer< ParticleDensity >(md3dDevice, g_iNumParticles, g_pParticleDensity.GetAddressOf(), g_pParticleDensitySRV.GetAddressOf(), g_pParticleDensityUAV.GetAddressOf()));
	D3D11SetDebugObjectName(g_pParticleDensity.Get(), "Density");
	D3D11SetDebugObjectName(g_pParticleDensitySRV.Get(), "Density SRV");
	D3D11SetDebugObjectName(g_pParticleDensityUAV.Get(), "Density UAV");
	HR(CreateStructuredBuffer< UINT2 >(md3dDevice, g_iNumParticles, g_pGrid.GetAddressOf(), g_pGridSRV.GetAddressOf(), g_pGridUAV.GetAddressOf()));
	D3D11SetDebugObjectName(g_pGrid.Get(), "Grid");
	D3D11SetDebugObjectName(g_pGridSRV.Get(), "Grid SRV");
	D3D11SetDebugObjectName(g_pGridUAV.Get(), "Grid UAV");
	HR(CreateStructuredBuffer< UINT2 >(md3dDevice, g_iNumParticles, g_pGridPingPong.GetAddressOf(), g_pGridPingPongSRV.GetAddressOf(), g_pGridPingPongUAV.GetAddressOf()));
	D3D11SetDebugObjectName(g_pGridPingPong.Get(), "PingPong");
	D3D11SetDebugObjectName(g_pGridPingPongSRV.Get(), "PingPong SRV");
	D3D11SetDebugObjectName(g_pGridPingPongUAV.Get(), "PingPong UAV");
	HR(CreateStructuredBuffer< UINT2 >(md3dDevice, NUM_GRID_INDICES, g_pGridIndices.GetAddressOf(), g_pGridIndicesSRV.GetAddressOf(), g_pGridIndicesUAV.GetAddressOf()));
	D3D11SetDebugObjectName(g_pGridIndices.Get(), "Indices");
	D3D11SetDebugObjectName(g_pGridIndicesSRV.Get(), "Indices SRV");
	D3D11SetDebugObjectName(g_pGridIndicesUAV.Get(), "Indices UAV");
	delete[] particles;

	// Create Constant Buffers
	HR(CreateConstantBuffer< CBTest >(md3dDevice, g_pcbSimulationConstants.GetAddressOf()));
	HR(CreateConstantBuffer< CBPlanes >(md3dDevice, g_pcbPlanes.GetAddressOf()));
	HR(CreateConstantBuffer< CBRenderConstants >(md3dDevice, g_pcbRenderConstants.GetAddressOf()));
	HR(CreateConstantBuffer< SortCB >(md3dDevice, g_pSortCB.GetAddressOf()));

	D3D11SetDebugObjectName(g_pcbSimulationConstants.Get(), "SimluationCB");
	D3D11SetDebugObjectName(g_pcbPlanes.Get(), "Planes");
	D3D11SetDebugObjectName(g_pcbRenderConstants.Get(), "Render");
	D3D11SetDebugObjectName(g_pSortCB.Get(), "Sort");
	return S_OK;
}

//Compile the Shaders
void  FluidPBF::BuildShader()
{
	ID3DBlob* pBlob = NULL;
	//rendering shaders
	SafeCompileShaderFromFile(L".\\shader\\Fluid3DRender.hlsl", "ParticleVS", "vs_5_0", &pBlob);
	HR(md3dDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL,g_pParticlesVS.GetAddressOf()));
	D3D11SetDebugObjectName(g_pParticlesVS.Get(), "ParticlesVS");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\Fluid3DRender.hlsl", "ParticleGS", "gs_5_0", &pBlob);
	HR(md3dDevice->CreateGeometryShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, g_pParticlesGS.GetAddressOf()));
	D3D11SetDebugObjectName(g_pParticlesGS.Get(), "ParticlesGS");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\Fluid3DRender.hlsl", "ParticlePS", "ps_5_0", &pBlob);
	HR(md3dDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, g_pParticlesPS.GetAddressOf()));
	D3D11SetDebugObjectName(g_pParticlesPS.Get(), "ParticlesPS");
	SAFE_RELEASE(pBlob);

	// Compute Shaders
	const char* CSTarget = (md3dDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0) ? "cs_5_0" : "cs_4_0";
	SafeCompileShaderFromFile(L".\\shader\\Fluid3DTest.hlsl", "IntegrateCS", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, g_pIntegrateCS.GetAddressOf()));
	D3D11SetDebugObjectName(g_pIntegrateCS.Get(), "IntegrateCS");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\Fluid3DTest.hlsl", "DensityCS_Simple", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, g_pDensity_SimpleCS.GetAddressOf()));
	D3D11SetDebugObjectName(g_pDensity_SimpleCS.Get(), "DensityCS_Simple");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\Fluid3DTest.hlsl", "ForceCS_Simple", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, g_pForce_SimpleCS.GetAddressOf()));
	D3D11SetDebugObjectName(g_pForce_SimpleCS.Get(), "ForceCS_Simple");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\Fluid3DTest.hlsl", "DensityCS_Grid", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, g_pDensity_GridCS.GetAddressOf()));
	D3D11SetDebugObjectName(g_pDensity_GridCS.Get(), "DensityCS_Grid");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\Fluid3DTest.hlsl", "ForceCS_Grid", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, g_pForce_GridCS.GetAddressOf()));
	D3D11SetDebugObjectName(g_pForce_GridCS.Get(), "ForceCS_Grid");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\Fluid3DTest.hlsl", "BuildGridCS", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, g_pBuildGridCS.GetAddressOf()));
	D3D11SetDebugObjectName(g_pBuildGridCS.Get(), "BuildGridCS");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\Fluid3DTest.hlsl", "ClearGridIndicesCS", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, g_pClearGridIndicesCS.GetAddressOf()));
	D3D11SetDebugObjectName(g_pClearGridIndicesCS.Get(), "ClearGridIndicesCS");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\Fluid3DTest.hlsl", "BuildGridIndicesCS", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, g_pBuildGridIndicesCS.GetAddressOf()));
	D3D11SetDebugObjectName(g_pBuildGridIndicesCS.Get(), "BuildGridIndicesCS");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\Fluid3DTest.hlsl", "RearrangeParticlesCS", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, g_pRearrangeParticlesCS.GetAddressOf()));
	D3D11SetDebugObjectName(g_pRearrangeParticlesCS.Get(), "RearrangeParticlesCS");
	SAFE_RELEASE(pBlob);

	// Sort Shaders
	SafeCompileShaderFromFile(L".\\shader\\BitonicSortCS.hlsl", "BitonicSort", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, g_pSortBitonic.GetAddressOf()));
	D3D11SetDebugObjectName(g_pSortBitonic.Get(), "BitonicSort");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\BitonicSortCS.hlsl", "MatrixTranspose", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, g_pSortTranspose.GetAddressOf()));
	D3D11SetDebugObjectName(g_pSortTranspose.Get(), "MatrixTranspose");
	SAFE_RELEASE(pBlob);
}


void  FluidPBF::SafeCompileShaderFromFile(const WCHAR * fileName, LPCSTR enterPoint, LPCSTR shaderModel, ID3DBlob **ppBlob)
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
void  FluidPBF::GPUSort(ID3D11DeviceContext * pd3dImmediateContext,
	ID3D11UnorderedAccessView * inUAV,
	ID3D11ShaderResourceView * inSRV,
	ID3D11UnorderedAccessView * tempUAV,
	ID3D11ShaderResourceView * tempSRV)
{
	pd3dImmediateContext->CSSetConstantBuffers(0, 1, g_pSortCB.GetAddressOf());

	const UINT NUM_ELEMENTS = g_iNumParticles;
	const UINT MATRIX_WIDTH = BITONIC_BLOCK_SIZE;
	const UINT MATRIX_HEIGHT = NUM_ELEMENTS / BITONIC_BLOCK_SIZE;

	// Sort the data
   // First sort the rows for the levels <= to the block size
	for (UINT level = 2; level <= BITONIC_BLOCK_SIZE; level <<= 1)
	{
		SortCB constants = { level, level, MATRIX_HEIGHT, MATRIX_WIDTH };
		pd3dImmediateContext->UpdateSubresource(g_pSortCB.Get(), 0, NULL, &constants, 0, 0);

		// Sort the row data
		UINT UAVInitialCounts = 0;
		pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &inUAV, &UAVInitialCounts);
		pd3dImmediateContext->CSSetShader(g_pSortBitonic.Get(), NULL, 0);
		pd3dImmediateContext->Dispatch(NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1);
	}
	// Then sort the rows and columns for the levels > than the block size
   // Transpose. Sort the Columns. Transpose. Sort the Rows.
	for (UINT level = (BITONIC_BLOCK_SIZE << 1); level <= NUM_ELEMENTS; level <<= 1)
	{
		SortCB constants1 = { (level / BITONIC_BLOCK_SIZE), (level & ~NUM_ELEMENTS) / BITONIC_BLOCK_SIZE, MATRIX_WIDTH, MATRIX_HEIGHT };
		pd3dImmediateContext->UpdateSubresource(g_pSortCB.Get(), 0, NULL, &constants1, 0, 0);

		// Transpose the data from buffer 1 into buffer 2
		ID3D11ShaderResourceView* pViewNULL = NULL;
		UINT UAVInitialCounts = 0;
		pd3dImmediateContext->CSSetShaderResources(0, 1, &pViewNULL);
		pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &tempUAV, &UAVInitialCounts);
		pd3dImmediateContext->CSSetShaderResources(0, 1, &inSRV);
		pd3dImmediateContext->CSSetShader(g_pSortTranspose.Get(), NULL, 0);
		pd3dImmediateContext->Dispatch(MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE, MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE, 1);

		// Sort the transposed column data
		pd3dImmediateContext->CSSetShader(g_pSortBitonic.Get(), NULL, 0);
		pd3dImmediateContext->Dispatch(NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1);

		SortCB constants2 = { BITONIC_BLOCK_SIZE, level, MATRIX_HEIGHT, MATRIX_WIDTH };
		pd3dImmediateContext->UpdateSubresource(g_pSortCB.Get(), 0, NULL, &constants2, 0, 0);

		// Transpose the data from buffer 2 back into buffer 1
		pd3dImmediateContext->CSSetShaderResources(0, 1, &pViewNULL);
		pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &inUAV, &UAVInitialCounts);
		pd3dImmediateContext->CSSetShaderResources(0, 1, &tempSRV);
		pd3dImmediateContext->CSSetShader(g_pSortTranspose.Get(), NULL, 0);
		pd3dImmediateContext->Dispatch(MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE, MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE, 1);

		// Sort the row data
		pd3dImmediateContext->CSSetShader(g_pSortBitonic.Get(), NULL, 0);
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
void  FluidPBF::SimulateFluid_Grid(ID3D11DeviceContext* pd3dImmediateContext)
{
	UINT UAVInitialCounts = 0;

	// Setup
	pd3dImmediateContext->CSSetConstantBuffers(0, 1, g_pcbSimulationConstants.GetAddressOf());
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, g_pGridUAV.GetAddressOf() , &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(0, 1, g_pParticlesSRV.GetAddressOf());

	// Build Grid
	pd3dImmediateContext->CSSetShader(g_pBuildGridCS.Get(), NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1);

	// Sort Grid
	GPUSort(pd3dImmediateContext, g_pGridUAV.Get(), g_pGridSRV.Get(), 
		g_pGridPingPongUAV.Get(), g_pGridPingPongSRV.Get());

	// Setup
	pd3dImmediateContext->CSSetConstantBuffers(0, 1, g_pcbSimulationConstants.GetAddressOf());
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, g_pGridIndicesUAV.GetAddressOf(), &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(3, 1, g_pGridSRV.GetAddressOf());

	// Build Grid Indices
	pd3dImmediateContext->CSSetShader(g_pClearGridIndicesCS.Get(), NULL, 0);
	pd3dImmediateContext->Dispatch(NUM_GRID_INDICES / SIMULATION_BLOCK_SIZE, 1, 1);
	pd3dImmediateContext->CSSetShader(g_pBuildGridIndicesCS.Get(), NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1);

	// Setup
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1,g_pSortedParticlesUAV.GetAddressOf(), &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(0, 1,g_pParticlesSRV.GetAddressOf());
	pd3dImmediateContext->CSSetShaderResources(3, 1, g_pGridSRV.GetAddressOf());

	// Rearrange
	pd3dImmediateContext->CSSetShader(g_pRearrangeParticlesCS.Get(), NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1);

	// Setup
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pNullUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(0, 1, &g_pNullSRV);
	pd3dImmediateContext->CSSetShaderResources(0, 1, g_pSortedParticlesSRV.GetAddressOf());
	pd3dImmediateContext->CSSetShaderResources(3, 1, g_pGridSRV.GetAddressOf());
	pd3dImmediateContext->CSSetShaderResources(4, 1, g_pGridIndicesSRV.GetAddressOf());

	// Density
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, g_pParticleDensityUAV.GetAddressOf(), &UAVInitialCounts);
	pd3dImmediateContext->CSSetShader(g_pDensity_GridCS.Get(), NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1);

	// Force
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1,g_pParticleForcesUAV.GetAddressOf(), &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(1, 1, g_pParticleDensitySRV.GetAddressOf());
	pd3dImmediateContext->CSSetShader(g_pForce_GridCS.Get(), NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1);

	// Integrate
	CBPlanes pData2 = {};
	pData2.vPlanes[0] = g_vPlanes2[0];
	pData2.vPlanes[1] = g_vPlanes2[1];
	pData2.vPlanes[2] = g_vPlanes2[2];
	pData2.vPlanes[3] = g_vPlanes2[3];
	pData2.vPlanes[4] = g_vPlanes2[4];
	pData2.vPlanes[5] = g_vPlanes2[5];
	pd3dImmediateContext->UpdateSubresource(g_pcbPlanes.Get(), 0, NULL, &pData2, 0, 0);
	pd3dImmediateContext->CSSetConstantBuffers(1, 1, g_pcbPlanes.GetAddressOf());

	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, g_pParticlesUAV.GetAddressOf(), &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(2, 1, g_pParticleForcesSRV.GetAddressOf());
	pd3dImmediateContext->CSSetShader(g_pIntegrateCS.Get(), NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1);

}

//--------------------------------------------------------------------------------------
// GPU Fluid Simulation
//--------------------------------------------------------------------------------------
void  FluidPBF::SimulateFluid(ID3D11DeviceContext* pd3dImmediateContext, float fElapsedTime)
{
	UINT UAVInitialCounts = 0;

	// Update per-frame variables
	CBTest pData = {};

	// Simulation Constants
	pData.iNumParticles = g_iNumParticles;
	// Clamp the time step when the simulation runs slowly to prevent numerical explosion
	pData.fTimeStep = min(g_fMaxAllowableTimeStep, fElapsedTime);
	pData.fSmoothlen = g_fSmoothlen;
	pData.fPressureStiffness = g_fPressureStiffness;
	pData.fRestDensity = g_fRestDensity;
	pData.fDensityCoef = g_fParticleMass * 315.0f / (64.0f * XM_PI * pow(g_fSmoothlen, 9));
	pData.fGradPressureCoef = g_fParticleMass * -45.0f / (XM_PI * pow(g_fSmoothlen, 6));
	pData.fLapViscosityCoef = g_fParticleMass * g_fViscosity * 45.0f / (XM_PI * pow(g_fSmoothlen, 6));

	pData.vGravity = g_vGravity;
	pData.originPos = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
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
	pData.vGridDim.x = pData.vGridDim.y = pData.vGridDim.z = 1 / g_fSmoothlen;

	// Collision information for the map
	pData.fWallStiffness = g_fWallStiffness;
	pData.vGridSize.x = pData.vGridSize.y = pData.vGridSize.z = GRID_SIZE;
	pData.fParticleRadius = g_fParticleRadius2 * 2;     //the diamater of sphere use for collision later

	pd3dImmediateContext->UpdateSubresource(g_pcbSimulationConstants.Get(), 0, NULL, &pData, 0, 0);


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
// GPU Fluid Rendering
//--------------------------------------------------------------------------------------
void   FluidPBF::RenderFluid(ID3D11DeviceContext* pd3dImmediateContext, float fElapsedTime)
{
	// Simple orthographic projection to display the entire map
	XMMATRIX view = m_pCamera->GetViewXM(); //XMLoadFloat4x4(&mView);// 
	XMMATRIX proj = m_pCamera->GetProjXM();//XMLoadFloat4x4(&mProj);// 
	XMMATRIX world = XMMatrixIdentity();
	XMMATRIX mViewProjection = world * view * proj;

	// Update Constants
	CBRenderConstants pData = {};

	XMStoreFloat4x4(&pData.mViewProjection, XMMatrixTranspose(mViewProjection));
	pData.fParticleSize = g_fParticleRenderSize;

	pd3dImmediateContext->UpdateSubresource(g_pcbRenderConstants.Get(), 0, NULL, &pData, 0, 0);

	// Set the shaders
	pd3dImmediateContext->VSSetShader(g_pParticlesVS.Get(), NULL, 0);
	pd3dImmediateContext->GSSetShader(g_pParticlesGS.Get(), NULL, 0);
	pd3dImmediateContext->PSSetShader(g_pParticlesPS.Get(), NULL, 0);

	// Set the constant buffers
//	pd3dImmediateContext->VSSetConstantBuffers(0, 1, g_pcbRenderConstants.GetAddressOf());
	pd3dImmediateContext->GSSetConstantBuffers(0, 1, g_pcbRenderConstants.GetAddressOf());

	// Setup the particles buffer and IA
	pd3dImmediateContext->VSSetShaderResources(0, 1, g_pParticlesSRV.GetAddressOf());
	pd3dImmediateContext->VSSetShaderResources(1, 1, g_pParticleDensitySRV.GetAddressOf());
	pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_pNullBuffer, &g_iNullUINT, &g_iNullUINT);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	// Draw the mesh
	pd3dImmediateContext->Draw(g_iNumParticles, 0);

	// Unset the particles buffer
	pd3dImmediateContext->VSSetShaderResources(0, 1, &g_pNullSRV);
	pd3dImmediateContext->VSSetShaderResources(1, 1, &g_pNullSRV);
}

void FluidPBF::InitCamera()
{
	m_CameraMode = CameraMode::ThirdPerson;
	auto camera = std::shared_ptr<ThirdPersonCamera>(new ThirdPersonCamera);
	m_pCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)mClientWidth, (float)mClientHeight);
	md3dDeviceContext->RSSetViewports(1, &m_pCamera->GetViewPort());

	// init the projection
	m_pCamera->SetFrustum(XM_PI / 4, GetAspectRatio(), 1.0f, 1000.0f);

	XMFLOAT3 target =XMFLOAT3(0.0f,0.0f,0.0f);
	camera->SetTarget(target);
	camera->SetDistance(150.0f);
	camera->SetDistanceMinMax(10.0f, 200.0f);
}

void FluidPBF::UpdateCamera(float dt)
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

	auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(m_pCamera);
	auto cam3rd = std::dynamic_pointer_cast<ThirdPersonCamera>(m_pCamera);

	if (m_CameraMode == CameraMode::FirstPerson || m_CameraMode == CameraMode::Free)
	{
		// Firstperson/free camera operate

		// direction move
		if (keyState.IsKeyDown(DirectX::Keyboard::W))
		{
			if (m_CameraMode == CameraMode::FirstPerson)
				cam1st->Walk(dt * 3.0f);
			else
				cam1st->MoveForward(dt * 3.0f);
		}
		if (keyState.IsKeyDown(DirectX::Keyboard::S))
		{
			if (m_CameraMode == CameraMode::FirstPerson)
				cam1st->Walk(dt * -3.0f);
			else
				cam1st->MoveForward(dt * -3.0f);
		}
		if (keyState.IsKeyDown(DirectX::Keyboard::A))
			cam1st->Strafe(dt * -3.0f);
		if (keyState.IsKeyDown(DirectX::Keyboard::D))
			cam1st->Strafe(dt * 3.0f);

		//// limited in  [-8.9f, 8.9f]
		//XMFLOAT3 adjustedPos;
		//XMStoreFloat3(&adjustedPos, XMVectorClamp(cam1st->GetPositionXM(), XMVectorSet(-150.0f, 0.0f, -8.9f, 0.0f), XMVectorReplicate(8.9f)));
		//cam1st->SetPosition(adjustedPos);

		// rotation
		cam1st->Pitch(mouseState.y * dt * 1.25f);
		cam1st->RotateY(mouseState.x * dt * 1.25f);
	}
	// ThirdPerson camera 
	else if (m_CameraMode == CameraMode::ThirdPerson)
	{
		
		/// rotation
		if (mouseState.leftButton == true && m_MouseTracker.leftButton == m_MouseTracker.HELD)
		{
			cam3rd->RotateX((mouseState.y - lastMouseState.y) * dt * 1.25f);
			cam3rd->RotateY((mouseState.x - lastMouseState.x) * dt * 1.25f);
		}
		cam3rd->Approach(-mouseState.scrollWheelValue / 60 * 1.0f);
	}

	// updateview matrix
	m_pCamera->UpdateViewMatrix();
	// reset mouse rollwheel value
	m_pMouse->ResetScrollWheelValue();

	// change cameramodel
	if (m_KeyboardTracker.IsKeyPressed(DirectX::Keyboard::D1) && m_CameraMode != CameraMode::FirstPerson)
	{
		if (!cam1st)
		{
			cam1st.reset(new FirstPersonCamera);
			cam1st->SetFrustum(XM_PI / 4, GetAspectRatio(), 1.0f, 1000.0f);
			m_pCamera = cam1st;
		}
		XMFLOAT3 targetPos = XMFLOAT3(0.0f, 0.0f, 0.0f);
		cam1st->LookTo(targetPos,
			XMFLOAT3(0.0f, 0.0f, 1.0f),
			XMFLOAT3(0.0f, 1.0f, 0.0f));

		m_CameraMode = CameraMode::FirstPerson;
	}
	else if (m_KeyboardTracker.IsKeyPressed(DirectX::Keyboard::D2) && m_CameraMode != CameraMode::ThirdPerson)
	{
		if (!cam3rd)
		{
			cam3rd.reset(new ThirdPersonCamera);
			cam3rd->SetFrustum(XM_PI / 4, GetAspectRatio(), 0.5f, 1000.0f);
			m_pCamera = cam3rd;
		}
		XMFLOAT3 target = XMFLOAT3(0.0f, 0.0f, 0.0f);
		cam3rd->SetTarget(target);
		cam3rd->SetDistance(8.0f);
		cam3rd->SetDistanceMinMax(3.0f, 20.0f);

		m_CameraMode = CameraMode::ThirdPerson;
	}
	else if (m_KeyboardTracker.IsKeyPressed(DirectX::Keyboard::D3) && m_CameraMode != CameraMode::Free)
	{
		if (!cam1st)
		{
			cam1st.reset(new FirstPersonCamera);
			cam1st->SetFrustum(XM_PI / 4, GetAspectRatio(), 1.0f, 1000.0f);
			m_pCamera = cam1st;
		}
		
		XMFLOAT3 pos = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 to = XMFLOAT3(0.0f, 0.0f, 1.0f);
		XMFLOAT3 up = XMFLOAT3(0.0f, 1.0f, 0.0f);
		pos.y += 3;
		cam1st->LookTo(pos, to, up);

		m_CameraMode = CameraMode::Free;
	}
}