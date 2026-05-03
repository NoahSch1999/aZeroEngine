#pragma once
#include "misc/Maths.hpp"
#include "WireframeShapes.hpp"
#include "WinPixEventRuntime/pix3.h"
#include "pipeline/pass/VertexShaderPass.hpp"

namespace aZero
{
    namespace ECS { class CameraComponent; }
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
            WireframeRenderer(ID3D12DeviceX* device, IDxcCompilerX& compiler);

            template<typename Shape>
            void AddShape(const Shape& shape)
            {
                for (const auto& line : shape.m_Lines)
                {
                    memcpy((char*)m_VertexBuffers[m_FrameIndex].GetCPUAccessibleMemory() + m_VertCount * sizeof(WireframeShape::LineVertex), &line, sizeof(line));
                    m_VertCount += 2;
                }
            }

        private:
            void BeginFrame(uint32_t frameIndex)  { m_VertCount = 0; m_FrameIndex = frameIndex; }

            void Render(RenderAPI::CommandList& cmdList, RenderAPI::DescriptorHeap& resourceHeap, RenderAPI::DescriptorHeap& samplerHeap, const ECS::CameraComponent& camera, RenderTarget& rtv, DepthStencilTarget& dsv);

            Pipeline::VertexShaderPass m_Pass;

            std::array<RenderAPI::Buffer, 3> m_VertexBuffers;
            D3D12_VERTEX_BUFFER_VIEW m_VBView;
            uint32_t m_VertCount = 0;
            uint32_t m_FrameIndex = 0;
        };
    }
}
