#include "ShaderPassBase.hpp"

bool aZero::Pipeline::ShaderPassBase::CreateRootSignatureImpl(ID3D12DeviceX* device, Microsoft::WRL::ComPtr<ID3D12RootSignature>& rootSignature, const std::vector<D3D12_ROOT_PARAMETER>& rootParams) const
{
	// todo Fill in?
	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
	//

	const D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{
		static_cast<UINT>(rootParams.size()),
		rootParams.data(),
		static_cast<UINT>(staticSamplers.size()), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		| D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
		| D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED };

	Microsoft::WRL::ComPtr<ID3DBlob> serializeBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

	const HRESULT rsSerializeRes = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializeBlob.GetAddressOf(), errorBlob.GetAddressOf());
	if (FAILED(rsSerializeRes))
	{
		DEBUG_PRINT("Failed to serialize root signature");
		return false;
	}

	const HRESULT rsRes = device->CreateRootSignature(0, serializeBlob->GetBufferPointer(), serializeBlob->GetBufferSize(), IID_PPV_ARGS(rootSignature.GetAddressOf()));
	if (FAILED(rsRes))
	{
		DEBUG_PRINT("Failed to create root signature");
		return false;
	}

	return true;
}