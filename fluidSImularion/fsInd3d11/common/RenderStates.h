#pragma once

#include <wrl/client.h>
#include <d3d11.h>

class RenderStates
{
public:
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	static bool IsInit();

	static void InitAll(ID3D11Device * device);

public:
	static ComPtr<ID3D11RasterizerState> RSWireframe;		    // RaseterState:Fwireframe
	static ComPtr<ID3D11RasterizerState> RSNoCull;			    // RaseterState:Fno back cull
	static ComPtr<ID3D11RasterizerState> RSCullClockWise;	    // RaseterState:FCullClockWise
	static ComPtr<ID3D11RasterizerState> RSDefault;	    // RaseterState:FCullClockWise

	static ComPtr<ID3D11SamplerState> SSLinearWrap;			    // SamplerState:FLinearWrap
	static ComPtr<ID3D11SamplerState> SSAnistropicWrap;		    // SamplerState:FAnistropicWrap
	static ComPtr<ID3D11SamplerState> SSPointClamped;

	static ComPtr<ID3D11BlendState> BSNoColorWrite;		        // BlendeState:FNoColorWrite
	static ComPtr<ID3D11BlendState> BSTransparent;		        // BlendeState:FTransparent
	static ComPtr<ID3D11BlendState> BSAlphaToCoverage;	        // BlendeState:FAlpha-To-Coverage
	static ComPtr<ID3D11BlendState> BSDefault;	        // BlendeState:FAlpha-To-Coverage

	static ComPtr<ID3D11DepthStencilState> DSSWriteStencil;		// depth/stenilState:FWriteStencil
	static ComPtr<ID3D11DepthStencilState> DSSDrawWithStencil;	// depth/stenilState:FDrawWithStencil
	static ComPtr<ID3D11DepthStencilState> DSSNoDoubleBlend;	// depth/stenilState:FNoDoubleBlend
	static ComPtr<ID3D11DepthStencilState> DSSNoDepthTest;		// depth/stenilState:FNoDepthTest
	static ComPtr<ID3D11DepthStencilState> DSSNoDepthWrite;		// depth/stenilState:FNoDepthWrite
	static ComPtr<ID3D11DepthStencilState> DSSDepthDisabledStencilUse;
};