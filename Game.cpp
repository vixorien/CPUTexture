#include "Game.h"
#include "Input.h"
#include "Helpers.h"
#include "Sphere.h"

#include <stdlib.h>	// Random
#include <time.h>	// For seeding random

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

// Helper macro for getting a float between min and max
#define RandomRange(min, max) (float)rand() / RAND_MAX * (max - min) + min

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// Direct3D itself, and our window, are not ready at this point!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,			// The application's handle
		L"DirectX Game",	// Text for the window's title bar (as a wide-character string)
		1280,				// Width of the window's client area
		720,				// Height of the window's client area
		false,				// Sync the framerate to the monitor refresh? (lock framerate)
		true),				// Show extra stats (fps) in title bar?
	resolutionReduction(4)
{
#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Delete all objects manually created within this class
//  - Release() all Direct3D objects created within this class
// --------------------------------------------------------
Game::~Game()
{
	// Call delete or delete[] on any objects or arrays you've
	// created using new or new[] within this class
	// - Note: this is unnecessary if using smart pointers

	// Call Release() on any Direct3D objects made within this class
	// - Note: this is unnecessary for D3D objects stored in ComPtrs
}

// --------------------------------------------------------
// Called once per program, after Direct3D and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Seed random with current time
	srand((unsigned int)time(0));

	cpuTexture = std::make_shared<CPUTexture>(windowWidth / resolutionReduction, windowHeight / resolutionReduction, device, context);
	cpuTexture->Clear({ 1,0,1,1 });

	camera = std::make_shared<Camera>(
		0.0f, 0.0f, -5.0f,
		5.0f,
		0.001f,
		XM_PIDIV4,
		(float)windowWidth / windowHeight);

	// Create random spheres
	for (int i = 0; i < 5; i++)
	{
		// Create a random sphere (pos, radius)
		Sphere s(
			{
				RandomRange(-5, 5),
				RandomRange(-5, 5),
				RandomRange(0, 20)
			},
			RandomRange(0.1f, 3.0f),
			{
				RandomRange(0.1f, 1.0f),
				RandomRange(0.1f, 1.0f),
				RandomRange(0.1f, 1.0f),
				1
			});

		spheres.push_back(s);
	}

	Sphere s(
		{ 0, -1000, 0 }, 
		995,
		{ 0.1f, 1.0f, 0.3f, 1 });
	spheres.push_back(s);
}


// --------------------------------------------------------
// Handle resizing to match the new window size.
//  - DXCore needs to resize the back buffer
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();

	cpuTexture->Resize(windowWidth / resolutionReduction, windowHeight / resolutionReduction);

	camera->UpdateProjectionMatrix((float)windowWidth / windowHeight);
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE))
		Quit();

	camera->Update(deltaTime);

	int max_recursion = 5;

	// Loop through all pixels and raycast
	unsigned int raycastWidth = cpuTexture->GetWidth();
	unsigned int raycastHeight = cpuTexture->GetHeight();
	for (unsigned int y = 0; y < raycastHeight; y++)
	{
		for (unsigned int x = 0; x < raycastWidth; x++)
		{
			Ray ray = camera->GetRayThroughPixel((float)x, (float)y, raycastWidth, raycastHeight);
			cpuTexture->SetColor(x, y, TraceRay(ray, max_recursion));
		}
	}

}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Frame START
	// - These things should happen ONCE PER FRAME
	// - At the beginning of Game::Draw() before drawing *anything*
	{
		// Clear the back buffer (erases what's on the screen)
		const float bgColor[4] = { 0.4f, 0.6f, 0.75f, 1.0f }; // Cornflower Blue
		context->ClearRenderTargetView(backBufferRTV.Get(), bgColor);

		// Clear the depth buffer (resets per-pixel occlusion information)
		context->ClearDepthStencilView(depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	// Put the texture onto the back buffer
	cpuTexture->Draw();

	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	{
		// Present the back buffer to the user
		//  - Puts the results of what we've drawn onto the window
		//  - Without this, the user never sees anything
		bool vsyncNecessary = vsync || !deviceSupportsTearing || isFullscreen;
		swapChain->Present(
			vsyncNecessary ? 1 : 0,
			vsyncNecessary ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Must re-bind buffers after presenting, as they become unbound
		context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	}
}

DirectX::XMFLOAT4 Game::TraceRay(Ray ray, int depth)
{
	// Too deep?
	if (depth <= 0)
		return XMFLOAT4(0, 0, 0, 0);

	// See if we hit any spheres
	bool hit = false;
	Sphere closestSphere;
	float closestDist = D3D11_FLOAT32_MAX;
	for (int s = 0; s < spheres.size(); s++)
	{
		float hitDist = 0;
		if (spheres[s].Bounds.Intersects(
			XMLoadFloat3(&ray.Origin),
			XMLoadFloat3(&ray.Direction),
			hitDist))
		{
			if (hitDist < closestDist)
			{
				closestSphere = spheres[s];
				closestDist = hitDist;
				hit = true;
			}
		}
	}

	// Any hits?
	if (hit)
	{
		// Calculate normal of sphere at hit point
		XMVECTOR rayDir = XMLoadFloat3(&ray.Direction);
		XMVECTOR hitPoint = XMLoadFloat3(&ray.Origin) + rayDir * closestDist;
		XMVECTOR sphereCenter = XMLoadFloat3(&closestSphere.Bounds.Center);
		XMVECTOR hitNormal = XMVector3Normalize(hitPoint - sphereCenter);

		XMVECTOR reflection = XMVector3Reflect(rayDir, hitNormal);

		// Reflection ray
		Ray reflRay = {};
		XMStoreFloat3(&reflRay.Origin, hitPoint + hitNormal * 0.0001f); // Don't hit ourselves
		XMStoreFloat3(&reflRay.Direction, reflection);

		// Recursively trace the ray
		XMFLOAT4 bounceColor = TraceRay(reflRay, depth - 1);
		
		// Combine colors and return
		XMVECTOR b = XMLoadFloat4(&bounceColor);
		XMVECTOR c = XMLoadFloat4(&closestSphere.Color);
		XMFLOAT4 result;
		XMStoreFloat4(&result, b * c);

		return result;
	}

	// No hit
	return XMFLOAT4(1,1,1,1);
}
