#include "Buffer.hpp"

D3D12_RESOURCE_DESC CreateDesc(const aZero::RenderAPI::Buffer::Desc& desc)
{
	D3D12_RESOURCE_DESC resourceDesc;
	ZeroMemory(&resourceDesc, sizeof(D3D12_RESOURCE_DESC));
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Height = 1;
	resourceDesc.Width = desc.NumBytes;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Flags = desc.AllowUAV ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
	resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	return resourceDesc;
}

aZero::RenderAPI::Buffer::Buffer(ID3D12DeviceX* device, const Desc& desc, std::optional<ResourceRecycler*> opt_diResourceRecycler)
	:ResourceBase(device, opt_diResourceRecycler.has_value() ? opt_diResourceRecycler.value() : nullptr, CreateDesc(desc), desc.AccessType, nullptr, desc.StartState)
{
	if (desc.AccessType == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD || desc.AccessType == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK || desc.AccessType == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_GPU_UPLOAD)
	{
		m_Resource->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedPtr));
	}
}

aZero::RenderAPI::Buffer::Buffer(Buffer&& other) noexcept
{
	*this = std::move(other);
}

aZero::RenderAPI::Buffer& aZero::RenderAPI::Buffer::operator=(Buffer&& other) noexcept
{
	ResourceBase::operator=(std::move(other));
	std::swap(m_MappedPtr, other.m_MappedPtr);
	return *this;
}

bool aZero::RenderAPI::Buffer::IsDescValid(const Desc& desc)
{
	if (desc.NumBytes > 0)
	{
		return true;
	}

	return false;
}