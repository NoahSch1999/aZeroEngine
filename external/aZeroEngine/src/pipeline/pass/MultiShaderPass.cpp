#include "MultiShaderPass.hpp"

aZero::Pipeline::MultiShaderPass::MultiShaderPass(MultiShaderPass&& other) noexcept
{
	*this = std::move(other);
}

aZero::Pipeline::MultiShaderPass& aZero::Pipeline::MultiShaderPass::operator=(MultiShaderPass&& other) noexcept
{
	ShaderPassBase::operator=(std::move(other));
	std::swap(m_RenderTargets, other.m_RenderTargets);
	std::swap(m_DepthStencilTarget, other.m_DepthStencilTarget);
	return *this;
}

void aZero::Pipeline::MultiShaderPass::Bind(RenderAPI::CommandList& cmdList) const
{
	ShaderPassBase::Bind(cmdList);
	cmdList->SetGraphicsRootSignature(m_RootSignature.Get());

	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 9> outputTargets;
	const auto& renderTargets = m_RenderTargets.GetData();
	for (uint32_t i = 0; i < renderTargets.size(); i++)
	{
		outputTargets[i] = renderTargets[i]->GetCpuHandle();
	}

	if (m_DepthStencilTarget.has_value())
	{
		D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptor = m_DepthStencilTarget.value()->GetCpuHandle();
		cmdList->OMSetRenderTargets(static_cast<UINT>(renderTargets.size()), outputTargets.data(), 0, &dsvDescriptor);
	}
	else
	{
		cmdList->OMSetRenderTargets(static_cast<UINT>(renderTargets.size()), outputTargets.data(), 0, nullptr);
	}

	for (const BufferBinding& buffer : m_BufferBindings.m_Bindings)
	{
		// Skip binding bufferslot if no buffer was bound
		if (!buffer.m_Buffer)
		{
			DEBUG_PRINT("A buffer wasn't bound to the pass.");
			continue;
		}

		switch (buffer.m_Type)
		{
		case BindingType::SRV:
			cmdList->SetGraphicsRootShaderResourceView(buffer.m_Index, buffer.m_Buffer->GetResource()->GetGPUVirtualAddress());
			break;
		case BindingType::UAV:
			cmdList->SetGraphicsRootUnorderedAccessView(buffer.m_Index, buffer.m_Buffer->GetResource()->GetGPUVirtualAddress());
			break;
		default:
			throw std::runtime_error("Bound buffer doesn't have a valid binding type.");
		}
	}

	for (const ConstantBinding& constant : m_ConstantBindings.m_Bindings)
	{
		cmdList->SetGraphicsRoot32BitConstants(constant.m_Index, constant.m_NumConstants, constant.m_Data.get(), 0);
	}
}