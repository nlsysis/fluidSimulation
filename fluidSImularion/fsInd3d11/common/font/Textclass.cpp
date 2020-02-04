#include "Textclass.h"
#include "Fontclass.h"
#include "FontShaderclass.h"

TextClass::TextClass()
	:
	m_Font(nullptr),
	m_FontShader(nullptr),
	m_sentence1(nullptr)
{
}

TextClass::TextClass(const TextClass &)
{
}

TextClass::~TextClass()
{
}

bool TextClass::Initialize(ID3D11Device *device,HWND hwnd, 
	int screenWidth, int screenHeight, XMMATRIX baseViewMatrix)
{
	bool result;

	m_screenWidth = screenWidth;
	m_screenHeight = screenHeight;

	m_baseViewMatrix = baseViewMatrix;

	//create the font object
	m_Font = new FontClass;
	if (!m_Font) return false;

	result = m_Font->Initialize(device,L".\\Data\\Fonts\\fontdata.txt",L".\\Data\\Fonts\\font.dds");
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the font object", L"Error",MB_OK);
		return false;
	}

	//create the font shader object
	m_FontShader = new FontShaderClass;
	if (!m_FontShader)
	{
		return false;
	}

	result = m_FontShader->Initialize(device, hwnd);
	if(!result)
	{
		MessageBox(hwnd,L"Could not initialize the font shader object",L"Error",MB_OK);
		return false;
	}

	//initialze the first sentence
	result = InitializeSentence(&m_sentence1,50,device);
	if (!result)
	{
		return false;
	}

	return true;
}

void TextClass::Shutdown()
{
	ReleaseSentence(&m_sentence1);

	if (m_FontShader)
	{
		m_FontShader->Shutdown();
		delete m_FontShader;
		m_FontShader = nullptr;
	} 

	if (m_Font)
	{
		m_Font->Shutdown();
		delete m_Font;
		m_Font = nullptr;
	}

}

bool TextClass::Render(ID3D11DeviceContext * deviceContext, XMMATRIX worldMatrix, XMMATRIX orthoMatrix)
{
	bool result;

	//draw the first sentence
	result = RenderSentence(deviceContext,m_sentence1,worldMatrix,orthoMatrix);
	if (!result)
	{
		return false;
	}
	return true;
}

bool TextClass::InitializeSentence(SentenceType ** sentence, int maxLength, ID3D11Device *device)
{
	VertexType* vertices;
	unsigned long* indices;
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;
	int i;

	//create a new sentence object
	*sentence = new SentenceType;
	if (!*sentence)
	{
		return false;
	}

	//initialze the sentence buffer to null
	(*sentence)->vertexBuffer = nullptr;
	(*sentence)->indexBuffer = nullptr;
	//set the max length of the sentence
	(*sentence)->maxLength = maxLength;

	(*sentence)->vertexCount = 6 * maxLength;
	(*sentence)->indexCount = (*sentence)->vertexCount;

	//create the vertex array
	vertices = new VertexType[(*sentence)->vertexCount];
	if (!vertices)
	{
		return false;
	}

	
	indices = new unsigned long[(*sentence)->indexCount];
	if (!indices)
	{
		return false;
	}

	//initialize the vertex array. 
	memset(vertices, 0, (sizeof(VertexType)*(*sentence)->vertexCount));

	//initialize the index array.
	for (i = 0; i < (*sentence)->indexCount; i++)
	{
		indices[i] = i;
	}

	//set the description of the dynamic vertex buffer
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.ByteWidth = sizeof(VertexType)*(*sentence)->vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	//give the subresource struct a  pointer to the vertex data
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	//create the vertex buffer
	result = device->CreateBuffer(&vertexBufferDesc,&vertexData,&(*sentence)->vertexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	//set the descripition of static index buffer
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * (*sentence)->indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&indexBufferDesc,&indexData,&(*sentence)->indexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	delete[] vertices;
	vertices = 0;

	delete[] indices;
	indices = 0;


	return true;
}
void TextClass::UpdateText(const char *text, float positionX, float positionY,
	float red, float green, float blue, ID3D11DeviceContext *deviceContext)
{
	UpdateSentence(m_sentence1, text, positionX, positionY, red, green, blue, deviceContext);
}
/**********************************************************************************
	*@brief be called to change the contents location and color of string
**********************************************************************************/
bool TextClass::UpdateSentence(SentenceType *sentence,const char *text, float positionX, float positionY, 
	float red, float green, float blue, ID3D11DeviceContext *deviceContext)
{
	int numLetters;
	VertexType* vertices;
	float drawX, drawY;
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	VertexType* verticesPtr;

	//sest the color
	sentence->red = red;
	sentence->green = green;
	sentence->blue = blue;

	sentence->text = (char*)text;
	numLetters = (int)strlen(text);

	//check the possible buffer overflow
	if (numLetters > sentence->maxLength)
	{
		return false;
	}

	//create the vertex array
	vertices = new VertexType[sentence->vertexCount];
	if (!vertices)
	{
		return false;
	}

	//initialize the vertex to zeros
	memset(vertices, 0, (sizeof(VertexType)* sentence->vertexCount));

	//calculate the X and Y pixel position on the screen to start drawing to
	drawX = (float)((m_screenWidth / 2 * -1) + positionX);
	drawY = (float)((m_screenHeight / 2) - positionY);

	m_Font->BuildVertexArray((void*)vertices, text, drawX, drawY);

	result = deviceContext->Map(sentence->vertexBuffer,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	verticesPtr = (VertexType*)mappedResource.pData;

	memcpy(verticesPtr,(void*)vertices,(sizeof(VertexType))*sentence->vertexCount);

	deviceContext->Unmap(sentence->vertexBuffer, 0);

	delete[] vertices;
	vertices = nullptr;
	return true;
}
/**********************************************************************************
	*@brief release the sentence vertex and index buffer
**********************************************************************************/
void TextClass::ReleaseSentence(SentenceType **sentence)
{
	if (*sentence)
	{
		// Release the sentence vertex buffer.
		if ((*sentence)->vertexBuffer)
		{
			(*sentence)->vertexBuffer->Release();
			(*sentence)->vertexBuffer = 0;
		}

		// Release the sentence index buffer.
		if ((*sentence)->indexBuffer)
		{
			(*sentence)->indexBuffer->Release();
			(*sentence)->indexBuffer = 0;
		}

		// Release the sentence.
		delete *sentence;
		*sentence = 0;
	}
}

bool TextClass::RenderSentence(ID3D11DeviceContext * deviceContext, SentenceType *sentence,
	XMMATRIX worldMatrix, XMMATRIX orthoMatrix)
{
	unsigned int stride, offset;
	D3DXVECTOR4 pixelColor;
	bool result;

	stride = sizeof(VertexType);
	offset = 0;

	deviceContext->IASetVertexBuffers(0, 1, &sentence->vertexBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(sentence->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	pixelColor = D3DXVECTOR4(sentence->red,sentence->green,sentence->blue,1.0f);
	result = m_FontShader->Render(deviceContext, sentence->indexCount, worldMatrix, m_baseViewMatrix, orthoMatrix,
		m_Font->GetTexture(), pixelColor);
	if (!result)
	{
		return false;
	}
	return true;
}
