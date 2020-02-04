#pragma once
#include <d3d11.h>
#include <d3dx10math.h>
#include <xnamath.h>

class FontClass;
class FontShaderClass;

class TextClass
{
private:
	struct SentenceType
	{
		ID3D11Buffer* vertexBuffer, *indexBuffer;
		int vertexCount, indexCount, maxLength;
		float red, green, blue;
		char* text;
	};
	struct VertexType
	{
		D3DXVECTOR3 position;
		D3DXVECTOR2 texture;
	};
public:
	TextClass();
	TextClass(const TextClass&);
	~TextClass();
	bool Initialize(ID3D11Device*, HWND,int,int,XMMATRIX);
	void Shutdown();
	bool Render(ID3D11DeviceContext*, XMMATRIX, XMMATRIX);
	void UpdateText(const char*, float, float, float, float, float, ID3D11DeviceContext*);
private:
	bool InitializeSentence(SentenceType** Type, int, ID3D11Device*);
	bool UpdateSentence(SentenceType*,const char*,float,float,float,float,float,ID3D11DeviceContext*);
	void ReleaseSentence(SentenceType**);
	bool RenderSentence(ID3D11DeviceContext*,SentenceType*,XMMATRIX,XMMATRIX);

private:
	FontClass* m_Font;
	FontShaderClass* m_FontShader;
	int m_screenWidth, m_screenHeight;
	XMMATRIX  m_baseViewMatrix;
	//this time use two sentences in the tutorial
	SentenceType* m_sentence1;
};