#pragma once
#include "RenderPass.hpp"
#include "pipeline/shader/ComputeShader.hpp"

namespace aZero
{
	namespace Pipeline
	{
		class ComputeShaderPass : public RenderPass
		{
		public:
			struct Description
			{
				int dummy;
			};

			ComputeShaderPass() = default;

			bool Compile(ID3D12DeviceX* device, const Description& description, Pipeline::ComputeShader& computeShader);

			void Bind(RenderAPI::CommandList& cmdList) const;

			[[nodiscard]] Pipeline::Shader::ThreadGroup GetThreadGroups() const { return m_ThreadGroupCount; }

		private:
			void Reset()
			{
				RenderPass::Reset();
				m_ThreadGroupCount = Pipeline::Shader::ThreadGroup();
			}

			bool ValidatePassInputs(const Description& description) const
			{
				// todo add validation

				return true;
			}

			bool CreatePipelineState(ID3D12DeviceX* device, const Description& description, Pipeline::ComputeShader& computeShader, Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipelineState) const;

			Pipeline::Shader::ThreadGroup m_ThreadGroupCount;
		};
	}
}