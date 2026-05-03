#include "WireframeRenderer.hpp"
#include "ecs/components/CameraComponent.hpp"

aZero::Rendering::WireframeRenderer::WireframeRenderer(ID3D12DeviceX* device, IDxcCompilerX& compiler)
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

void aZero::Rendering::WireframeRenderer::Render(RenderAPI::CommandList& cmdList, RenderAPI::DescriptorHeap& resourceHeap, RenderAPI::DescriptorHeap& samplerHeap, const ECS::CameraComponent& camera, RenderTarget& rtv, DepthStencilTarget& dsv)
{
    m_Pass.Begin(cmdList, resourceHeap, samplerHeap, { &rtv.GetDescriptor() }, &dsv.GetDescriptor());

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
}