#pragma once
#include "d3dUtil.h"
#include<vector>
#include<string>
using std::string;
using std::vector;

class ObjModelClass
{
private:
	struct Vertex
	{
		XMFLOAT3 pos;
		XMFLOAT2 tex;
		XMFLOAT3 normal;
	};
public:
	struct ModelType
	{
		float x, y, z;
		float u, v;
		float nx, ny, nz;
	};

	struct OBJMaterial
	{
		XMFLOAT3 AmbientMaterial;
		XMFLOAT3 DiffuseMaterial;
		XMFLOAT3 SpecularMaterial;
		float SpecularPower; 
	};


public:
	ObjModelClass();
	ObjModelClass(const ObjModelClass&);
	~ObjModelClass();

public:
	bool Initialize(ID3D11Device* d3dDevice);
	void Shutdown();
	void Render(ID3D11DeviceContext* d3dDeviceContext);

	int GetIndexCount() { return mIndexCount; }     //return parament to DrawIndexed();
	vector<ModelType*>& GetVertexArrayRef() { return mVertexData; }

	//Set function
	void SetModelName(string ModelName) { ObjModelName = ModelName; }
	void SetMaterialName(string MaterialName) { ObjMaterialName = MaterialName; }
	void SetAmbientMaterial(float r, float g, float b) { mMaterial->AmbientMaterial = XMFLOAT3(r, g, b); }
	void SetDiffuseMaterial(float r, float g, float b) { mMaterial->DiffuseMaterial = XMFLOAT3(r, g, b); }
	void SetSpecularMaterial(float r, float g, float b) { mMaterial->SpecularMaterial = XMFLOAT3(r, g, b); }
	void SetSpecularPower(float SpecularPower) { mMaterial->SpecularPower = SpecularPower; }
	void SetTextureName(string TextureName) { ObjTextureName = TextureName; }
	void LoadObjectSRV(ID3D11Device* d3dDevice) {
		D3DX11CreateShaderResourceViewFromFile(d3dDevice, (LPCWSTR)ObjTextureName.c_str(),
			NULL, NULL, &ObjSRV, NULL);
	}
		

	//Get function
	string GetMaterialName() { return ObjMaterialName; }
	XMVECTOR GetAmbientMaterial() { return XMLoadFloat3(&mMaterial->AmbientMaterial); }
	XMVECTOR GetDiffuseMaterial() { return XMLoadFloat3(&mMaterial->DiffuseMaterial); }
	XMVECTOR GetSpecularMaterial() { return XMLoadFloat3(&mMaterial->SpecularMaterial); }
	float GetSpecularPower() { return mMaterial->SpecularPower; }
	string GetTextureName() { return ObjTextureName; }

private:
	bool InitializeBuffer(ID3D11Device* d3dDevice);   //initialze the buffers
	void ShutdownBuffer();
	void RenderBuffers(ID3D11DeviceContext* d3dDeviceContext);   //set and render buffers
	void ReleaseModelData();       //release external model data
	void ReleaseMaterialData();    //release external material data

private:
	ID3D11Buffer* md3dVertexBuffer; //vertex buffer
	ID3D11Buffer* md3dIndexBuffer;  //index buffer
	int mVertexCount;
	int mIndexCount;
	vector<ModelType*> mVertexData; //load the external vertexData 
	string ObjModelName;
	string ObjMaterialName;         //material name
	string ObjTextureName;
	ID3D11ShaderResourceView* ObjSRV;
	OBJMaterial* mMaterial;
};
