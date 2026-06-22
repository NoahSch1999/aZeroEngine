#include "Descriptor.hpp"
#include "DescriptorHeap.hpp"

aZero::RenderAPI::Descriptor::Descriptor(const D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle,
	const D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle,
	const DescriptorIndex HeapIndex, DescriptorHeap* OwningHeap, bool destroyOnScope)
	:m_CpuHandle(CpuHandle), m_GpuHandle(GpuHandle), m_HeapIndex(HeapIndex), m_diOwningHeap(OwningHeap), m_DestroyOnScope(destroyOnScope) {}

aZero::RenderAPI::Descriptor::Descriptor()
	:m_diOwningHeap(nullptr), m_HeapIndex(Descriptor::InvalidDescriptorIndex) {
}

aZero::RenderAPI::Descriptor::~Descriptor()
{
	if (m_DestroyOnScope && m_diOwningHeap)
	{
		m_diOwningHeap->OnDescriptorDestructor(*this);
	}
}

aZero::RenderAPI::Descriptor::Descriptor(Descriptor&& other) noexcept
{
	*this = std::move(other);
}

aZero::RenderAPI::Descriptor& aZero::RenderAPI::Descriptor::operator=(Descriptor&& other) noexcept
{
	std::swap(m_CpuHandle, other.m_CpuHandle);
	std::swap(m_GpuHandle, other.m_GpuHandle);
	std::swap(m_HeapIndex, other.m_HeapIndex);
	std::swap(m_DestroyOnScope, other.m_DestroyOnScope);
	std::swap(m_diOwningHeap, other.m_diOwningHeap);
	return *this;
}