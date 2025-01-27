#include "Descriptor.h"
#include "DescriptorHeap.h"

aZero::D3D12::Descriptor::~Descriptor()
{
	if (m_OwningHeap)
	{
		m_OwningHeap->RecycleDescriptor(*this);
	}
}