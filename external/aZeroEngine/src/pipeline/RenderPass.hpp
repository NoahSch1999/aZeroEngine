#pragma once
#include <optional>
#include <unordered_map>
#include "ShaderNEW.hpp"

namespace aZero
{
	namespace RenderAPI { class CommandList; }

	namespace NEW_Pipeline
	{
		enum class ERenderPassType { INVALID, VERTEX, MESHLET, COMPUTE };
		enum ETopologyType { UNDEFINED = 0, POINT = 1, LINE = 2, TRIANGLE = 3 }; // Matches D3D12_PRIMITIVE_TOPOLOGY_TYPE
		enum class EShaderBindingType { SRV = D3D12_ROOT_PARAMETER_TYPE_SRV, CBV = D3D12_ROOT_PARAMETER_TYPE_CBV, UAV = D3D12_ROOT_PARAMETER_TYPE_UAV, CONSTANT = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS };

		using RootIndex = uint32_t;

		class BufferBinding
		{
		public:
			BufferBinding() = default;
			BufferBinding(EShaderBindingType type, RootIndex index)
				:m_Index(index), m_Type(type){}

			EShaderBindingType GetBindingType() const { return m_Type; }
			RootIndex GetRootIndex() const { return m_Index; }

		private:
			EShaderBindingType m_Type;
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

		// TODO: Impl support for named rtvs
		class RenderPass
		{
		public:
			struct Desc
			{
				std::vector<DXGI_FORMAT> RtvFormats;
				DXGI_FORMAT DsvFormat;
			};

			struct VertexPassDesc : public Desc
			{
				ETopologyType TopologyType;
			};

			using MeshletPassDesc = Desc;

			RenderPass() = default;

			RenderPass(const VertexPassDesc& desc, ID3D12DeviceX* device, const Shader& vertexShader, std::optional<std::reference_wrapper<const Shader>> pixelShader) {
				this->CompileVertexPass(desc, device, vertexShader, pixelShader); 
			}
			
			RenderPass(const MeshletPassDesc& desc, ID3D12DeviceX* device, std::optional<std::reference_wrapper<const Shader>> amplificationShader, const Shader& meshShader, std::optional<std::reference_wrapper<const Shader>> pixelShader) {
				this->CompileMeshletPass(desc, device, amplificationShader, meshShader, pixelShader);
			}

			RenderPass(ID3D12DeviceX* device, const Shader& computeShader) {
				this->CompileComputePass(device, computeShader);
			}

			ERenderPassType GetType() const { return m_Type; }

			std::optional<std::reference_wrapper<BufferBinding>> GetBufferBinding(std::string_view name);
			std::optional<std::reference_wrapper<ConstantBinding>> GetConstantBinding(std::string_view name);

			bool CompileVertexPass(const VertexPassDesc& desc, ID3D12DeviceX* device, const Shader& vertexShader, std::optional<std::reference_wrapper<const Shader>> pixelShader);
			bool CompileMeshletPass(const MeshletPassDesc& desc, ID3D12DeviceX* device, std::optional<std::reference_wrapper<const Shader>> amplificationShader, const Shader& meshShader, std::optional<std::reference_wrapper<const Shader>> pixelShader);
			bool CompileComputePass(ID3D12DeviceX* device, const Shader& computeShader);

			void Begin(RenderAPI::CommandList& cmdList);

		private:
			void ExtractRootParameters(const Shader& shader, D3D12_SHADER_VISIBILITY shaderVisType, std::vector<D3D12_ROOT_PARAMETER>& inoutRootParams, MappedBufferBindings& inoutBufferBindings, MappedConstantBindings& inoutConstantBindings);
			bool CreateRootSignature(ID3D12DeviceX* device, const std::vector<D3D12_ROOT_PARAMETER>& rootParams);

			bool CreateVertexPipelineState(const VertexPassDesc& desc, ID3D12DeviceX* device, const Shader& vertexShader, std::optional<std::reference_wrapper<const Shader>> pixelShader);
			bool CreateMeshletPipelineState(const MeshletPassDesc& desc, ID3D12DeviceX* device, std::optional<std::reference_wrapper<const Shader>> amplificationShader, const Shader& meshShader, std::optional<std::reference_wrapper<const Shader>> pixelShader);
			bool CreateComputePipelineState(ID3D12DeviceX* device, const Shader& computeShader);

			ERenderPassType m_Type = ERenderPassType::INVALID;
			ETopologyType m_TopologyType = ETopologyType::UNDEFINED;
			MappedBufferBindings m_BufferBindings;
			MappedConstantBindings m_ConstantBindings;
			Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
			Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
		};
	}
}