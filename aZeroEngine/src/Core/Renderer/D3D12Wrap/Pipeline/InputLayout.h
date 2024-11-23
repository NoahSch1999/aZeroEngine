#pragma once
#include "Core/D3D12Core.h"

class InputLayout
{
private:
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_Descs;

public:
	InputLayout() = default;

	void AddElement(const char* semantic, DXGI_FORMAT format,
		D3D12_INPUT_CLASSIFICATION inputClassification = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		uint32_t instanceStep = 0, uint32_t semanticIndex = 0, uint32_t inputAssemblerSlot = 0)
	{
		D3D12_INPUT_ELEMENT_DESC desc;
		desc.SemanticName = semantic;
		desc.Format = format;
		desc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		desc.SemanticIndex = semanticIndex;
		desc.InputSlot = inputAssemblerSlot;
		desc.InputSlotClass = inputClassification;
		desc.InstanceDataStepRate = instanceStep;
		m_Descs.push_back(desc);
	}

	const D3D12_INPUT_ELEMENT_DESC* GetDescription() const { return m_Descs.data(); }

	uint32_t GetNumElements() const { return m_Descs.size(); }
};