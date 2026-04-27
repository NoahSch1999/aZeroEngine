#pragma once
#include "Renderer.hpp"
#include "misc/Maths.hpp"
#include "WireframeShapes.hpp"
#include <numbers>

namespace aZero
{
    namespace Rendering
    {
        class WireframeRenderer
        {
        public:
            WireframeRenderer() = default;
            WireframeRenderer(ID3D12DeviceX* device, IDxcCompilerX& compiler)
            {
                Pipeline::VertexShader vs(compiler, PROJECT_DIRECTORY + std::string("shaderSource/DebugLine.vs.hlsl"));
                Pipeline::PixelShader ps(compiler, PROJECT_DIRECTORY + std::string("shaderSource/DebugLine.ps.hlsl"));
                Pipeline::VertexShaderPass::Description desc;
                Pipeline::MultiShaderPassDesc::RenderTarget rt;
                rt.m_Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
                rt.m_Name = "COLOR";
                desc.m_RenderTargets.push_back(rt);
                desc.m_DepthStencil.m_Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
                desc.m_TopologyType = Pipeline::VertexShaderPass::LINE;
                m_Pass.Compile(device, desc, vs, &ps);

                for (int i = 0; i < 3; i++)
                {
                    m_VertexBuffers[i] = RenderAPI::Buffer(device, RenderAPI::Buffer::Desc(sizeof(WireframeShape::LineVertex) * 20000, D3D12_HEAP_TYPE_UPLOAD));
                }

                m_VBView.StrideInBytes = sizeof(WireframeShape::LineVertex);
                m_VBView.SizeInBytes = m_VertexBuffers[0].GetResource()->GetDesc().Width;
            }

            void BeginFrame(const Rendering::Renderer& renderer)
            {
                m_VertCount = 0;
                m_FrameIndex = renderer.GetFrameIndex();
            }

            void Render(Rendering::Renderer& renderer, const ECS::CameraComponent& camera, RenderTarget& rtv, DepthStencilTarget& dsv)
            {
                auto& frameContext = renderer.GetCurrentContext();
                auto& cmdList = frameContext.m_DirectCmdList;

                m_Pass.Begin(cmdList, renderer.GetResourceHeap(), renderer.GetSamplerHeap(), { &rtv.GetDescriptor() }, &dsv.GetDescriptor());

                auto binding = m_Pass.GetConstantBindingIndex("VP");
                struct Constants
                {
                    DXM::Matrix vp;
                }constants;

                constants.vp = camera.GetViewProjectionMatrix();
                cmdList.SetGraphicsRoot32BitConstantsSafe(binding.GetRootIndex(), binding.GetNumConstants(), &constants, 0);

                auto viewport = camera.GetViewport();
                cmdList->RSSetViewports(1, &viewport);
                auto rect = camera.GetScizzorRect();
                cmdList->RSSetScissorRects(1, &rect);

                m_VBView.BufferLocation = m_VertexBuffers[m_FrameIndex].GetResource()->GetGPUVirtualAddress();
                cmdList->IASetVertexBuffers(0, 1, &m_VBView);

                cmdList->DrawInstanced(m_VertCount, 1, 0, 0);
                renderer.GetGraphicsCommandQueue().ExecuteCommandList(cmdList);
            }

            template<typename Shape>
            void AddShape(const Shape& shape)
            {
                const auto lines = shape.GetLines();
                for (const auto& line : lines)
                {
                    memcpy((char*)m_VertexBuffers[m_FrameIndex].GetCPUAccessibleMemory() + m_VertCount * sizeof(WireframeShape::LineVertex), &line, sizeof(line));
                    m_VertCount += 2;
                }
            }

        private:
            Pipeline::VertexShaderPass m_Pass;

            std::array<RenderAPI::Buffer, 3> m_VertexBuffers;
            D3D12_VERTEX_BUFFER_VIEW m_VBView;
            uint32_t m_VertCount = 0;
            uint32_t m_FrameIndex = 0;
        };
    }
}
