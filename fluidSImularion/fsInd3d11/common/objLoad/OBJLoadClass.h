#pragma once
#include "ObjModelClass.h"
#include<fstream>
#include <sstream>
using std::ifstream;
using std::istringstream;

class OBJLoadClass
{
private:
	struct VertexPosition
	{
		float x, y, z;
		VertexPosition(float a, float b, float c) :x(a), y(b), z(c){}
	};
	struct VertexNormal
	{
		float nx, ny, nz;
		VertexNormal(float a, float b, float c) : nx(a), ny(b), nz(c){}
	};
	struct VertexTex
	{
		float u, v;
		VertexTex(float a, float b) : u(a), v(b){}
	};
public:
	OBJLoadClass();
	OBJLoadClass(const OBJLoadClass&);
	~OBJLoadClass();

	bool Initialize(ID3D11Device* d3dDevice, string OBJFileName);
	void Shutdown();

	vector<ObjModelClass*> GetOBJModelArrayCopy() { return mOBJModelArray; }  //return copy of model array
private:
	bool InitializeOBJModelArray(ID3D11Device* d3dDevice);
	void ReleaseOBJModelArray();
	bool LoadOBJFile(string OBJFileName);
	bool LoadOBJMaterialFile(string MaterialFileName);

private:
	vector<VertexPosition> mModelPosArray;      //load model postion data
	vector<VertexNormal> mModelNormalArray;     //load model normal data
	vector<VertexTex> mModelTexArray;           //load model texture data
	vector<ObjModelClass*> mOBJModelArray;      //OBJ Model object array(may multi objects in one model file)
	string ObjMaterialFileName;
};