#include "WireframeRenderer.hpp"
#include "ecs/Components.hpp"
#include "graphics_api/resource/texture/RenderTarget.hpp"
#include "graphics_api/resource/texture/DepthStencilTarget.hpp"
#include "Renderer.hpp"

aZero::Rendering::WireframeRenderer::WireframeRenderer(Rendering::Renderer& renderer, ID3D12DeviceX* device, IDxcCompilerX& compiler)
    :m_diRenderer(&renderer)
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
        m_VertexBuffers[i] = RenderAPI::Buffer(device, RenderAPI::Buffer::Desc(sizeof(WireframeShape::LineVertex) * 2000000, D3D12_HEAP_TYPE_UPLOAD));
    }

    m_VBView.StrideInBytes = sizeof(WireframeShape::LineVertex);
    m_VBView.SizeInBytes = m_VertexBuffers[0].GetResource()->GetDesc().Width;
}

void aZero::Rendering::WireframeRenderer::Render(const Component::Camera& camera, const Component::Position& cameraPosition, const Component::Rotation& cameraRotation, RenderTarget& rtv, DepthStencilTarget& dsv)
{
    FrameContext& frameContext = m_diRenderer->GetCurrentContext();
    PIXScopedEvent(frameContext.m_DirectCmdList.Get(), PIX_COLOR(0, 255, 255), "Render debug colliders");
    RenderAPI::CommandList& cmdList = frameContext.m_DirectCmdList;

    m_Pass.Begin(cmdList, m_diRenderer->GetResourceHeap(), m_diRenderer->GetSamplerHeap(), {&rtv.GetDescriptor()}, &dsv.GetDescriptor());

    auto binding = m_Pass.GetConstantBindingIndex("VP");
    struct Constants
    {
        DXM::Matrix vp;
    }constants;

    constants.vp = camera.GetViewProjectionMatrix(cameraPosition, cameraRotation);
    cmdList.SetGraphicsRoot32BitConstantsSafe(binding.GetRootIndex(), binding.GetNumConstants(), &constants, 0);

    auto viewport = camera.GetViewport();
    cmdList->RSSetViewports(1, &viewport);
    auto rect = camera.GetScizzorRect();
    cmdList->RSSetScissorRects(1, &rect);

    m_VBView.BufferLocation = m_VertexBuffers[m_FrameIndex].GetResource()->GetGPUVirtualAddress();
    cmdList->IASetVertexBuffers(0, 1, &m_VBView);

    cmdList->DrawInstanced(m_VertCount, 1, 0, 0);

    m_diRenderer->GetGraphicsCommandQueue().ExecuteCommandList(frameContext.m_DirectCmdList);
}

void aZero::Rendering::WireframeRenderer::BeginFrame() { m_VertCount = 0; m_FrameIndex = m_diRenderer->GetFrameIndex(); }