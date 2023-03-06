#pragma once

#include <DirectXCollision.h>
#include <DirectXMath.h>

struct Sphere
{
	DirectX::BoundingSphere Bounds;
	DirectX::XMFLOAT4 Color;

	Sphere() : Bounds(), Color() { }

	Sphere(DirectX::XMFLOAT3 center, float radius, DirectX::XMFLOAT4 color) :
		Bounds(center, radius),
		Color(color)
	{}
};