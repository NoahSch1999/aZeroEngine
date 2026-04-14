#include "MultiShaderPass.hpp"

aZero::Pipeline::MultiShaderPass::MultiShaderPass(MultiShaderPass&& other) noexcept
{
	*this = std::move(other);
}

aZero::Pipeline::MultiShaderPass& aZero::Pipeline::MultiShaderPass::operator=(MultiShaderPass&& other) noexcept
{
	ShaderPassBase::operator=(std::move(other));
	return *this;
}

void aZero::Pipeline::MultiShaderPass::Begin(RenderAPI::CommandList& cmdList, const RenderAPI::DescriptorHeap& resourceHeap, const RenderAPI::DescriptorHeap& samplerHeap, const std::vector<RenderAPI::Descriptor*>& renderTargets, const std::optional<RenderAPI::Descriptor*>& depthStencilTarget) const
{
	ShaderPassBase::Begin(cmdList);

	std::array<ID3D12DescriptorHeap*, 2> heaps{ resourceHeap.Get(), samplerHeap.Get() };
	cmdList->SetDescriptorHeaps(heaps.size(), heaps.data());

	cmdList->SetGraphicsRootSignature(m_RootSignature.Get());

	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 8> outputTargets;
	for (uint32_t i = 0; i < renderTargets.size(); i++)
	{
		outputTargets[i] = renderTargets[i]->GetCpuHandle();
	}

	if (depthStencilTarget.has_value())
	{
		D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptor = depthStencilTarget.value()->GetCpuHandle();
		cmdList->OMSetRenderTargets(static_cast<UINT>(renderTargets.size()), outputTargets.data(), 0, &dsvDescriptor);
	}
	else
	{
		cmdList->OMSetRenderTargets(static_cast<UINT>(renderTargets.size()), outputTargets.data(), 0, nullptr);
	}
}