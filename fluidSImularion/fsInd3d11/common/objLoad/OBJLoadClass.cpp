#include "OBJLoadClass.h"

OBJLoadClass::OBJLoadClass()
{

}


OBJLoadClass::OBJLoadClass(const OBJLoadClass&)
{

}


OBJLoadClass::~OBJLoadClass()
{

}

void OBJLoadClass::Shutdown()
{
	//release objects model data
	ReleaseOBJModelArray();
}


void OBJLoadClass::ReleaseOBJModelArray()
{
	for (size_t i = 0; i < mOBJModelArray.size(); ++i)
	{
		mOBJModelArray[i]->Shutdown();
		delete mOBJModelArray[i];
		mOBJModelArray[i] = NULL;
	}
}


bool OBJLoadClass::Initialize(ID3D11Device* d3dDevice, string OBJFileName)
{
	bool result;

	//First: load obj file
	result = LoadOBJFile(OBJFileName);
	if (!result)
	{

		MessageBox(NULL, L"LoadOBJFilefailure", NULL, MB_OK);
		return false;
	}


	//Second:initialize obj model objects array
	result = InitializeOBJModelArray(d3dDevice);
	if (!result)
	{

		MessageBox(NULL, L"OBJModelArray Initialize", NULL, MB_OK);
		return false;
	}


	//Third:load obj material form file
	result = LoadOBJMaterialFile(ObjMaterialFileName);
	if (!result)
	{

		MessageBox(NULL, L"LoadOBJMaterialFile Initialize", NULL, MB_OK);
		return false;
	}



	return true;
}


bool OBJLoadClass::LoadOBJFile(string OBJFileName)
{

	ifstream in(OBJFileName);
	if (!in)
	{
		return false;
	}

	std::string line, word;  //the `line` presents whole line while the word presents word without space

	UINT NullStrCount = 0;  //the number of blank line
	UINT OBJCount = 0;    //the numer of obj

	ObjModelClass* memObjModel = new ObjModelClass();
	if (!memObjModel)
	{
		return false;
	}
	vector<ObjModelClass::ModelType*>& mVertexArray = memObjModel->GetVertexArrayRef();

	//read form the first line
	while (!in.eof())
	{
		getline(in, line);
		istringstream re(line);
		re >> word;
		//if line include `mtllib` then save the obj material route.
		if (line.find("mtllib") != string::npos)
		{
			string MaterialFileName;
			istringstream record(line);
			record >> word;
			record >> word;
			MaterialFileName = string(".\\Data\\Model\\") + word;
			ObjMaterialFileName = MaterialFileName;
		}
		else if (line.find("usemtl") != string::npos)
		{
			istringstream re1(line);
			re1 >> word;
			re1 >> word;
			memObjModel->SetMaterialName(word);
		}
		//if the line start by `v` which stand for vertex
		if (word == string("v"))
		{
			re >> word;
			float PosX = atof(&word[0]);
			re >> word;
			float PosY = atof(&word[0]);
			re >> word;
			float PosZ = atof(&word[0]);
			mModelPosArray.push_back(VertexPosition(PosX, PosY, PosZ));

		}
		//if the line start by `vn` which stand for normal
		else if (word == string("vn"))
		{
			re >> word;
			float NormalX = atof(&word[0]);
			re >> word;
			float NormalY = atof(&word[0]);
			re >> word;
			float NormalZ = atof(&word[0]);
			mModelNormalArray.push_back(VertexNormal(NormalX, NormalY, NormalZ));
		}
		//if the line start by `vt` which stand for vertex texture
		else if (word == string("vt"))
		{
			re >> word;
			float u = atof(&word[0]);
			re >> word;
			float v = atof(&word[0]);
			mModelTexArray.push_back(VertexTex(u, v));
		}
		//if the line start by `f` which stand for faces
		else if (word == string("f"))
		{
			//1 face - 3 vertices
			ObjModelClass::ModelType* vertex1 = new ObjModelClass::ModelType();
			if (!vertex1)
			{
				return false;
			}

			ObjModelClass::ModelType* vertex2 = new ObjModelClass::ModelType();
			if (!vertex2)
			{
				return false;
			}
			ObjModelClass::ModelType* vertex3 = new ObjModelClass::ModelType();
			if (!vertex3)
			{
				return false;
			}

			//if texture coordinate is exist, some model do not have texture coordinate
			bool IsTexExist;
			re >> word;

			if (word.find("//") == string::npos)
			{
				IsTexExist = true;
			}
			else
			{
				IsTexExist = false;
			}

			//if not include `//`,read the vertex position,normal,texcoord
			if (IsTexExist)
			{
				/*******first vertex*******/
				//find the position of vertex
				UINT FirstIndex = word.find("/");
				string NumericStr = word.substr(0, FirstIndex);
				UINT PosIndex = atoi(&NumericStr[0]);
				vertex1->x = mModelPosArray[PosIndex - 1].x;
				vertex1->y = mModelPosArray[PosIndex - 1].y;
				vertex1->z = mModelPosArray[PosIndex - 1].z;

				//find tex of vertex
				UINT SecondIndex = word.find("/", FirstIndex + 1);
				NumericStr = word.substr(FirstIndex + 1, SecondIndex - FirstIndex - 1);
				PosIndex = atoi(&NumericStr[0]);
				vertex1->u = mModelTexArray[PosIndex - 1].u;
				vertex1->v = mModelTexArray[PosIndex - 1].v;

				//find normal of vertex
				NumericStr = word.substr(SecondIndex + 1);
				PosIndex = atoi(&NumericStr[0]);
				vertex1->nx = mModelNormalArray[PosIndex - 1].nx;
				vertex1->ny = mModelNormalArray[PosIndex - 1].ny;
				vertex1->nz = mModelNormalArray[PosIndex - 1].nz;


				/*******second vertex*******/
				re >> word;
				FirstIndex = word.find("/");
				NumericStr = word.substr(0, FirstIndex);
				PosIndex = atoi(&NumericStr[0]);
				vertex2->x = mModelPosArray[PosIndex - 1].x;
				vertex2->y = mModelPosArray[PosIndex - 1].y;
				vertex2->z = mModelPosArray[PosIndex - 1].z;

				//find tex of vertex2
				SecondIndex = word.find("/", FirstIndex + 1);
				NumericStr = word.substr(FirstIndex + 1, SecondIndex - FirstIndex - 1);
				PosIndex = atoi(&NumericStr[0]);
				vertex2->u = mModelTexArray[PosIndex - 1].u;
				vertex2->v = mModelTexArray[PosIndex - 1].v;

				//find normal of vertex2
				NumericStr = word.substr(SecondIndex + 1);
				PosIndex = atoi(&NumericStr[0]);
				vertex2->nx = mModelNormalArray[PosIndex - 1].nx;
				vertex2->ny = mModelNormalArray[PosIndex - 1].ny;
				vertex2->nz = mModelNormalArray[PosIndex - 1].nz;


				/*******third vertex*******/
				re >> word;
				FirstIndex = word.find("/");
				NumericStr = word.substr(0, FirstIndex);
				PosIndex = atoi(&NumericStr[0]);
				vertex3->x = mModelPosArray[PosIndex - 1].x;
				vertex3->y = mModelPosArray[PosIndex - 1].y;
				vertex3->z = mModelPosArray[PosIndex - 1].z;

				//find tex of vertex3
				SecondIndex = word.find("/", FirstIndex + 1);
				NumericStr = word.substr(FirstIndex + 1, SecondIndex - FirstIndex - 1);
				PosIndex = atoi(&NumericStr[0]);
				vertex3->u = mModelTexArray[PosIndex - 1].u;
				vertex3->v = mModelTexArray[PosIndex - 1].v;


				//find normal of vertex3
				NumericStr = word.substr(SecondIndex + 1);
				PosIndex = atoi(&NumericStr[0]);
				vertex3->nx = mModelNormalArray[PosIndex - 1].nx;
				vertex3->ny = mModelNormalArray[PosIndex - 1].ny;
				vertex3->nz = mModelNormalArray[PosIndex - 1].nz;

			}

			//if include `//`
			else
			{

				/*******first vertex*******/
				//find the position of vertex
				UINT FirstIndex = word.find("//");
				string NumericStr = word.substr(0, FirstIndex);
				UINT PosIndex = atoi(&NumericStr[0]);
				vertex1->x = mModelPosArray[PosIndex - 1].x;
				vertex1->y = mModelPosArray[PosIndex - 1].y;
				vertex1->z = mModelPosArray[PosIndex - 1].z;

				//find normal of vertex
				NumericStr = word.substr(FirstIndex + 2);
				PosIndex = atoi(&NumericStr[0]);
				vertex1->nx = mModelNormalArray[PosIndex - 1].nx;
				vertex1->ny = mModelNormalArray[PosIndex - 1].ny;
				vertex1->nz = mModelNormalArray[PosIndex - 1].nz;

				//set uv 
				vertex1->u = 0.0f;
				vertex1->v = 0.0f;

				/*******second vertex*******/
				re >> word;
				FirstIndex = word.find("//");
				NumericStr = word.substr(0, FirstIndex);
				PosIndex = atoi(&NumericStr[0]);
				vertex2->x = mModelPosArray[PosIndex - 1].x;
				vertex2->y = mModelPosArray[PosIndex - 1].y;
				vertex2->z = mModelPosArray[PosIndex - 1].z;

				//find normal of vertex2
				NumericStr = word.substr(FirstIndex + 2);
				PosIndex = atoi(&NumericStr[0]);
				vertex2->nx = mModelNormalArray[PosIndex - 1].nx;
				vertex2->ny = mModelNormalArray[PosIndex - 1].ny;
				vertex2->nz = mModelNormalArray[PosIndex - 1].nz;

				vertex2->u = 0.0f;
				vertex2->v = 0.0f;



				/*******third vertex*******/
				re >> word;
				FirstIndex = word.find("//");
				NumericStr = word.substr(0, FirstIndex);
				PosIndex = atoi(&NumericStr[0]);
				vertex3->x = mModelPosArray[PosIndex - 1].x;
				vertex3->y = mModelPosArray[PosIndex - 1].y;
				vertex3->z = mModelPosArray[PosIndex - 1].z;


				NumericStr = word.substr(FirstIndex + 2);
				PosIndex = atoi(&NumericStr[0]);
				vertex3->nx = mModelNormalArray[PosIndex - 1].nx;
				vertex3->ny = mModelNormalArray[PosIndex - 1].ny;
				vertex3->nz = mModelNormalArray[PosIndex - 1].nz;

				vertex3->u = 0.0f;
				vertex3->v = 0.0f;
			}
			mVertexArray.push_back(vertex1);
			mVertexArray.push_back(vertex2);
			mVertexArray.push_back(vertex3);
		}
		//else if (line.find("Object") != string::npos)
		//{
		//	istringstream record(line);
		//	record >> word;
		//	record >> word;
		//	record >> word;
		//	ObjModelClass* memObjModel = new ObjModelClass();
		//	if (!memObjModel)
		//	{
		//		return false;
		//	}
		//	vector<ObjModelClass::ModelType*>& mVertexArray = memObjModel->GetVertexArrayRef();;

		//	memObjModel->SetModelName(word);
		//	while (1)
		//	{
		//		getline(in, line);

		//		istringstream re(line);
		//		re >> word;

		//		//if `line` include `faces` then end loop
		//		if (line.find("faces") != string::npos)
		//		{
		//			//save obj to obj array
		//			mOBJModelArray.push_back(memObjModel);
		//			break;
		//		}

		//		//if `line` includes `usemtl`,save the material name of this object
		//		if (line.find("usemtl") != string::npos)
		//		{
		//			istringstream re1(line);
		//			re1 >> word;
		//			re1 >> word;
		//			memObjModel->SetMaterialName(word);
		//		}

		//		//if the line start by `v` which stand for vertex
		//		if (word == string("v"))
		//		{
		//			re >> word;
		//			float PosX = atof(&word[0]);
		//			re >> word;
		//			float PosY = atof(&word[0]);
		//			re >> word;
		//			float PosZ = atof(&word[0]);
		//			mModelPosArray.push_back(VertexPosition(PosX, PosY, PosZ));

		//		}
		//		//if the line start by `vn` which stand for normal
		//		else if (word == string("vn"))
		//		{
		//			re >> word;
		//			float NormalX = atof(&word[0]);
		//			re >> word;
		//			float NormalY = atof(&word[0]);
		//			re >> word;
		//			float NormalZ = atof(&word[0]);
		//			mModelNormalArray.push_back(VertexNormal(NormalX, NormalY, NormalZ));
		//		}
		//		//if the line start by `vt` which stand for vertex texture
		//		else if (word == string("vt"))
		//		{
		//			re >> word;
		//			float u = atof(&word[0]);
		//			re >> word;
		//			float v = atof(&word[0]);
		//			mModelTexArray.push_back(VertexTex(u, v));
		//		}
		//		//if the line start by `f` which stand for faces
		//		else if (word == string("f"))
		//		{
		//			//1 face - 3 vertices
		//			ObjModelClass::ModelType* vertex1 = new ObjModelClass::ModelType();
		//			if (!vertex1)
		//			{
		//				return false;
		//			}

		//			ObjModelClass::ModelType* vertex2 = new ObjModelClass::ModelType();
		//			if (!vertex2)
		//			{
		//				return false;
		//			}
		//			ObjModelClass::ModelType* vertex3 = new ObjModelClass::ModelType();
		//			if (!vertex3)
		//			{
		//				return false;
		//			}

		//			//if texture coordinate is exist, some model do not have texture coordinate
		//			bool IsTexExist;
		//			re >> word;

		//			if (word.find("//") == string::npos)   
		//			{
		//				IsTexExist = true;
		//			}
		//			else
		//			{
		//				IsTexExist = false;
		//			}

		//			//if not include `//`,read the vertex position,normal,texcoord
		//			if (IsTexExist)
		//			{
		//				/*******first vertex*******/
		//				//find the position of vertex
		//				UINT FirstIndex = word.find("/");
		//				string NumericStr = word.substr(0, FirstIndex);
		//				UINT PosIndex = atoi(&NumericStr[0]);
		//				vertex1->x = mModelPosArray[PosIndex - 1].x;
		//				vertex1->y = mModelPosArray[PosIndex - 1].y;
		//				vertex1->z = mModelPosArray[PosIndex - 1].z;

		//				//find tex of vertex
		//				UINT SecondIndex = word.find("/", FirstIndex + 1);
		//				NumericStr = word.substr(FirstIndex + 1, SecondIndex - FirstIndex - 1);
		//				PosIndex = atoi(&NumericStr[0]);
		//				vertex1->u = mModelTexArray[PosIndex - 1].u;
		//				vertex1->v = mModelTexArray[PosIndex - 1].v;

		//				//find normal of vertex
		//				NumericStr = word.substr(SecondIndex + 1);
		//				PosIndex = atoi(&NumericStr[0]);
		//				vertex1->nx = mModelNormalArray[PosIndex - 1].nx;
		//				vertex1->ny = mModelNormalArray[PosIndex - 1].ny;
		//				vertex1->nz = mModelNormalArray[PosIndex - 1].nz;


		//				/*******second vertex*******/
		//				re >> word;
		//				FirstIndex = word.find("/");
		//				NumericStr = word.substr(0, FirstIndex);
		//				PosIndex = atoi(&NumericStr[0]);
		//				vertex2->x = mModelPosArray[PosIndex - 1].x;
		//				vertex2->y = mModelPosArray[PosIndex - 1].y;
		//				vertex2->z = mModelPosArray[PosIndex - 1].z;

		//				//find tex of vertex2
		//				SecondIndex = word.find("/", FirstIndex + 1);
		//				NumericStr = word.substr(FirstIndex + 1, SecondIndex - FirstIndex - 1);
		//				PosIndex = atoi(&NumericStr[0]);
		//				vertex2->u = mModelTexArray[PosIndex - 1].u;
		//				vertex2->v = mModelTexArray[PosIndex - 1].v;

		//				//find normal of vertex2
		//				NumericStr = word.substr(SecondIndex + 1);
		//				PosIndex = atoi(&NumericStr[0]);
		//				vertex2->nx = mModelNormalArray[PosIndex - 1].nx;
		//				vertex2->ny = mModelNormalArray[PosIndex - 1].ny;
		//				vertex2->nz = mModelNormalArray[PosIndex - 1].nz;


		//				/*******third vertex*******/
		//				re >> word;
		//				FirstIndex = word.find("/");
		//				NumericStr = word.substr(0, FirstIndex);
		//				PosIndex = atoi(&NumericStr[0]);
		//				vertex3->x = mModelPosArray[PosIndex - 1].x;
		//				vertex3->y = mModelPosArray[PosIndex - 1].y;
		//				vertex3->z = mModelPosArray[PosIndex - 1].z;

		//				//find tex of vertex3
		//				SecondIndex = word.find("/", FirstIndex + 1);
		//				NumericStr = word.substr(FirstIndex + 1, SecondIndex - FirstIndex - 1);
		//				PosIndex = atoi(&NumericStr[0]);
		//				vertex3->u = mModelTexArray[PosIndex - 1].u;
		//				vertex3->v = mModelTexArray[PosIndex - 1].v;


		//				//find normal of vertex3
		//				NumericStr = word.substr(SecondIndex + 1);
		//				PosIndex = atoi(&NumericStr[0]);
		//				vertex3->nx = mModelNormalArray[PosIndex - 1].nx;
		//				vertex3->ny = mModelNormalArray[PosIndex - 1].ny;
		//				vertex3->nz = mModelNormalArray[PosIndex - 1].nz;

		//			}

		//			//if include `//`
		//			else
		//			{

		//				/*******first vertex*******/
		//				//find the position of vertex
		//				UINT FirstIndex = word.find("//");
		//				string NumericStr = word.substr(0, FirstIndex);
		//				UINT PosIndex = atoi(&NumericStr[0]);
		//				vertex1->x = mModelPosArray[PosIndex - 1].x;
		//				vertex1->y = mModelPosArray[PosIndex - 1].y;
		//				vertex1->z = mModelPosArray[PosIndex - 1].z;

		//				//find normal of vertex
		//				NumericStr = word.substr(FirstIndex + 2);
		//				PosIndex = atoi(&NumericStr[0]);
		//				vertex1->nx = mModelNormalArray[PosIndex - 1].nx;
		//				vertex1->ny = mModelNormalArray[PosIndex - 1].ny;
		//				vertex1->nz = mModelNormalArray[PosIndex - 1].nz;

		//				//set uv 
		//				vertex1->u = 0.0f;
		//				vertex1->v = 0.0f;

		//				/*******second vertex*******/
		//				re >> word;
		//				FirstIndex = word.find("//");
		//				NumericStr = word.substr(0, FirstIndex);
		//				PosIndex = atoi(&NumericStr[0]);
		//				vertex2->x = mModelPosArray[PosIndex - 1].x;
		//				vertex2->y = mModelPosArray[PosIndex - 1].y;
		//				vertex2->z = mModelPosArray[PosIndex - 1].z;

		//				//find normal of vertex2
		//				NumericStr = word.substr(FirstIndex + 2);
		//				PosIndex = atoi(&NumericStr[0]);
		//				vertex2->nx = mModelNormalArray[PosIndex - 1].nx;
		//				vertex2->ny = mModelNormalArray[PosIndex - 1].ny;
		//				vertex2->nz = mModelNormalArray[PosIndex - 1].nz;
		//				
		//				vertex2->u = 0.0f;
		//				vertex2->v = 0.0f;



		//				/*******third vertex*******/
		//				re >> word;
		//				FirstIndex = word.find("//");
		//				NumericStr = word.substr(0, FirstIndex);
		//				PosIndex = atoi(&NumericStr[0]);
		//				vertex3->x = mModelPosArray[PosIndex - 1].x;
		//				vertex3->y = mModelPosArray[PosIndex - 1].y;
		//				vertex3->z = mModelPosArray[PosIndex - 1].z;

		//				
		//				NumericStr = word.substr(FirstIndex + 2);
		//				PosIndex = atoi(&NumericStr[0]);
		//				vertex3->nx = mModelNormalArray[PosIndex - 1].nx;
		//				vertex3->ny = mModelNormalArray[PosIndex - 1].ny;
		//				vertex3->nz = mModelNormalArray[PosIndex - 1].nz;
		//				
		//				vertex3->u = 0.0f;
		//				vertex3->v = 0.0f;

		//			}

		//			mVertexArray.push_back(vertex1);
		//			mVertexArray.push_back(vertex2);
		//			mVertexArray.push_back(vertex3);
		//		}
		//	}
		//}

	}

	mOBJModelArray.push_back(memObjModel);
	return true;
}


bool OBJLoadClass::InitializeOBJModelArray(ID3D11Device* d3dDevice)
{
	bool result;
	for (size_t i = 0; i < mOBJModelArray.size(); ++i)
	{
		result = mOBJModelArray[i]->Initialize(d3dDevice);
		if (!result)
		{
			return false;
		}
	}
	return true;
}

//load obj texture file
bool OBJLoadClass::LoadOBJMaterialFile(string MaterialFileName)
{
	ifstream in(MaterialFileName);
	if (!in)
	{
		return false;
	}


	//the `line` presents whole line while the word presents word without space
	std::string line, word;
	UINT index;

	while (!in.eof())
	{
		getline(in, line);

		//the start signal 
		if (line.find("newmtl") != string::npos)
		{
			istringstream record(line);
			record >> word;
			record >> word;

			//find material
			for (size_t i = 0; i < mOBJModelArray.size(); ++i)
			{
				if (word == mOBJModelArray[i]->GetMaterialName())
				{
					index = i;
					break;
				}
			}
		}
		//load the material attribute 
		if (line.find("Ns") != string::npos)
		{
			istringstream re(line);
			re >> word;
			re >> word;
			float SpecularPower = atof(&word[0]);
			mOBJModelArray[index]->SetSpecularPower(SpecularPower);
		}

		else if (line.find("Ka") != string::npos)
		{
			istringstream re(line);
			re >> word;
			re >> word;
			float r = atof(&word[0]);
			re >> word;
			float g = atof(&word[0]);
			re >> word;
			float b = atof(&word[0]);
			mOBJModelArray[index]->SetAmbientMaterial(r, g, b);
		}
		else if (line.find("Kd") != string::npos)
		{
			istringstream re(line);
			re >> word;
			re >> word;
			float r = atof(&word[0]);
			re >> word;
			float g = atof(&word[0]);
			re >> word;
			float b = atof(&word[0]);
			mOBJModelArray[index]->SetDiffuseMaterial(r, g, b);

		}
		else if (line.find("Ks") != string::npos)
		{
			istringstream re(line);
			re >> word;
			re >> word;
			float r = atof(&word[0]);
			re >> word;
			float g = atof(&word[0]);
			re >> word;
			float b = atof(&word[0]);
			mOBJModelArray[index]->SetSpecularMaterial(r, g, b);
		}
		else if (line.find("map_Kd") != string::npos)
		{
			string TextureFileName;
			istringstream re(line);
			re >> word;
			re >> word;
			TextureFileName = string(".\\Data\\Model\\") + word;
			mOBJModelArray[index]->SetTextureName(TextureFileName);
		}			
	}
	return true;
}