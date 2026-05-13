#pragma once
#include <optional>
#include <unordered_map>
#include "ShaderNEW.hpp"

namespace aZero
{
	namespace NEW_Pipeline
	{
		enum class ShaderBindingType { SRV = D3D12_ROOT_PARAMETER_TYPE_SRV, CBV = D3D12_ROOT_PARAMETER_TYPE_CBV, UAV = D3D12_ROOT_PARAMETER_TYPE_UAV, CONSTANT = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS };
		using RootIndex = uint32_t;

		class BufferBinding
		{
		public:
			BufferBinding() = default;
			BufferBinding(ShaderBindingType type, RootIndex index)
				:m_Index(index), m_Type(type){}

			ShaderBindingType GetBindingType() const { return m_Type; }
			RootIndex GetRootIndex() const { return m_Index; }

		private:
			ShaderBindingType m_Type;
			RootIndex m_Index = std::numeric_limits<RootIndex>::max();
		};

		class ConstantBinding
		{
		public:
			ConstantBinding() = default;

			ConstantBinding(uint32_t numConstants, RootIndex index)
				:m_Index(index), m_NumConstants(numConstants) {
			}

			uint32_t GetNumConstants() const { return m_NumConstants; }
			RootIndex GetRootIndex() const { return m_Index; }

		private:
			uint32_t m_NumConstants = std::numeric_limits<uint32_t>::max();
			RootIndex m_Index = std::numeric_limits<RootIndex>::max();
		};

		template<typename T>
		struct BindingCombo
		{
			std::unordered_map<std::string, uint32_t> m_Name_To_Binding; // Reflection name => root index
			std::vector<T> m_Bindings; // Binding slots
		};

		using MappedBufferBindings = BindingCombo<BufferBinding>;
		using MappedConstantBindings = BindingCombo<ConstantBinding>;

		class RenderPass
		{
		public:
			struct Desc
			{
				std::vector<DXGI_FORMAT> RtvFormats;
				DXGI_FORMAT DsvFormat;
			};

			RenderPass() = default;

			RenderPass(const Desc& desc, ID3D12DeviceX* device, const Shader& vertexShader, std::optional<std::reference_wrapper<const Shader>> pixelShader) { 
				this->CompileVertexPass(desc, device, vertexShader, pixelShader); 
			}
			
			RenderPass(const Desc& desc, ID3D12DeviceX* device, std::optional<std::reference_wrapper<const Shader>> amplificationShader, const Shader& meshShader, std::optional<std::reference_wrapper<const Shader>> pixelShader) { 
				this->CompileMeshletPass(desc, device, amplificationShader, meshShader, pixelShader);
			}

			std::optional<std::reference_wrapper<BufferBinding>> GetBufferBinding(std::string_view name);
			std::optional<std::reference_wrapper<ConstantBinding>> GetConstantBinding(std::string_view name);

			// TODO: Impl vertexpass init
			bool CompileVertexPass(const Desc& desc, ID3D12DeviceX* device, const Shader& vertexShader, std::optional<std::reference_wrapper<const Shader>> pixelShader);
			bool CompileMeshletPass(const Desc& desc, ID3D12DeviceX* device, std::optional<std::reference_wrapper<const Shader>> amplificationShader, const Shader& meshShader, std::optional<std::reference_wrapper<const Shader>> pixelShader);

		private:
			void ExtractRootParameters(const Shader& shader, D3D12_SHADER_VISIBILITY shaderVisType, std::vector<D3D12_ROOT_PARAMETER>& inoutRootParams, MappedBufferBindings& inoutBufferBindings, MappedConstantBindings& inoutConstantBindings);
			bool CreateRootSignature(ID3D12DeviceX* device, const std::vector<D3D12_ROOT_PARAMETER>& rootParams);

			bool CreateVertexPipelineState(const Desc& desc, ID3D12DeviceX* device, const Shader& vertexShader, std::optional<std::reference_wrapper<const Shader>> pixelShader);
			bool CreateMeshletPipelineState(const Desc& desc, ID3D12DeviceX* device, std::optional<std::reference_wrapper<const Shader>> amplificationShader, const Shader& meshShader, std::optional<std::reference_wrapper<const Shader>> pixelShader);

			MappedBufferBindings m_BufferBindings;
			MappedConstantBindings m_ConstantBindings;
			Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
			Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
		};
	}
}