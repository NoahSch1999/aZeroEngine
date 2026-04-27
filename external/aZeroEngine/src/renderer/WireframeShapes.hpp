#pragma once
#include "graphics_api/D3D12Include.hpp"

namespace aZero
{
    namespace Rendering
    {
        namespace WireframeShape
        {
            struct LineVertex
            {
                DXM::Vector3 Position;
                DXM::Vector3 Color;
            };

            struct Line
            {
                LineVertex Start;
                LineVertex End;

                Line() = default;
                Line(const DXM::Vector3& start, const DXM::Vector3& end, const DXM::Vector3& color)
                    :Start({ start, color }), End({ end, color })
                {

                }
            };

            struct BaseShape
            {
            public:
                BaseShape() = default;
                std::vector<Line> GetLines() const { return m_Lines; }

            protected:
                std::vector<Line> m_Lines;
            };

            struct Sphere : public BaseShape
            {
                Sphere() = default;
                Sphere(const DXM::Vector3& color, const DXM::Vector3& center, float radius, uint32_t detail)
                {
                    // TODO: Impl
                    const float PI = 3.1415;
                    for (uint32_t i = 1; i < detail; i++)
                    {
                        const float phi = PI * i / detail;
                        for (uint32_t j = 0; j < detail; j++)
                        {
                            const float theta1 = 2.0f * PI * j / detail;
                            const float theta2 = 2.0f * PI * (j + 1) / detail;

                            Line line;
                            line.Start.Position = DXM::Vector3(center.x + radius * sin(phi) * cos(theta1), center.y + radius * cos(phi), center.z + radius * sin(phi) * sin(theta1));
                            line.Start.Color = color;

                            line.End.Position = DXM::Vector3(center.x + radius * sin(phi) * cos(theta2), center.y + radius * cos(phi), center.z + radius * sin(phi) * sin(theta2));
                            line.End.Color = color;

                            m_Lines.emplace_back(line);
                        }
                    }
                }
            };

            struct AABB : public BaseShape
            {
                AABB() = default;
                AABB(const DXM::Vector3& color, const DXM::Vector3& center, const DXM::Vector3& halfExtents)
                {
                    std::array<DXM::Vector3, 8> points;
                    // Top verts
                    points[0] = DXM::Vector3(halfExtents.x, halfExtents.y, halfExtents.z);
                    points[1] = DXM::Vector3(halfExtents.x, halfExtents.y, -halfExtents.z);
                    points[2] = DXM::Vector3(-halfExtents.x, halfExtents.y, -halfExtents.z);
                    points[3] = DXM::Vector3(-halfExtents.x, halfExtents.y, halfExtents.z);

                    // Bottom verts
                    points[4] = DXM::Vector3(-halfExtents.x, -halfExtents.y, halfExtents.z);
                    points[5] = DXM::Vector3(halfExtents.x, -halfExtents.y, halfExtents.z);
                    points[6] = DXM::Vector3(halfExtents.x, -halfExtents.y, -halfExtents.z);
                    points[7] = DXM::Vector3(-halfExtents.x, -halfExtents.y, -halfExtents.z);

                    DXM::Matrix world = DXM::Matrix::CreateTranslation(center);
                    for (auto& p : points)
                    {
                        DXM::Vector3 temp = DXM::Vector3::Transform(p, world);
                        p = temp;
                    }

                    m_Lines.emplace_back(Line(points[0], points[1], color));
                    m_Lines.emplace_back(Line(points[1], points[2], color));
                    m_Lines.emplace_back(Line(points[2], points[3], color));
                    m_Lines.emplace_back(Line(points[3], points[0], color));
                    m_Lines.emplace_back(Line(points[4], points[5], color));
                    m_Lines.emplace_back(Line(points[5], points[6], color));
                    m_Lines.emplace_back(Line(points[6], points[7], color));
                    m_Lines.emplace_back(Line(points[7], points[4], color));
                    m_Lines.emplace_back(Line(points[3], points[4], color));
                    m_Lines.emplace_back(Line(points[0], points[5], color));
                    m_Lines.emplace_back(Line(points[1], points[6], color));
                    m_Lines.emplace_back(Line(points[2], points[7], color));
                }
            };

            struct OBB : public BaseShape
            {
                OBB() = default;
                OBB(const DXM::Vector3& color, const DXM::Vector3& center, const DXM::Quaternion& rotation, const DXM::Vector3& halfExtents)
                {
                    std::array<DXM::Vector3, 8> points;
                    // Top verts
                    points[0] = DXM::Vector3(halfExtents.x, halfExtents.y, halfExtents.z);
                    points[1] = DXM::Vector3(halfExtents.x, halfExtents.y, -halfExtents.z);
                    points[2] = DXM::Vector3(-halfExtents.x, halfExtents.y, -halfExtents.z);
                    points[3] = DXM::Vector3(-halfExtents.x, halfExtents.y, halfExtents.z);

                    // Bottom verts
                    points[4] = DXM::Vector3(-halfExtents.x, -halfExtents.y, halfExtents.z);
                    points[5] = DXM::Vector3(halfExtents.x, -halfExtents.y, halfExtents.z);
                    points[6] = DXM::Vector3(halfExtents.x, -halfExtents.y, -halfExtents.z);
                    points[7] = DXM::Vector3(-halfExtents.x, -halfExtents.y, -halfExtents.z);
                    DXM::Matrix world = DXM::Matrix::CreateTranslation(center);
                    DXM::Matrix::Transform(DXM::Matrix::CreateTranslation(center), rotation, world);
                    for (auto& p : points)
                    {
                        DXM::Vector3 temp = DXM::Vector3::Transform(p, world);
                        p = temp;
                    }
                    m_Lines.emplace_back(Line(points[0], points[1], color));
                    m_Lines.emplace_back(Line(points[1], points[2], color));
                    m_Lines.emplace_back(Line(points[2], points[3], color));
                    m_Lines.emplace_back(Line(points[3], points[0], color));
                    m_Lines.emplace_back(Line(points[4], points[5], color));
                    m_Lines.emplace_back(Line(points[5], points[6], color));
                    m_Lines.emplace_back(Line(points[6], points[7], color));
                    m_Lines.emplace_back(Line(points[7], points[4], color));
                    m_Lines.emplace_back(Line(points[3], points[4], color));
                    m_Lines.emplace_back(Line(points[0], points[5], color));
                    m_Lines.emplace_back(Line(points[1], points[6], color));
                    m_Lines.emplace_back(Line(points[2], points[7], color));
                }
            };

            struct Frustum
            {
                // TODO
            };
        }
    }
}