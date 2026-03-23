#pragma once
#include "ShaderPassBase.hpp"
#include "pipeline/shader/ComputeShader.hpp"

namespace aZero
{
	namespace Pipeline
	{
		class ComputeShaderPass : public ShaderPassBase
		{
		public:
			struct Description
			{
				int dummy;
			};

			ComputeShaderPass() = default;
			ComputeShaderPass(ComputeShaderPass&& other) noexcept;
			ComputeShaderPass& operator=(ComputeShaderPass&& other) noexcept;

			bool Compile(ID3D12DeviceX* device, const Description& description, Pipeline::ComputeShader& computeShader);

			void Bind(RenderAPI::CommandList& cmdList) const;

			[[nodiscard]] ComputeShader::ThreadGroup GetThreadGroups() const { return m_ThreadGroupCount; }

		private:
			bool ValidatePassInputs(const Description& description) const
			{
				// todo add validation

				return true;
			}

			bool CreatePipelineState(ID3D12DeviceX* device, const Description& description, Pipeline::ComputeShader& computeShader, Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipelineState) const;

			ComputeShader::ThreadGroup m_ThreadGroupCount;
		};
	}
}