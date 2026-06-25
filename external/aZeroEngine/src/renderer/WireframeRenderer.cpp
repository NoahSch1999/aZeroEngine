#include "WireframeRenderer.hpp"
#include "ecs/Components.hpp"
#include "render_api/resource/texture/RenderTarget.hpp"
#include "render_api/resource/texture/DepthStencilTarget.hpp"
#include "Renderer.hpp"

aZero::Rendering::WireframeRenderer::WireframeRenderer(Rendering::Renderer& renderer, ID3D12DeviceX* device, IDxcCompilerX& compiler)
    :m_diRenderer(&renderer)
{
    Pipeline::Shader vs;
    vs.Compile(compiler, Pipeline::GetShaderDirectoryPath() + "DebugLine.vs.hlsl");
    Pipeline::Shader ps;
    ps.Compile(compiler, Pipeline::GetShaderDirectoryPath() + "DebugLine.ps.hlsl");

    Pipeline::RenderPass::VertexPassDesc vsDesc;
    vsDesc.DsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    vsDesc.RtvFormats.push_back(DXGI_FORMAT_R8G8B8A8_UNORM);
    vsDesc.TopologyType = Pipeline::ETopologyType::LINE;
    m_Pass.CompileVertexPass(vsDesc, device, vs, ps);

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
    RenderAPI::CommandList& cmdList = frameContext.GetCommandList();
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(0, 255, 255), "Render debug colliders");

    cmdList.SetDescriptorHeaps(m_diRenderer->GetResourceHeap(), m_diRenderer->GetSamplerHeap());

    m_Pass.Begin(cmdList);
    cmdList.OMSetRenderTargets({ rtv.GetDescriptor() }, dsv.GetDescriptor());

    auto binding = m_Pass.GetConstantBinding("VP_CONSTANT");
    struct Constants
    {
        DXM::Matrix vp;
    }constants;

    constants.vp = camera.GetViewProjectionMatrix(cameraPosition, cameraRotation);
    cmdList.SetGraphicsRoot32BitConstantsSafe(binding.value().get().GetRootIndex(), binding.value().get().GetNumConstants(), &constants, 0);

    auto viewport = camera.GetViewport();
    cmdList->RSSetViewports(1, &viewport);
    auto rect = camera.GetScizzorRect();
    cmdList->RSSetScissorRects(1, &rect);

    m_VBView.BufferLocation = m_VertexBuffers[m_FrameIndex].GetResource()->GetGPUVirtualAddress();
    cmdList->IASetVertexBuffers(0, 1, &m_VBView);

    cmdList->DrawInstanced(m_VertCount, 1, 0, 0);

    m_diRenderer->GetGraphicsCommandQueue().ExecuteCommandList(cmdList);
}

void aZero::Rendering::WireframeRenderer::BeginFrame() { m_VertCount = 0; m_FrameIndex = m_diRenderer->GetFrameIndex(); }