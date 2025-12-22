#pragma once
#include "MultiShaderPass.hpp"
#include "pipeline/MeshShader.hpp"

namespace aZero
{
	namespace Pipeline
	{
		class MeshShaderPass : public MultiShaderPass
		{
			using Description = MultiShaderPassDesc;
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

			bool CreatePipelineState(ID3D12Device* device, const Description& description, Pipeline::MeshShader& meshShader, std::optional<Pipeline::PixelShader*> pixelShader, ID3D12PipelineState* pipelineState, ID3D12RootSignature* rootSignature) const;

		public:
			MeshShaderPass() = default;

			bool Compile(ID3D12Device* device, const Description& description, Pipeline::MeshShader& meshShader, std::optional<Pipeline::PixelShader*> pixelShader);
		};
	}
}