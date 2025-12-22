#pragma once
#include "MultiShaderPass.hpp"
#include "pipeline/VertexShader.hpp"

namespace aZero
{
	namespace Pipeline
	{
		class VertexShaderPass : public MultiShaderPass
		{
			enum class TopologyType { INVALID = 0, POINT = 1, LINE = 2, TRIANGLE = 3 };

			struct Description : public MultiShaderPassDesc
			{
				TopologyType m_TopologyType;
			};

		private:
			void Reset()
			{
				NewRenderPass::Reset();
				m_TopologyType = TopologyType::INVALID;
			}

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

			bool CreatePipelineState(ID3D12Device* device, const Description& description, Pipeline::VertexShader& vertexShader, std::optional<Pipeline::PixelShader*> pixelShader, ID3D12PipelineState* pipelineState, ID3D12RootSignature* rootSignature) const;

			TopologyType m_TopologyType;

		public:
			VertexShaderPass() = default;

			bool Compile(ID3D12Device* device, const Description& description, Pipeline::VertexShader& vertexShader, std::optional<Pipeline::PixelShader*> pixelShader);
		};
	}
}