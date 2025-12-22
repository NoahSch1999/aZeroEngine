#pragma once
#include "NewRenderPass.hpp"
#include "pipeline/ComputeShader.hpp"

namespace aZero
{
	namespace Pipeline
	{
		class ComputeShaderPass : public NewRenderPass
		{
		public:
			struct Description
			{
				int dummy;
			};

		private:
			void Reset()
			{
				NewRenderPass::Reset();
				// todo Add pipeline-specific resets
			}

			bool ValidatePassInputs(const Description& description) const
			{
				// todo add validation

				return true;
			}

			bool CreatePipelineState(ID3D12Device* device, const Description& description, Pipeline::ComputeShader& computeShader, ID3D12PipelineState* pipelineState) const;

			bool CreatePipeline(ID3D12Device* device, const Description& description, Pipeline::ComputeShader& computeShader, ID3D12PipelineState* pipelineState, ID3D12RootSignature* rootSignature) const;

		public:
			ComputeShaderPass() = default;

			bool Compile(ID3D12Device* device, const Description& description, Pipeline::ComputeShader& computeShader);

			void Bind(RenderAPI::CommandList& cmdList) const;
		};
	}
}