#include "CPUTexture.h"
#include <d3dcompiler.h>

using namespace DirectX;

CPUTexture::CPUTexture(
	unsigned int width,
	unsigned int height,
	Microsoft::WRL::ComPtr<ID3D11Device> device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context)
	:
	width(width),
	height(height),
	device(device),
	context(context),
	pixelColors{}
{
	// Perform a resize to create the initial data
	Resize(width, height);
	
	// Compile necessary shaders into bytecode
	ID3DBlob* vsBlob;
	ID3DBlob* psBlob;
	D3DCompile(copyVSCode, strlen(copyVSCode), 0, 0, 0,	"main",	"vs_5_0", 0, 0, &vsBlob, 0);
	D3DCompile(copyPSCode, strlen(copyPSCode), 0, 0, 0, "main", "ps_5_0", 0, 0, &psBlob, 0);

	// Create D3D shaders from bytecode
	device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), 0, copyVS.GetAddressOf());
	device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), 0, copyPS.GetAddressOf());

	// Create a sampler state for copy
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	device->CreateSamplerState(&sampDesc, sampler.GetAddressOf());
}


CPUTexture::~CPUTexture()
{
	delete[] pixelColors;
}


void CPUTexture::Resize(unsigned int width, unsigned int height)
{
	// Grab new data
	this->width = width;
	this->height = height;

	// Reset resources
	copyTexture.Reset();
	copyTextureSRV.Reset();
	if (pixelColors) { delete[] pixelColors; }

	// Create the dynamic texture for uploading
	D3D11_TEXTURE2D_DESC copyDesc = {};
	copyDesc.ArraySize = 1;
	copyDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	copyDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	copyDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // Need to be able to copy to GPU quickly, so this must match our CPU data type
	copyDesc.Height = height;
	copyDesc.MipLevels = 1;
	copyDesc.MiscFlags = 0;
	copyDesc.SampleDesc.Count = 1;
	copyDesc.SampleDesc.Quality = 0;
	copyDesc.Usage = D3D11_USAGE_DYNAMIC;
	copyDesc.Width = width;
	device->CreateTexture2D(&copyDesc, 0, copyTexture.GetAddressOf());

	// Make a default SRV
	device->CreateShaderResourceView(copyTexture.Get(), 0, copyTextureSRV.GetAddressOf());

	// Re-created pixel grid
	pixelColors = new XMFLOAT4[width * height];
	ClearFast();
}


void CPUTexture::Draw()
{
	// Copy the pixels to the actual texture
	D3D11_MAPPED_SUBRESOURCE map = {};
	context->Map(copyTexture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
	memcpy(map.pData, pixelColors, sizeof(XMFLOAT4) * width * height);
	context->Unmap(copyTexture.Get(), 0);

	// Perform a quick render to copy from our float texture to the back buffer
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->VSSetShader(copyVS.Get(), 0, 0);
	context->PSSetShader(copyPS.Get(), 0, 0);
	context->PSSetShaderResources(0, 1, copyTextureSRV.GetAddressOf());
	context->PSSetSamplers(0, 1, sampler.GetAddressOf());
	context->Draw(3, 0);
	
}

unsigned int CPUTexture::GetWidth() { return width; }
unsigned int CPUTexture::GetHeight() { return height; }

void CPUTexture::Clear(DirectX::XMFLOAT4 color)
{
	unsigned int pixelCount = width * height;
	for (unsigned int i = 0; i < pixelCount; i++)
		memcpy(&pixelColors[i], &color, sizeof(XMFLOAT4));
}


void CPUTexture::ClearFast()
{
	memset(pixelColors, 0, sizeof(XMFLOAT4) * width * height);
}


void CPUTexture::SetColor(unsigned int x, unsigned int y, DirectX::XMFLOAT4 color)
{
	memcpy(&pixelColors[PixelIndex(x, y)], &color, sizeof(XMFLOAT4));
}


void CPUTexture::AddColor(unsigned int x, unsigned int y, DirectX::XMFLOAT4 color)
{
	unsigned int index = PixelIndex(x, y);
	XMVECTOR p = XMLoadFloat4(&pixelColors[index]);
	XMVECTOR c = XMLoadFloat4(&color);
	XMStoreFloat4(&pixelColors[index], XMVectorAdd(p, c));
}


unsigned int CPUTexture::PixelIndex(unsigned int x, unsigned int y)
{
	return y * width + x;
}
