#include "ResourceBase.hpp"

aZero::RenderAPI::ResourceBase::ResourceBase(ResourceBase&& other) noexcept
{
	*this = std::move(other);
}

aZero::RenderAPI::ResourceBase& aZero::RenderAPI::ResourceBase::operator=(ResourceBase&& other) noexcept
{
	std::swap(m_Resource, other.m_Resource);
	std::swap(m_diResourceRecycler, other.m_diResourceRecycler);
	return *this;
}

aZero::RenderAPI::ResourceBase::~ResourceBase()
{
	this->OnDestroy();
}

aZero::RenderAPI::ResourceBase::ResourceBase(ID3D12DeviceX* device, aZero::RenderAPI::ResourceRecycler* diResourceRecycler, const D3D12_RESOURCE_DESC& resourceDesc, D3D12_HEAP_TYPE accessType, const D3D12_CLEAR_VALUE* clearValue)
{
	D3D12_HEAP_PROPERTIES heapProperties;
	ZeroMemory(&heapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	heapProperties.Type = accessType;

	const HRESULT res = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		clearValue,
		IID_PPV_ARGS(m_Resource.GetAddressOf()));

	if (FAILED(res))
	{
		throw std::invalid_argument("Resource::Init() => Failed to create commited resource");
	}

	m_diResourceRecycler = diResourceRecycler;
}

void aZero::RenderAPI::ResourceBase::OnDestroy()
{
	if (m_diResourceRecycler)
	{
		m_diResourceRecycler->Append(std::move(m_Resource));
	}
}