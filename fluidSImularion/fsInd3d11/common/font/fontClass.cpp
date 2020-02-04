#include "fontClass.h"
#include "textureClass.h"

FontClass::FontClass()
	:
	m_Font(nullptr),
	m_Texture(nullptr)
{
}

FontClass::FontClass(const FontClass &)
{
}

FontClass::~FontClass()
{
}

bool FontClass::Initialize(ID3D11Device *device, const WCHAR *fontFilename, const WCHAR *textureFilename)
{
	bool result;
	result = LoadFontData(fontFilename);
	if (!result)
	{
		return false;
	}
	result = LoadTexture(device, textureFilename);
	if (!result)
	{
		return false;
	}
	return true;
}

void FontClass::Shutdown()
{
	ReleaseTexture();

	// Release the font data.
	ReleaseFontData();

}

/******************************************
	*@brief load the uv and the font from fontdata.txt file 
*******************************************/
bool FontClass::LoadFontData(const WCHAR *filename)
{
	ifstream fin;
	int i;
	char temp;
	m_Font = new FontType[95];
	if (!m_Font)
	{
		return false;
	}
	fin.open(filename);
	if (fin.fail())
	{
		return false;
	}
	//read in the 95 used ascii characters
	for (i = 0; i < 95; i++)
	{
		fin.get(temp);
		while (temp != ' ')
		{
			fin.get(temp);
		}
		fin.get(temp);
		while (temp != ' ')
		{
			fin.get(temp);
		}
		fin.get(temp);
		fin >> m_Font[i].left;
		fin >> m_Font[i].right;
		fin >> m_Font[i].size;

	}
	fin.close();
	return true;
}

void FontClass::ReleaseFontData()
{
	if (m_Font)
	{
		delete[] m_Font;
		m_Font = 0;
	}
}
/**
	*@brief load font.dds texture file
**/
bool FontClass::LoadTexture(ID3D11Device *device, const WCHAR *filename)
{
	bool result;
	m_Texture = new TextureClass;
	if (!m_Texture)
	{
		return false;
	}

	result = m_Texture->Initialize(device, filename);
	if (!result)
	{
		return false;
	}
	return true;
}

void FontClass::ReleaseTexture()
{
	if (m_Texture)
	{
		m_Texture->Shutdown();
		delete m_Texture;
		m_Texture = 0;
	}
}

ID3D11ShaderResourceView* FontClass::GetTexture()
{
	return m_Texture->GetTexture();
}
/********************************************************
	*@brief:called by Textclass to build vertex buffers
*********************************************************/
void FontClass::BuildVertexArray(void* vertices, const char* sentence, float drawX, float drawY)
{
	VertexType* vertexPtr;
	int numLetters, index, i, letter;

	vertexPtr = (VertexType*)vertices;
	numLetters = (int)strlen(sentence);
	index = 0;
	///build vertex and index array take each character from sentences
	///map the each characte from the font texture
	for (i = 0; i < numLetters; i++)
	{
		letter = ((int)sentence[i]) - 32;
		if (letter == 0)
		{
			drawX = drawX + 3.0f;
		}
		else
		{
			///first triangle in quad
			vertexPtr[index].position = D3DXVECTOR3(drawX, drawY, 0.0f);   // Top left
			vertexPtr[index].texture = D3DXVECTOR2(m_Font[letter].left, 0.0f);
			index++;

			vertexPtr[index].position = D3DXVECTOR3(drawX + m_Font[letter].size, drawY - 16, 0.0f);  // Bottom right.
			vertexPtr[index].texture = D3DXVECTOR2(m_Font[letter].right,1.0f);
			index++;
		
			vertexPtr[index].position = D3DXVECTOR3(drawX, drawY - 16, 0.0f);   // Bottom left.
			vertexPtr[index].texture = D3DXVECTOR2(m_Font[letter].left, 1.0f);
			index++;
			///second triangle in quad
			vertexPtr[index].position = D3DXVECTOR3(drawX, drawY, 0.0f);
			vertexPtr[index].texture = D3DXVECTOR2(m_Font[letter].left,0.0f);
			index++;

			vertexPtr[index].position = D3DXVECTOR3(drawX + m_Font[letter].size,drawY,0.0f);
			vertexPtr[index].texture = D3DXVECTOR2(m_Font[letter].right,0.0f);
			index++;

			vertexPtr[index].position = D3DXVECTOR3(drawX + m_Font[letter].size,drawY-16,0.0f);
			vertexPtr[index].texture = D3DXVECTOR2(m_Font[letter].right, 1.0f);
			index++;
			// Update the x location for drawing by the size of the letter and one pixel.
			drawX = drawX + m_Font[letter].size + 1.0f;
		}
	}
}
