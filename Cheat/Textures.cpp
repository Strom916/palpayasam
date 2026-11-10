#include "main.h"
#include "Frame.h"

CTextures* pTextures;

CTextures::CTextures(IDirect3DDevice9* pDevice)
{
	tFrame = nullptr;
	D3DXCreateTextureFromFileInMemoryEx(pDevice, &byteLogo, sizeof(byteLogo), 990, 330, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &tLogo);
	D3DXCreateTextureFromFileInMemoryEx(pDevice, byteFrame, sizeof(byteFrame), 1672, 941, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &tFrame);
}

CTextures::~CTextures()
{
	tLogo->Release();
	if (tFrame)
		tFrame->Release();
}
