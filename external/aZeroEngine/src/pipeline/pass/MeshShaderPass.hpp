#pragma once
#include "MultiShaderPass.hpp"
#include "pipeline/shader/MeshShader.hpp"

namespace aZero
{
	namespace Pipeline
	{
		class MeshShaderPass : public MultiShaderPass
		{
		public:
			using Description = MultiShaderPassDesc;
		private:
			void Reset()
			{
				ShaderPassBase::Reset();
				m_MSThreadGroups = Pipeline::Shader::ThreadGroup();
			}

			bool ValidatePassInputs(const Description& description) const
			{
				// todo add validation

				return true;
			}

			bool CreatePipelineState(ID3D12DeviceX* device, const Description& description, Pipeline::MeshShader& meshShader, std::optional<Pipeline::PixelShader*> pixelShader, Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature>& rootSignature) const;

			Pipeline::Shader::ThreadGroup m_MSThreadGroups;
		public:
			MeshShaderPass() = default;

			bool Compile(ID3D12DeviceX* device, const Description& description, Pipeline::MeshShader& meshShader, std::optional<Pipeline::PixelShader*> pixelShader);

			[[nodiscard]] Pipeline::Shader::ThreadGroup GetMSThreadGroups() const { return m_MSThreadGroups; }
		};
	}
}