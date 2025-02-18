#include "Descriptor.hpp"
#include "DescriptorHeap.hpp"

aZero::D3D12::Descriptor::~Descriptor()
{
	if (m_OwningHeap)
	{
		m_OwningHeap->RecycleDescriptor(*this);
	}
}