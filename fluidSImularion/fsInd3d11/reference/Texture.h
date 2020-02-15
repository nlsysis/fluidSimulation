#pragma once
#include "..\\common\d3dUtil.h"
#include <wrl/client.h>
#include <map>

class TextureManager
{
public:
	TextureManager();
	~TextureManager();

	void Init(ID3D11Device* device, ID3D11DeviceContext* dc);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateTexture(std::string fileName);
	void DeleteTexture(ID3D11ShaderResourceView* srv);
private:
	Microsoft::WRL::ComPtr<ID3D11Device> md3dDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> mDC;

	std::map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> mTextureSRV;
};