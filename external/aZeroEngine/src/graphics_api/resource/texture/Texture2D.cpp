#include "Texture2D.hpp"

void aZero::RenderAPI::Texture2D::Move(Texture2D& other)
{
	ResourceBase::operator=(std::move(other));
	m_CurrentResourceState = other.m_CurrentResourceState;
	m_OptClearValue = other.m_OptClearValue;
	other.m_OptClearValue = std::optional<D3D12_CLEAR_VALUE>{};
}

aZero::RenderAPI::Texture2D::Texture2D(ID3D12DeviceX* device, const Desc& desc, std::optional<ResourceRecycler*> opt_diResourceRecycler, std::optional<D3D12_CLEAR_VALUE> optClearValue)
{
	this->Init(device, desc, opt_diResourceRecycler, optClearValue);
}

aZero::RenderAPI::Texture2D::Texture2D(Texture2D&& other) noexcept
{
	this->Move(other);
}

aZero::RenderAPI::Texture2D& aZero::RenderAPI::Texture2D::operator=(Texture2D&& other) noexcept
{
	if (this != &other)
	{
		this->Move(other);
	}
	return *this;
}

void aZero::RenderAPI::Texture2D::Init(ID3D12DeviceX* device, const Desc& desc, std::optional<ResourceRecycler*> opt_diResourceRecycler, std::optional<D3D12_CLEAR_VALUE> optClearValue)
{
	if (!this->IsDescValid(desc))
	{
		DEBUG_PRINT("Invalid texture description.");
		return;
	}

	this->Reset();

	m_OptClearValue = optClearValue;
	const D3D12_CLEAR_VALUE* clearValue = optClearValue.has_value() ? &optClearValue.value() : nullptr;

	D3D12_RESOURCE_DESC resourceDesc;
	ZeroMemory(&resourceDesc, sizeof(D3D12_RESOURCE_DESC));
	resourceDesc.Format = desc.Format;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = static_cast<UINT64>(desc.Width);
	resourceDesc.Height = static_cast<UINT>(desc.Height);
	resourceDesc.DepthOrArraySize = static_cast<UINT16>(1);
	resourceDesc.MipLevels = desc.MipLevels;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = desc.Flags;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	m_CurrentState = D3D12_RESOURCE_STATE_COMMON;

	ResourceBase::Init(device, opt_diResourceRecycler.has_value() ? opt_diResourceRecycler.value() : nullptr, resourceDesc, D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT, clearValue);
}

D3D12_TEXTURE_BARRIER aZero::RenderAPI::Texture2D::CreateTransition(D3D12_BARRIER_SYNC newSync, D3D12_BARRIER_ACCESS newAccess, D3D12_BARRIER_LAYOUT newLayout)
{
	D3D12_TEXTURE_BARRIER barrier;
	barrier.SyncBefore = m_CurrentResourceState.Sync;
	barrier.SyncAfter = newSync;
	barrier.AccessBefore = m_CurrentResourceState.Access;
	barrier.AccessAfter = newAccess;
	barrier.LayoutBefore = m_CurrentResourceState.Layout;
	barrier.LayoutAfter = newLayout;
	barrier.pResource = m_Resource.Get();
	barrier.Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE;
	// todo ????
	barrier.Subresources.FirstArraySlice = 0;
	barrier.Subresources.FirstPlane = 0;
	barrier.Subresources.IndexOrFirstMipLevel = 0;
	barrier.Subresources.NumArraySlices = 1;
	barrier.Subresources.NumMipLevels = 1;
	barrier.Subresources.NumPlanes = 1;

	m_CurrentResourceState.Sync = newSync;
	m_CurrentResourceState.Access = newAccess;
	m_CurrentResourceState.Layout = newLayout;

	return barrier;
}

D3D12_RESOURCE_BARRIER aZero::RenderAPI::Texture2D::CreateTransition(D3D12_RESOURCE_STATES newState)
{
	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	D3D12_RESOURCE_TRANSITION_BARRIER transitionBarrier;
	barrier.Transition.pResource = m_Resource.Get();
	barrier.Transition.StateBefore = m_CurrentState;
	barrier.Transition.StateAfter = newState;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_CurrentState = newState;
	return barrier;
}

void aZero::RenderAPI::Texture2D::Reset()
{
	m_OptClearValue = std::optional<D3D12_CLEAR_VALUE>{};
	m_CurrentResourceState = ResourceState();
	m_CurrentState = D3D12_RESOURCE_STATE_COMMON;
	ResourceBase::Reset();
}

bool aZero::RenderAPI::Texture2D::IsDescValid(const Desc& desc) const
{
	if (desc.Width > 0 && desc.Height > 0 && desc.MipLevels > 0 && desc.Format != DXGI_FORMAT::DXGI_FORMAT_UNKNOWN)
	{
		return true;
	}

	return false;
}