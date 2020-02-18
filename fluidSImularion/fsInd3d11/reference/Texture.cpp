#include "Texture.h"
TextureManager::TextureManager(void)
	: 
	md3dDevice(nullptr),
	mDC(nullptr)
{
}


TextureManager::~TextureManager(void)
{
}

void TextureManager::Init(ID3D11Device* device, ID3D11DeviceContext* dc)
{
	md3dDevice = device;
	mDC = dc;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> TextureManager::CreateTexture(std::string fileName)
{
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv = nullptr;

	// Find if texture already exists
	if (mTextureSRV.find(fileName) != mTextureSRV.end())
	{
		//srv = mLevelTextureSRV[fileName]; // Just point to existing texture
		srv = mTextureSRV[fileName].Get(); // Just point to existing texture
	}

	// Otherwise create the new texture
	else
	{
		std::wstring path(fileName.begin(), fileName.end());

		HRESULT hr;

		// Try loading the texture as dds
		hr = D3DX11CreateShaderResourceViewFromFile(md3dDevice.Get(), path.c_str(), nullptr, nullptr,srv.GetAddressOf(),nullptr);

		// Texture loading still failed, format either unsupported or file doesn't exist
		if (srv == NULL || FAILED(hr))
		{
			std::wostringstream ErrorStream;
			ErrorStream << "Failed to load texture " << path;
			MessageBox(0, ErrorStream.str().c_str(), 0, 0);
		}

		mTextureSRV[fileName] = srv.Get();
	}
	return srv;
}

void TextureManager::DeleteTexture(ID3D11ShaderResourceView * srv)
{
	srv = nullptr;
}

