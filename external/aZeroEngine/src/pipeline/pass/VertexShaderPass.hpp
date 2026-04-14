#pragma once
#include "MultiShaderPass.hpp"
#include "pipeline/shader/VertexShader.hpp"

namespace aZero
{
	namespace Pipeline
	{
		class VertexShaderPass : public MultiShaderPass
		{
		public:
			// Matches D3D12_PRIMITIVE_TOPOLOGY_TYPE
			enum TopologyType { INVALID = 0, POINT = 1, LINE = 2, TRIANGLE = 3 };

			struct Description : public MultiShaderPassDesc
			{
				TopologyType m_TopologyType;
			};

			VertexShaderPass() = default;
			VertexShaderPass(VertexShaderPass&& other) noexcept;
			VertexShaderPass& operator=(VertexShaderPass&& other) noexcept;

			bool Compile(ID3D12DeviceX* device, const Description& description, Pipeline::VertexShader& vertexShader, std::optional<Pipeline::PixelShader*> pixelShader);
			void Begin(RenderAPI::CommandList& cmdList, const RenderAPI::DescriptorHeap& resourceHeap, const RenderAPI::DescriptorHeap& samplerHeap, const std::vector<RenderAPI::Descriptor*>& renderTargets, const std::optional<RenderAPI::Descriptor*>& depthStencilTarget);

		private:
			bool ValidatePassInputs(const Description& description) const
			{
				if (!MultiShaderPass::ValidatePassInputs(description))
				{
					// todo LOG
					return false;
				}

				if (description.m_TopologyType == TopologyType::INVALID)
				{
					// todo LOG - Not an acceptable topology
					return false;
				}

				return true;
			}

			bool CreatePipelineState(ID3D12DeviceX* device, const Description& description, Pipeline::VertexShader& vertexShader, std::optional<Pipeline::PixelShader*> pixelShader, Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature>& rootSignature) const;

			TopologyType m_TopologyType;
		};
	}
}