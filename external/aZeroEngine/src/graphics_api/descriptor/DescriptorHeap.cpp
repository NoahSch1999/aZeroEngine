#include "DescriptorHeap.hpp"
#include "misc/CallbackExecutor.hpp"

void aZero::RenderAPI::DescriptorHeap::OnDescriptorDestructor(Descriptor& descriptor)
{
	if (descriptor.m_diOwningHeap == this)
	{
		DescriptorIndex index = descriptor.m_HeapIndex;
		m_diCallbackExecutor->Append([this, index] {
			this->m_Freelist.Recycle(index);
			});
		descriptor.m_HeapIndex = Descriptor::InvalidDescriptorIndex;
	}
}

aZero::RenderAPI::DescriptorHeap::DescriptorHeap(ID3D12DeviceX* device, CallbackExecutor& diCallbackExecutor, const D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors, const bool gpuVisible)
{
	this->Init(device, diCallbackExecutor, type, numDescriptors, gpuVisible);
}

void aZero::RenderAPI::DescriptorHeap::Init(ID3D12DeviceX* device, CallbackExecutor& diCallbackExecutor, const D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors, const bool gpuVisible)
{
	if (!m_Heap)
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc;
		desc.NumDescriptors = numDescriptors;
		desc.Type = type;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask = 0;

		if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
		{
			desc.Flags = gpuVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		}

		const HRESULT res = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_Heap.GetAddressOf()));
		if (FAILED(res))
		{
			throw std::invalid_argument("DescriptorHeap::Init() => Failed to create heap");
		}

		m_CpuHeapStart = m_Heap->GetCPUDescriptorHandleForHeapStart();
		if (desc.Flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
		{
			m_GpuHeapStart = m_Heap->GetGPUDescriptorHandleForHeapStart();
		}

		m_DescriptorSize = device->GetDescriptorHandleIncrementSize(type);
		m_diCallbackExecutor = &diCallbackExecutor;
	}
}

aZero::RenderAPI::Descriptor aZero::RenderAPI::DescriptorHeap::CreateDescriptor()
{
	const DescriptorIndex descriptorIndex = m_Freelist.New();
	if (descriptorIndex >= m_Heap->GetDesc().NumDescriptors)
	{
		throw std::invalid_argument("DescriptorHeap::GetDescriptor() => Out of descriptors");
	}

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = { 0 };
	cpuHandle.ptr = m_CpuHeapStart.ptr + descriptorIndex * m_DescriptorSize;

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = { 0 };
	if (m_Heap->GetDesc().Flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
	{
		gpuHandle.ptr = m_GpuHeapStart.ptr + descriptorIndex * m_DescriptorSize;
	}

	return Descriptor(cpuHandle, gpuHandle, descriptorIndex, this);
}

void aZero::RenderAPI::DescriptorHeap::DestroyDescriptor(Descriptor& descriptor)
{
	if (descriptor.m_diOwningHeap == this)
	{
		descriptor = Descriptor();
	}
}

D3D12_DESCRIPTOR_HEAP_TYPE aZero::RenderAPI::DescriptorHeap::GetType() const
{
	const D3D12_DESCRIPTOR_HEAP_DESC desc = m_Heap->GetDesc();
	return desc.Type;
}

uint32_t aZero::RenderAPI::DescriptorHeap::GetMaxDescriptors() const
{
	const D3D12_DESCRIPTOR_HEAP_DESC desc = m_Heap->GetDesc();
	return desc.NumDescriptors;
}

bool aZero::RenderAPI::DescriptorHeap::IsGpuVisible() const
{
	const D3D12_DESCRIPTOR_HEAP_DESC desc = m_Heap->GetDesc();
	return desc.Flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
}

uint32_t aZero::RenderAPI::DescriptorHeap::GetDescriptorSize() const
{
	ID3D12DeviceX* device;
	const HRESULT getDeviceRes = m_Heap->GetDevice(IID_PPV_ARGS(&device));
	if (FAILED(getDeviceRes))
	{
		throw std::invalid_argument("Failed to get descriptor heap device.");
	}
	return device->GetDescriptorHandleIncrementSize(this->GetType());
}