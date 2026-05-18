#pragma once
#include "misc/Maths.hpp"
#include "WireframeShapes.hpp"
#include "WinPixEventRuntime/pix3.h"
#include "pipeline/RenderPass.hpp"
#include "ecs/Components.hpp"
#include "graphics_api/resource/buffer/Buffer.hpp"

namespace aZero
{
    namespace Rendering
    {
        class Renderer;
        class RenderTarget;
        class DepthStencilTarget;

        class WireframeRenderer
        {
            friend class Renderer;
        public:
            WireframeRenderer() = default;
            WireframeRenderer(Rendering::Renderer& renderer, ID3D12DeviceX* device, IDxcCompilerX& compiler);

            template<typename Shape>
            void AddShape(const Shape& shape)
            {
                for (const auto& line : shape.m_Lines)
                {
                    memcpy((char*)m_VertexBuffers[m_FrameIndex].GetCPUAccessibleMemory() + m_VertCount * sizeof(WireframeShape::LineVertex), &line, sizeof(line));
                    m_VertCount += 2;
                }
            }

            void Render(const Component::Camera& camera, const Component::Position& cameraPosition, const Component::Rotation& cameraRotation, RenderTarget& rtv, DepthStencilTarget& dsv);

        private:
            void BeginFrame();

            //Pipeline::VertexShaderPass m_Pass;
            Pipeline::RenderPass m_Pass;

            std::array<RenderAPI::Buffer, 3> m_VertexBuffers;
            D3D12_VERTEX_BUFFER_VIEW m_VBView;
            uint32_t m_VertCount = 0;
            uint32_t m_FrameIndex = 0;

            Rendering::Renderer* m_diRenderer;
        };
    }
}
