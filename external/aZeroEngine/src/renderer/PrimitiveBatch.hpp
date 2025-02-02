#pragma once
#include "graphics_api/D3D12Include.hpp"
#include <vector>

namespace aZero
{
	namespace Rendering
	{
		class PrimitiveBatch
		{
		public:
			enum class PrimitiveType { INVALID, POINT, LINE, TRIANGLE };
			enum class RenderLayer { INVALID, NO_DEPTH, DEPTH };

			struct Point
			{
				DXM::Vector3 Position;
				DXM::Vector3 Color;
			};

			using PrimitiveList = std::vector<Point>;

		private:
			PrimitiveList m_Points;
			PrimitiveType m_Type = PrimitiveType::INVALID;
			RenderLayer m_Layer = RenderLayer::INVALID;

		public:
			PrimitiveBatch() = default;

			PrimitiveBatch(const PrimitiveList& Points, PrimitiveType Type, RenderLayer Layer)
				:m_Points(Points), m_Type(Type), m_Layer(Layer) { }

			PrimitiveBatch(PrimitiveList&& Points, PrimitiveType Type, RenderLayer Layer)
				:m_Points(Points), m_Type(Type), m_Layer(Layer) { }

			void SetLayer(RenderLayer Layer)
			{
				m_Layer = Layer;
			}

			void SetPrimitiveType(PrimitiveType Type)
			{
				m_Type = Type;
			}

			void AddPoints(std::same_as<Point> auto&&...Points)
			{
				(m_Points.emplace_back(Points), ...);
			}

			void AddPoints(std::same_as<Point> auto&...Points)
			{
				(m_Points.emplace_back(Points), ...);
			}

			void AddPoints(std::vector<Point>&& Points)
			{
				for (Point& P : Points)
				{
					m_Points.emplace_back(std::move(P));
				}
			}

			uint32_t GetNumBytes() const
			{
				return m_Points.size() * sizeof(Point);
			}

			uint32_t GetNumPoints() const
			{
				return m_Points.size();
			}

			void* GetData() const
			{
				return (void*)m_Points.data();
			}

			RenderLayer GetLayer() const
			{
				return m_Layer;
			}

			D3D_PRIMITIVE_TOPOLOGY GetPrimitiveType() const
			{
				switch (m_Type)
				{
					case PrimitiveType::POINT:
					{
						return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
					}
					case PrimitiveType::LINE:
					{
						return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
					}
					case PrimitiveType::TRIANGLE:
					{
						return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					}
					default:
					{
						break;
					}
				}

				DEBUG_PRINT("Invalid primitive type for batch");
				return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
			}

			bool IsDrawable() const
			{
				return m_Type != PrimitiveType::INVALID && m_Layer != RenderLayer::INVALID && m_Points.size() > 0;
			}
		};
	}
}