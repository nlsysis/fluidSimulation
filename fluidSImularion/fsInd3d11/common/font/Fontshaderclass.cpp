#include "Fontshaderclass.h"

FontShaderClass::FontShaderClass()
	:
	m_vertexShader(nullptr),
	m_pixelShader(nullptr),
	m_layout(nullptr),
	m_constantBuffer(nullptr),
	m_sampleState(nullptr),
	m_pixelBuffer(nullptr)
{
}

FontShaderClass::FontShaderClass(const FontShaderClass &)
{
}

FontShaderClass::~FontShaderClass()
{
}

bool FontShaderClass::Initialize(ID3D11Device * device,HWND hwnd)
{
	return InitializeShader(device, hwnd, L".\\shader\\font.hlsl", L".\\shader\\font.hlsl");
}
/**********************************************
	*@brief calls ShutdownShader function
***********************************************/
void FontShaderClass::Shutdown()
{
	ShutdownShader();
}
/**********************************************
	*@brief set shader patameters
***********************************************/
bool FontShaderClass::Render(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix,
	XMMATRIX projectionMatrix, ID3D11ShaderResourceView* texture, D3DXVECTOR4 pixelcolor)
{
	bool result;
	result = SetShaderParmeters(deviceContext, worldMatrix, viewMatrix, projectionMatrix, texture, pixelcolor);
	if (!result) return false;
	
	//render the prepared buffers with the shader
	RenderShader(deviceContext,indexCount);
	return true;
}
/**********************************************
	*@brief loads the new HLSL font vertex
***********************************************/
bool FontShaderClass::InitializeShader(ID3D11Device *device, HWND hwnd,const WCHAR *vsFilename, const WCHAR *psFilename)
{
	HRESULT result;
	ID3D10Blob* errorMessage;
	ID3D10Blob* vertexShaderBuffer;
	ID3D10Blob* pixelShaderBuffer;
	D3D11_INPUT_ELEMENT_DESC polygonLayout[2];
	unsigned int numElements;
	D3D11_BUFFER_DESC constantBufferDesc;
	D3D11_SAMPLER_DESC samplerDesc;
	D3D11_BUFFER_DESC pixelBufferDesc;

	errorMessage = 0;
	vertexShaderBuffer = 0;
	pixelShaderBuffer = 0;

	result = D3DX11CompileFromFile(vsFilename, NULL, NULL, "FontVertexShader", "vs_5_0",
		D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL, &vertexShaderBuffer, &errorMessage, NULL);
	if (FAILED(result))
	{
		if (errorMessage)
		{
			OutputShaderErrorMessage(errorMessage,hwnd,vsFilename);
		}
		else
		{
			MessageBox(hwnd,vsFilename,L"Missing Shader File",MB_OK);
		}
		return false;
	}

	///compile the pixel shader code
	result = D3DX11CompileFromFile(psFilename,NULL,NULL,"FontPixelShader","ps_5_0",D3D10_SHADER_ENABLE_STRICTNESS,0,NULL,
		&pixelShaderBuffer,&errorMessage,NULL);
	if (FAILED(result))
	{
		if (errorMessage)
		{
			OutputShaderErrorMessage(errorMessage, hwnd, psFilename);
		}
		else
		{
			MessageBox(hwnd,psFilename,L"Missing Shader File",MB_OK);
		}
		return false;
	}
	///create the vertex shader from the buffer
	result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(),vertexShaderBuffer->GetBufferSize(),NULL,&m_vertexShader);
	if (FAILED(result))
	{
		return false;
	}
	///create the pixel shader from the buffer
	result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_pixelShader);
	if (FAILED(result))
	{
		return false;
	}
	//create the vertex input layout description
	polygonLayout[0].SemanticName = "POSITION";
	polygonLayout[0].SemanticIndex = 0;
	polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[0].InputSlot = 0;
	polygonLayout[0].AlignedByteOffset = 0;
	polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate = 0;

	polygonLayout[1].SemanticName = "TEXCOORD";
	polygonLayout[1].SemanticIndex = 0;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	polygonLayout[1].InputSlot = 0;
	polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate = 0;

	numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);
	result = device->CreateInputLayout(polygonLayout,numElements,vertexShaderBuffer->GetBufferPointer(),
		vertexShaderBuffer->GetBufferSize(),&m_layout);
	if (FAILED(result))
	{
		return false;
	}

	vertexShaderBuffer->Release();
	vertexShaderBuffer = 0;

	pixelShaderBuffer->Release();
	pixelShaderBuffer = 0;

	///setup the description of the dynamic constant buffer that is in the vertex shader
	constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	constantBufferDesc.ByteWidth = sizeof(ConstantBufferType);
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constantBufferDesc.MiscFlags = 0;
	constantBufferDesc.StructureByteStride = 0;

	///create the constant buffer pointerso we can access the vertex shader constant buffer from within the class
	result = device->CreateBuffer(&constantBufferDesc,NULL,&m_constantBuffer);
	if (FAILED(result))
	{
		return false;
	}

	///create a texture sampler state description
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	result = device->CreateSamplerState(&samplerDesc, &m_sampleState);
	if (FAILED(result))
	{
		return false;
	}

	pixelBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	pixelBufferDesc.ByteWidth = sizeof(PixelBufferType);
	pixelBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	pixelBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	pixelBufferDesc.MiscFlags = 0;
	pixelBufferDesc.StructureByteStride = 0;

	result = device->CreateBuffer(&pixelBufferDesc,NULL,&m_pixelBuffer);
	if (FAILED(result))
	{
		return false;
	}
	return true;
}

void FontShaderClass::ShutdownShader()
{
	if (m_pixelBuffer)
	{
		m_pixelBuffer->Release();
		m_pixelBuffer = 0;
	}

	// Release the sampler state.
	if (m_sampleState)
	{
		m_sampleState->Release();
		m_sampleState = 0;
	}

	// Release the constant buffer.
	if (m_constantBuffer)
	{
		m_constantBuffer->Release();
		m_constantBuffer = 0;
	}

	// Release the layout.
	if (m_layout)
	{
		m_layout->Release();
		m_layout = 0;
	}

	// Release the pixel shader.
	if (m_pixelShader)
	{
		m_pixelShader->Release();
		m_pixelShader = 0;
	}

	// Release the vertex shader.
	if (m_vertexShader)
	{
		m_vertexShader->Release();
		m_vertexShader = 0;
	}
}

void FontShaderClass::OutputShaderErrorMessage(ID3D10Blob *errorMessage, HWND hwnd, const WCHAR *shaderFilename)
{
	char* compileErrors;
	unsigned long bufferSize, i;
	ofstream fout;


	// Get a pointer to the error message text buffer.
	compileErrors = (char*)(errorMessage->GetBufferPointer());

	// Get the length of the message.
	bufferSize = errorMessage->GetBufferSize();

	// Open a file to write the error message to.
	fout.open("shader-error.txt");

	// Write out the error message.
	for (i = 0; i < bufferSize; i++)
	{
		fout << compileErrors[i];
	}

	// Close the file.
	fout.close();

	// Release the error message.
	errorMessage->Release();
	errorMessage = 0;

	// Pop a message up on the screen to notify the user to check the text file for compile errors.
	MessageBox(hwnd, L"Error compiling shader.  Check shader-error.txt for message.", shaderFilename, MB_OK);
}

bool FontShaderClass::SetShaderParmeters(ID3D11DeviceContext *deviceContext, XMMATRIX worldMatrix,
	XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView * texture, D3DXVECTOR4 pixelColor)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ConstantBufferType* dataPtr;
	unsigned int bufferNumber;
	PixelBufferType* dataPtr2;
	XMMATRIX mvp = worldMatrix * viewMatrix * projectionMatrix;

	//lock the  constant buffer
	result = deviceContext->Map(m_constantBuffer,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	dataPtr = (ConstantBufferType*)mappedResource.pData;

	dataPtr->gWorldViewProjection = mvp;

	deviceContext->Unmap(m_constantBuffer,0);
	//set the constant buffer to vsshader
	deviceContext->VSSetConstantBuffers(0,1,&m_constantBuffer);

	deviceContext->PSSetShaderResources(0,1,&texture);

	deviceContext->Map(m_pixelBuffer,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	dataPtr2 = (PixelBufferType*)mappedResource.pData;
	dataPtr2->pixelColor = pixelColor;
	deviceContext->Unmap(m_pixelBuffer, 0);
	///set the pixel constant buffer in the pixel shader
	deviceContext->PSSetConstantBuffers(1,1,&m_pixelBuffer);
	return true;
}

void FontShaderClass::RenderShader(ID3D11DeviceContext *deviceContext, int indexCount)
{
	deviceContext->IASetInputLayout(m_layout);
	deviceContext->VSSetShader(m_vertexShader, NULL, 0);
	deviceContext->PSSetShader(m_pixelShader, NULL, 0);

	//set sampler state in the pixel shader
	deviceContext->PSSetSamplers(0,1,&m_sampleState);

	deviceContext->DrawIndexed(indexCount,0,0);
}
