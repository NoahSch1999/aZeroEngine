#pragma once
#include "render_api/D3D12Include.hpp"
#include <algorithm>

namespace aZero
{
	namespace Helper
	{
		template<typename ValueType>
		std::string HandleNameCollision(const std::string& name, const std::unordered_map<std::string, ValueType>& map)
		{
			std::string tempName;
			while (map.count(name) > 0)
			{
				uint32_t incr = 0;
				tempName = name + "_" + std::to_string(incr);
				incr++;
			}

			if (tempName.length() > 0)
			{
				return tempName;
			}

			return name;
		}

		inline std::string GetPathSuffix(const std::string& filePath)
		{
			const size_t lastDot = filePath.find_last_of('.');
			return filePath.substr(lastDot + 1, filePath.length() - (filePath.length() - lastDot));
		}

		inline std::string GetFilenameFromPath(const std::string& filePath)
		{
			const size_t lastSlash = filePath.find_last_of('/');
			if (lastSlash != std::wstring::npos)
			{
				return filePath.substr(lastSlash + 1, filePath.length() - lastSlash);
			}
			return "";
		}

		inline std::string StripSuffixFromFilePath(const std::string& filePath)
		{
			const size_t lastDot = filePath.find_last_of('.');
			return filePath.substr(0, lastDot);
		}

		inline uint32_t Pack8To32(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
		{
			return 
				(static_cast<uint32_t>(d) << 24) |
				(static_cast<uint32_t>(c) << 16) |
				(static_cast<uint32_t>(b) << 8) |
				(static_cast<uint32_t>(a));
		}

		inline std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> Unpack32To8(uint32_t value32Bit)
		{
			return std::make_tuple(static_cast<uint8_t>(value32Bit & 0xFFFF), static_cast<uint8_t>((value32Bit >> 8) & 0xFFFF), static_cast<uint8_t>((value32Bit >> 16) & 0xFFFF), static_cast<uint8_t>((value32Bit >> 24) & 0xFFFF));
		}

		inline uint32_t Pack16To32(uint16_t low, uint16_t high) {
			return (static_cast<uint32_t>(high) << 16) | low;
		}

		inline std::tuple<uint16_t, uint16_t> Unpack32To16(uint32_t value32Bit)
		{
			return std::make_tuple(static_cast<uint16_t>(value32Bit & 0xFFFF), static_cast<uint16_t>((value32Bit >> 16) & 0xFFFF));
		}

		inline DirectX::BoundingSphere ComputeBoundingSphere(const std::vector<DXM::Vector3>& points)
		{
			DXM::Vector3 p0 = points[0];

			int i1 = 0;
			float maxDist = 0.0f;

			for (int i = 0; i < points.size(); i++)
			{
				float d = (points[i] - p0).LengthSquared();
				if (d > maxDist)
				{
					maxDist = d;
					i1 = i;
				}
			}

			int i2 = i1;
			maxDist = 0.0f;

			for (int i = 0; i < points.size(); i++)
			{
				float d = (points[i] - points[i1]).LengthSquared();
				if (d > maxDist)
				{
					maxDist = d;
					i2 = i;
				}
			}

			DXM::Vector3 center = (points[i1] + points[i2]) * 0.5f;
			float radius = (points[i2] - center).Length();

			for (const auto& p : points)
			{
				DXM::Vector3 d = p - center;
				float dist = d.Length();

				if (dist > radius)
				{
					float newRadius = (radius + dist) * 0.5f;
					float k = (newRadius - radius) / dist;

					center += d * k;
					radius = newRadius;
				}
			}

			return { DXM::Vector3(center.x, center.y, center.z), radius };
		}

		inline DXM::Vector2 EncodeNormalOctahedral(const DXM::Vector3& n)
		{
			float invL1 = 1.0f / (std::fabs(n.x) + std::fabs(n.y) + std::fabs(n.z));
			float x = n.x * invL1;
			float y = n.y * invL1;
			float z = n.z * invL1;

			DXM::Vector2 enc = { x, y };

			if (z < 0.0f)
			{
				float oldX = enc.x;
				float oldY = enc.y;

				enc.x = (1.0f - std::fabs(oldY)) * (oldX >= 0.0f ? 1.0f : -1.0f);
				enc.y = (1.0f - std::fabs(oldX)) * (oldY >= 0.0f ? 1.0f : -1.0f);
			}

			return enc;
		}
	}
}