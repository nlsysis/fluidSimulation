#include "ObjModelClass.h"

ObjModelClass::ObjModelClass()
	:
	md3dVertexBuffer(nullptr),
	md3dIndexBuffer(nullptr),
	mVertexCount(0),
	mIndexCount(0),
	mMaterial(nullptr),
	ObjSRV(nullptr)
{
}


ObjModelClass::ObjModelClass(const ObjModelClass& other)
{

}

ObjModelClass::~ObjModelClass()
{

}

bool ObjModelClass::Initialize(ID3D11Device* d3dDevice)
{
	bool result;

	//initialize material
	mMaterial = new OBJMaterial();
	if (!mMaterial)
	{
		MessageBox(NULL, L"Initialize OBJMaterial failure", L"ERROR", MB_OK);
		return false;
	}

	//initialize vertex buffer and index buffer
	result = InitializeBuffer(d3dDevice);
	if (!result)
	{
		MessageBox(NULL, L"Initialize Buffer failure", L"ERROR", MB_OK);
		return false;
	}

	return true;
}

void ObjModelClass::Shutdown()
{
	ShutdownBuffer();
	ReleaseModelData();
	ReleaseMaterialData();
}


void ObjModelClass::Render(ID3D11DeviceContext* d3dDeviceContext)
{
	//set render pipeling of  vertex and index buffer (IA)
	RenderBuffers(d3dDeviceContext);
}

bool ObjModelClass::InitializeBuffer(ID3D11Device* d3dDevice)
{
	Vertex* vertexs = NULL;
	unsigned long *indices = nullptr;

	//initilize vertex count and index count
	mVertexCount = mIndexCount = mVertexData.size();

	//create vertex array
	vertexs = new Vertex[mVertexCount];
	if (!vertexs)
		return false;

	//create index array
	indices = new unsigned long[mIndexCount];
	if (!indices)
		return false;

	//input the data to vertex
	for (size_t i = 0; i < mVertexCount; ++i)
	{
		vertexs[i].pos = XMFLOAT3(mVertexData[i]->x, mVertexData[i]->y, mVertexData[i]->z);
		vertexs[i].tex = XMFLOAT2(mVertexData[i]->u, mVertexData[i]->v);
		vertexs[i].normal = XMFLOAT3(mVertexData[i]->nx, mVertexData[i]->ny, mVertexData[i]->nz);
	}
	//input index data
	for (int i = 0; i < mIndexCount; ++i)
	{
		indices[i] = i;
	}

	//First create vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(Vertex) * mVertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = vertexs;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;
	HR(d3dDevice->CreateBuffer(&vertexBufferDesc, &vertexData, &md3dVertexBuffer));

	//Second create index buffer
	D3D11_BUFFER_DESC  indexBufferDesc;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * mIndexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;
	HR(d3dDevice->CreateBuffer(&indexBufferDesc, &indexData, &md3dIndexBuffer));

	//release vertex array and index array
	delete[]vertexs;
	vertexs = NULL;
	delete[]indices;
	indices = NULL;

	return true;
}

void ObjModelClass::ShutdownBuffer()
{
	//release indexBudder and vertexBuffer
	ReleaseCOM(md3dIndexBuffer);
	ReleaseCOM(md3dVertexBuffer);
}

void ObjModelClass::RenderBuffers(ID3D11DeviceContext* d3dDeviceContext)
{
	UINT stride = sizeof(Vertex); 
	UINT offset = 0;
	d3dDeviceContext->IASetVertexBuffers(0, 1, &md3dVertexBuffer, &stride, &offset);
	d3dDeviceContext->IASetIndexBuffer(md3dIndexBuffer, DXGI_FORMAT_R32_UINT, 0); 
	d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}


void ObjModelClass::ReleaseModelData()
{
	for (auto it = mVertexData.begin(); it != mVertexData.end(); ++it)
	{
		delete (*it);
		(*it) = NULL;
	}
}


void ObjModelClass::ReleaseMaterialData()
{
	if (mMaterial)
	{
		delete mMaterial;
		mMaterial = NULL;
	}
}