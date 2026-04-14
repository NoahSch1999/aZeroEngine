#pragma once
#include "MultiShaderPass.hpp"
#include "pipeline/shader/MeshShader.hpp"
#include "pipeline/shader/AmplificationShader.hpp"

namespace aZero
{
	namespace Pipeline
	{
		class MeshShaderPass : public MultiShaderPass
		{
		public:
			using Description = MultiShaderPassDesc;

			MeshShaderPass() = default;
			MeshShaderPass(MeshShaderPass&& other) noexcept;
			MeshShaderPass& operator=(MeshShaderPass&& other) noexcept;

			bool Compile(ID3D12DeviceX* device, const Description& description, std::optional<Pipeline::AmplificationShader*> amplificationShader, Pipeline::MeshShader& meshShader, std::optional<Pipeline::PixelShader*> pixelShader);

			[[nodiscard]] MeshShader::ThreadGroup GetMSThreadGroups() const { return m_MSThreadGroups; }
			[[nodiscard]] AmplificationShader::ThreadGroup GetASThreadGroups() const { return m_ASThreadGroups; }
		private:

			bool ValidatePassInputs(const Description& description) const
			{
				// todo add validation

				return true;
			}

			bool CreatePipelineState(ID3D12DeviceX* device, const Description& description, std::optional<Pipeline::AmplificationShader*> amplificationShader, Pipeline::MeshShader& meshShader, std::optional<Pipeline::PixelShader*> pixelShader, Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature>& rootSignature) const;

			AmplificationShader::ThreadGroup m_ASThreadGroups;
			MeshShader::ThreadGroup m_MSThreadGroups;
		};
	}
}