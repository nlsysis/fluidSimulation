#pragma once
#include <d3d11.h>
#include <d3dx10math.h>
#include <fstream>
using namespace std;

class TextureClass;

class FontClass
{
private:
	struct FontType
	{
		float left, right;  //texture coordinate
		int size;
	};
	struct VertexType
	{
		D3DXVECTOR3 position;
		D3DXVECTOR2 texture;
	};
public:
	FontClass();
	FontClass(const FontClass&);
	~FontClass();

	bool Initialize(ID3D11Device*, const WCHAR*, const WCHAR*);
	void Shutdown();
	ID3D11ShaderResourceView* GetTexture();
	void BuildVertexArray(void*,const char*,float,float);

private:
	bool LoadFontData(const WCHAR*);
	void ReleaseFontData();
	bool LoadTexture(ID3D11Device*, const WCHAR*);
	void ReleaseTexture();
private:
	FontType* m_Font;
	TextureClass* m_Texture;
};