#include "Texture2D.hpp"

D3D12_RESOURCE_DESC CreateDesc(const aZero::RenderAPI::Texture2D::Desc& desc)
{
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
	return resourceDesc;
}

aZero::RenderAPI::Texture2D::Texture2D(ID3D12DeviceX* device, const Desc& desc, std::optional<ResourceRecycler*> opt_diResourceRecycler, std::optional<D3D12_CLEAR_VALUE> optClearValue)
	:ResourceBase(device, opt_diResourceRecycler.has_value() ? opt_diResourceRecycler.value() : nullptr, CreateDesc(desc), D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT, optClearValue.has_value() ? &optClearValue.value() : nullptr),
	m_OptClearValue(optClearValue), m_CurrentState(D3D12_RESOURCE_STATE_COMMON){}

aZero::RenderAPI::Texture2D::Texture2D(Texture2D&& other) noexcept
{
	*this = std::move(other);
}

aZero::RenderAPI::Texture2D& aZero::RenderAPI::Texture2D::operator=(Texture2D&& other) noexcept
{
	ResourceBase::operator=(std::move(other));
	std::swap(m_OptClearValue, other.m_OptClearValue);
	std::swap(m_CurrentState, other.m_CurrentState);
	return *this;
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