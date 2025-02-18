#include "GPUResource.hpp"

void aZero::D3D12::TransitionResources(ID3D12GraphicsCommandList* CmdList, const std::vector<ResourceTransitionBundles>& Bundles)
{
	std::vector<D3D12_RESOURCE_BARRIER> Barriers(Bundles.size());
	for (int BarrierIndex = 0; BarrierIndex < Barriers.size(); BarrierIndex++)
	{
		Barriers[BarrierIndex].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		Barriers[BarrierIndex].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		D3D12_RESOURCE_TRANSITION_BARRIER TransitionBarrier;
		TransitionBarrier.pResource = Bundles[BarrierIndex].ResourcePtr;
		TransitionBarrier.StateBefore = Bundles[BarrierIndex].StateBefore;
		TransitionBarrier.StateAfter = Bundles[BarrierIndex].StateAfter;
		TransitionBarrier.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		Barriers[BarrierIndex].Transition = TransitionBarrier;
	}

	CmdList->ResourceBarrier(Barriers.size(), Barriers.data());
}
