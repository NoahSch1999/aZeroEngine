#include "MultiShaderPass.hpp"

void aZero::Pipeline::MultiShaderPass::Bind(RenderAPI::CommandList& cmdList, const RenderAPI::DescriptorHeap& resourceHeap, const RenderAPI::DescriptorHeap& samplerHeap) const
{
	NewRenderPass::Bind(cmdList, resourceHeap, samplerHeap);
	cmdList->SetGraphicsRootSignature(m_RootSignature.Get());
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