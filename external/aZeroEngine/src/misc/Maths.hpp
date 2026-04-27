#pragma once
#include "graphics_api/D3D12Include.hpp"
#include "physics/JoltInclude.hpp"

namespace aZero
{
	namespace Math
	{
		// TO DXM
		static inline DXM::Vector3 Convert(const JPH::Vec3& other) { return DXM::Vector3(other.GetX(), other.GetY(), other.GetZ()); }
		static inline DXM::Vector4 Convert(const JPH::Vec4& other) { return DXM::Vector4(other.GetX(), other.GetY(), other.GetZ(), other.GetW()); }

		static inline DXM::Quaternion Convert(const JPH::Quat& other) { return DXM::Quaternion(Convert(other.GetXYZW())); }

		static inline DXM::Matrix Convert(const JPH::Mat44& other)
		{
			return DXM::Matrix(
				Convert(other.GetColumn4(0)),
				Convert(other.GetColumn4(1)),
				Convert(other.GetColumn4(2)),
				Convert(other.GetColumn4(3))
			);
		}

		// TO JOLT
		static inline JPH::Vec3 Convert(const DXM::Vector3& other) { return JPH::Vec3(other.x, other.y, other.z); }
		static inline JPH::Vec4 Convert(const DXM::Vector4& other) { return JPH::Vec4(other.x, other.y, other.z, other.w); }

		static inline JPH::Quat Convert(const DXM::Quaternion& other) { return JPH::Quat(other.x, other.y, other.z, other.w); }

		static inline JPH::Mat44 Convert(const DXM::Matrix& other) 
		{ 
			return JPH::Mat44(
				JPH::Vec4(other.m[0][0], other.m[0][1], other.m[0][2], other.m[0][3]),
				JPH::Vec4(other.m[1][0], other.m[1][1], other.m[1][2], other.m[1][3]),
				JPH::Vec4(other.m[2][0], other.m[2][1], other.m[2][2], other.m[2][3]),
				JPH::Vec4(other.m[3][0], other.m[3][1], other.m[3][2], other.m[3][3])
				); 
		}
	}
}