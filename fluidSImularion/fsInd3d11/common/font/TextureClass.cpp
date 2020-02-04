#include "TextureClass.h"

TextureClass::TextureClass()
	:
	m_texture(nullptr)
{
}

TextureClass::TextureClass(const TextureClass &other)
{
}

TextureClass::~TextureClass()
{
}

bool TextureClass::Initialize(ID3D11Device *device, const WCHAR *fileName)
{
	HRESULT result;
	result = D3DX11CreateShaderResourceViewFromFile(device, fileName, NULL, NULL, &m_texture, NULL);
	if (FAILED(result))
	{
		return false;
	}
	return true;
}

void TextureClass::Shutdown()
{
	if (m_texture)
	{
		m_texture->Release();
		m_texture = 0;
	}

}

ID3D11ShaderResourceView * TextureClass::GetTexture()
{
	return m_texture;
}
