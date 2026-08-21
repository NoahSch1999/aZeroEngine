#pragma once
#include "render_api/D3D12Include.hpp"
#include "physics/Body.hpp"

namespace aZero
{
	namespace Math
	{
		static inline float ToDegree(float radian)
		{
			float degree = radian * (180.f / 3.1415f); 
			degree = fmodf(degree, 360.0f); // Calculate remainder when dividing degree by 360. Ex: degree = 370 -> remainder is 10 which means we actually have rotated by +10 which is the same as +370 visually.
			if (degree < 0.0f) // If negative we add a "lap" so it stays positive.
				degree += 360.0f;
			return degree;
		}

		static inline float ToRadian(float degree)
		{
			return  degree * (3.1415f / 180.f);
		}

		static inline DXM::Vector3 PositiveZero(const DXM::Vector3& v)
		{
			DXM::Vector3 ret = v;
			if (ret.x == 0.f) {
				ret.x = 0.f;
			}
			if (ret.y == 0.f) {
				ret.y = 0.f;
			}
			if (ret.z == 0.f) {
				ret.z = 0.f;
			}
			return ret;
		}

		static inline DXM::Vector3 PositiveZero(float x, float y, float z)
		{
			return Math::PositiveZero(DXM::Vector3(x, y, z));
		}

		static inline DXM::Vector3 ToDegree(const DXM::Vector3& radians)
		{
			return DXM::Vector3(Math::ToDegree(radians.x), Math::ToDegree(radians.y), Math::ToDegree(radians.z));
		}

		static inline DXM::Vector3 ToRadian(const DXM::Vector3& degrees)
		{
			return DXM::Vector3(Math::ToRadian(degrees.x), Math::ToRadian(degrees.y), Math::ToRadian(degrees.z));
		}

		struct Matrix4x3
		{
			float m[4][3];
			Matrix4x3() = default;
			Matrix4x3(const DXM::Matrix& matrix)
			{
				m[0][0] = matrix.m[0][0];
				m[0][1] = matrix.m[0][1];
				m[0][2] = matrix.m[0][2];

				m[1][0] = matrix.m[1][0];
				m[1][1] = matrix.m[1][1];
				m[1][2] = matrix.m[1][2];

				m[2][0] = matrix.m[2][0];
				m[2][1] = matrix.m[2][1];
				m[2][2] = matrix.m[2][2];

				m[3][0] = matrix.m[3][0];
				m[3][1] = matrix.m[3][1];
				m[3][2] = matrix.m[3][2];
			}
		};

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