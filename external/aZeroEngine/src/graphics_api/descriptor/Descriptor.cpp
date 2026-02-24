#include "Descriptor.hpp"
#include "DescriptorHeap.hpp"

aZero::RenderAPI::Descriptor::Descriptor(const D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle,
	const D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle,
	const DescriptorIndex HeapIndex, DescriptorHeap* OwningHeap)
	:m_CpuHandle(CpuHandle), m_GpuHandle(GpuHandle), m_HeapIndex(HeapIndex), m_diOwningHeap(OwningHeap) {}

void aZero::RenderAPI::Descriptor::Move(Descriptor& other)
{
	m_CpuHandle = other.m_CpuHandle;
	m_GpuHandle = other.m_GpuHandle;
	m_HeapIndex = other.m_HeapIndex;
	m_diOwningHeap = other.m_diOwningHeap;
	other.m_HeapIndex = Descriptor::InvalidDescriptorIndex;
	other.m_diOwningHeap = nullptr;
}

aZero::RenderAPI::Descriptor::Descriptor()
	:m_diOwningHeap(nullptr), m_HeapIndex(Descriptor::InvalidDescriptorIndex) {
}

aZero::RenderAPI::Descriptor::~Descriptor()
{
	if (m_diOwningHeap)
	{
		m_diOwningHeap->OnDescriptorDestructor(*this);
	}
}

aZero::RenderAPI::Descriptor::Descriptor(Descriptor&& other) noexcept
{
	m_CpuHandle = other.m_CpuHandle;
	m_GpuHandle = other.m_GpuHandle;
	m_HeapIndex = other.m_HeapIndex;
	m_diOwningHeap = other.m_diOwningHeap;

	other.m_diOwningHeap = nullptr;
}

aZero::RenderAPI::Descriptor& aZero::RenderAPI::Descriptor::operator=(Descriptor&& other) noexcept
{
	if (this != &other)
	{
		m_CpuHandle = other.m_CpuHandle;
		m_GpuHandle = other.m_GpuHandle;
		m_HeapIndex = other.m_HeapIndex;
		m_diOwningHeap = other.m_diOwningHeap;

		other.m_diOwningHeap = nullptr;
	}
	return *this;
}