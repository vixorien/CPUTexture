#pragma once

#include "DXCore.h"
#include "CPUTexture.h"
#include "Camera.h"
#include "Ray.h"
#include "Sphere.h"

#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <memory>
#include <vector>


class Game 
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

private:

	DirectX::XMFLOAT4 TraceRay(Ray ray, int depth);

	std::shared_ptr<Camera> camera;
	std::vector<Sphere> spheres;

	// Resources for putting data into our texture
	std::shared_ptr<CPUTexture> cpuTexture;

	unsigned int resolutionReduction;
	
};

