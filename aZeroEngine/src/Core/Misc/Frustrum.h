#pragma once
#include "Core/D3D12Include.h"

namespace aZero
{
	namespace Shapes
	{
		struct Plane
		{
			DXM::Vector3 Normal;
			float Distance;

			float GetSignedDistanceToPlane(const DXM::Vector3& Point) const
			{
				float Dot = Normal.DXM::Vector3::Dot(Point);
				return Dot - Distance;
			}
		};

		class Frustum
		{
		private:
			Plane Top;
			Plane Bottom;
			Plane Right;
			Plane Left;
			Plane Far;
			Plane Near;

			bool IsOnOrForwardPlane(Plane InPlane, const DXM::Vector3& Point, float Radius) const
			{
				return InPlane.GetSignedDistanceToPlane(Point) > -Radius;
			}

		public:
			Frustum() = default;

			Frustum(
				const DXM::Vector3& Position,
				const DXM::Vector3& Forward,
				const DXM::Vector3& Right,
				float NearDist,
				float FarDist,
				float Fov,
				float AspectRatio
				)
			{
				Frustum Frustum;
				const float HalfVSide = FarDist * tanf(Fov * 0.5f);
				const float HalfHSide = HalfVSide * AspectRatio;

				const DXM::Vector3 Up = Forward.Cross(Right);
				const DXM::Vector3 Far = Forward * FarDist;

				DXM::Vector3 FarNormal = Forward;
				float FarDistance = abs(FarNormal.Dot(Position + Far));
				Frustum.Far = { -FarNormal, FarDistance };

				DXM::Vector3 NearNormal = Forward;
				float NearDistance = abs(NearNormal.Dot(Position + Forward * NearDist));
				Frustum.Near = { NearNormal, NearDistance };

				DXM::Vector3 idk = Far + Right * HalfHSide;
				idk.Normalize();
				DXM::Vector3 RightNormal = idk.Cross(Up);
				float RightDist = abs(RightNormal.Dot(Position));
				RightNormal.x = -RightNormal.x;
				Frustum.Right = { RightNormal, RightDist };

				idk = Far - Right * HalfHSide;
				idk.Normalize();
				DXM::Vector3 LeftNormal = Up.Cross(idk);
				float LeftDist = abs(LeftNormal.Dot(Position));
				LeftNormal.x = -LeftNormal.x;
				Frustum.Left = { LeftNormal, LeftDist };

				idk = Far + Up * HalfVSide;
				idk.Normalize();
				DXM::Vector3 TopNormal = Right.Cross(idk);
				float TopDist = abs(TopNormal.Dot(Position));
				Frustum.Top = { TopNormal, TopDist };

				idk = Far -Up * HalfVSide;
				idk.Normalize();
				DXM::Vector3 BottomNormal = idk.Cross(Right);
				float BottomDist = abs(BottomNormal.Dot(Position));
				Frustum.Bottom = { BottomNormal, BottomDist };

				*this = Frustum;
			}

			bool IsCollidingSphere(const DXM::Vector3& Position, float Radius) const
			{
				bool LeftCheck = IsOnOrForwardPlane(Left, Position, Radius);
				bool RightCheck = IsOnOrForwardPlane(Right, Position, Radius);
				bool FarCheck = IsOnOrForwardPlane(Far, Position, Radius);
				bool NearCheck = IsOnOrForwardPlane(Near, Position, Radius);
				bool TopCheck = IsOnOrForwardPlane(Top, Position, Radius);
				bool BottomCheck = IsOnOrForwardPlane(Bottom, Position, Radius);
				return LeftCheck && RightCheck && FarCheck && NearCheck && TopCheck && BottomCheck;
				/*return (IsOnOrForwardPlane(Left, Position, Radius) &&
					IsOnOrForwardPlane(Right, Position, Radius) &&
					IsOnOrForwardPlane(Far, Position, Radius) &&
					IsOnOrForwardPlane(Near, Position, Radius) &&
					IsOnOrForwardPlane(Top, Position, Radius) &&
					IsOnOrForwardPlane(Bottom, Position, Radius));*/
			}
		};
	}
}