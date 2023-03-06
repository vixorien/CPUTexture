#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

class CPUTexture
{
public:

	CPUTexture(
		unsigned int width, 
		unsigned int height,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
	~CPUTexture();
	
	// Altering data
	void Clear(DirectX::XMFLOAT4 color);
	void ClearFast();
	void SetColor(unsigned int x, unsigned int y, DirectX::XMFLOAT4 color);
	void AddColor(unsigned int x, unsigned int y, DirectX::XMFLOAT4 color);

	// GPU calls
	void Resize(unsigned int width, unsigned int height);
	void Draw();

	// Getters for current size
	unsigned int GetWidth();
	unsigned int GetHeight();

private:

	// Helper for 2D indices to 1D index
	unsigned int PixelIndex(unsigned int x, unsigned int y);

	// CPU-side color data
	unsigned int width;
	unsigned int height;
	DirectX::XMFLOAT4* pixelColors;

	// General D3D
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;

	// Resources for copying
	Microsoft::WRL::ComPtr<ID3D11Texture2D> copyTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> copyTextureSRV;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
	
	// Shaders for quick copy via rendering pipeline
	Microsoft::WRL::ComPtr<ID3D11VertexShader> copyVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> copyPS;

	// Vertex shader code 
	// - To be compiled at run time
	// - Full-screen triangle from 3 verts
	const char* copyVSCode = 
	R"(
		struct VS2PS
		{
			float4 pos : SV_POSITION;
			float2 uv  : TEXCOORD;
		};

		VS2PS main(uint id : SV_VertexID)
		{
			VS2PS output;
			output.uv = float2((id << 1) & 2, id & 2);

            output.pos = float4(output.uv, 0, 1);
			output.pos.x = output.pos.x * 2 - 1;
			output.pos.y = output.pos.y * -2 + 1;
			
			return output;
		}

	)";

	// Pixel shader code
	// - To be compiled at run time
	// - Copies pixels from texture (mip level 0)
	const char* copyPSCode = 
	R"(
		Texture2D Pixels          : register(t0);
		SamplerState PointSampler : register(s0);

		float4 main(float4 position : SV_POSITION, float2 uv : TEXCOORD) : SV_TARGET
		{
			return Pixels.Sample(PointSampler, uv);
		}
	)";
};

