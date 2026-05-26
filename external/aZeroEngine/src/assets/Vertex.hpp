#pragma once

namespace aZero::Asset
{
	using Index = uint32_t;

	struct Vertex
	{
		DXM::Vector3 Position;
		DXM::Vector2 UV;
		DXM::Vector3 Normal;
		DXM::Vector3 Tangent;
	};
}