#pragma once
#include <span>
#include <optional>
#include "graphics_api/Wrappers/CommandWrapping.hpp"
#include "graphics_api/Wrappers/ResourceWrapping.hpp"
#include "renderer/RenderSurface.hpp"

namespace aZero
{
	namespace Pipeline
	{
		class NewRenderPass
		{
		public:
			// Currently not supporting CBV since we can't differentiate them from constants when reflecting. The performance of SRV is pretty much the same on modern hardware anyways.
			enum class BindingType { SRV = D3D12_ROOT_PARAMETER_TYPE_SRV, UAV = D3D12_ROOT_PARAMETER_TYPE_UAV, CONSTANT = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS };
			using RootIndex = uint32_t;

			// todo Support texture transitions?
			struct TextureTransitions
			{
				RenderAPI::Texture2D* m_Texture = nullptr;
				D3D12_RESOURCE_STATES m_InOutState;
				D3D12_RESOURCE_STATES m_AccessState;
			};
			std::unordered_map<uint32_t, TextureTransitions> m_TextureTransitions;

			class BindingBase
			{
				friend NewRenderPass;
				friend class MultiShaderPass;
				friend class ComputeShaderPass;
			private:
				RootIndex m_Index = std::numeric_limits<RootIndex>::max();
			public:
				BindingBase() = default;
				BindingBase(RootIndex index)
					:m_Index(index)
				{

				}
			};

			class BufferBinding : public BindingBase
			{
				friend NewRenderPass;
				friend class MultiShaderPass;
				friend class ComputeShaderPass;
			private:
				RenderAPI::Buffer* m_Buffer = nullptr;
				BindingType m_Type;

			public:
				BufferBinding() = default;
				BufferBinding(BindingType type, RootIndex index)
					:BindingBase(index), m_Type(type)
				{

				}

				void SetBuffer(RenderAPI::Buffer& buffer, BindingType type)
				{
					m_Buffer = &buffer;
					m_Type = type;
				}
			};

			class ConstantBinding : public BindingBase, public NonCopyable
			{
				friend NewRenderPass;
				friend class MultiShaderPass;
				friend class ComputeShaderPass;
			private:
				uint32_t m_NumConstants;
				// Workaround to enable unique_ptr to delete allocated memory thats referenced as a void*
				std::unique_ptr<void, decltype(&std::free)> m_Data;

				void* Alloc(int32_t numBytes)
				{
					return std::malloc(numBytes);
				}
			public:

				ConstantBinding()
					:m_Data(nullptr, &std::free) {
				}

				ConstantBinding(int32_t numConstants, RootIndex index)
					:BindingBase(index), m_Data(this->Alloc(numConstants * sizeof(int32_t)), &std::free), m_NumConstants(numConstants)
				{

				}

				void Write(void* data, int32_t offset, int32_t numConstants)
				{
					if (offset + numConstants >= m_NumConstants)
					{
						throw std::out_of_range("Tried to write out-of-bounds into root constant.");
					}

					memcpy(static_cast<int32_t*>(m_Data.get()) + offset, &data, numConstants);
				}
			};

			template<typename T>
			struct BindingCombo
			{
				std::unordered_map<std::string, uint32_t> m_Name_To_Binding; // Reflection name => root index
				std::vector<T> m_Bindings; // Binding slots
			};

		protected:
			// !note Reuse partially
			void Reset()
			{
				m_PipelineState = nullptr;
				m_RootSignature = nullptr;
				m_BufferBindings.m_Bindings.clear();
				m_BufferBindings.m_Name_To_Binding.clear();
				m_ConstantBindings.m_Bindings.clear();
				m_ConstantBindings.m_Name_To_Binding.clear();
			}

			template<typename ...Args>
			std::vector<D3D12_ROOT_PARAMETER> GenerateRootParams(const Args&...args) const
			{
				std::vector<D3D12_ROOT_PARAMETER> params;
				(params.insert(params.end(), args.GetRootParameters().begin(), args.GetRootParameters().end()), ...);
				return params;
			}

			template<typename T>
			void GenerateBindingsImpl(BindingCombo<BufferBinding>& bufferBindings, BindingCombo<ConstantBinding>& constantBindings, const T& shader) const
			{
				for (const auto& [name, bindingInfo] : shader.GetResourceBindings())
				{
					if (bindingInfo.m_ResourceType != D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
					{
						// Ugly but consise!!!! :)
						const BindingType descriptorType = 
							bindingInfo.m_ResourceType == D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_SRV ? BindingType::SRV
							: bindingInfo.m_ResourceType == D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_UAV ? BindingType::UAV : throw std::runtime_error("Invalid shader binding reflected.");

						bufferBindings.m_Bindings.emplace_back(descriptorType, bindingInfo.m_RootIndex);
						bufferBindings.m_Name_To_Binding[name] = bufferBindings.m_Bindings.size() - 1;
					}
					else
					{
						constantBindings.m_Bindings.emplace_back(bindingInfo.m_Num32BitConstants, bindingInfo.m_RootIndex);
						constantBindings.m_Name_To_Binding[name] = constantBindings.m_Bindings.size() - 1;
					}
				}
			}

			// I <3 fold expressions
			template<typename ...Args>
			void GenerateBindings(BindingCombo<BufferBinding>& bufferBindings, BindingCombo<ConstantBinding>& constantBindings, const Args&...args) const
			{
				(this->GenerateBindingsImpl(bufferBindings, constantBindings, args), ...);
			}

			bool CreateRootSignatureImpl(ID3D12Device* device, ID3D12RootSignature* rootSignature, const std::vector<D3D12_ROOT_PARAMETER>& rootParams) const;

			template<typename DescriptionType, typename ...Args>
			bool CreateRootSignature(ID3D12Device* device, const DescriptionType& description, ID3D12RootSignature* rootSignature, const Args&...args) const
			{
				const std::vector<D3D12_ROOT_PARAMETER> rootParams = this->GenerateRootParams(args...);
				if (!NewRenderPass::CreateRootSignatureImpl(device, rootSignature, rootParams))
				{
					DEBUG_PRINT("Failed to create root signature.");
					return false;
				}

				return true;
			}

			void PostCompile(
				Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState,
				Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature,
				BindingCombo<BufferBinding>&& bufferBindings,
				BindingCombo<ConstantBinding>&& constantBindings
			)
			{
				m_PipelineState = pipelineState;
				m_RootSignature = rootSignature;
				m_BufferBindings = std::move(bufferBindings);
				m_ConstantBindings = std::move(constantBindings);
			}

			Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState = nullptr;
			Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
			BindingCombo<BufferBinding> m_BufferBindings;
			BindingCombo<ConstantBinding> m_ConstantBindings;
		public:
			NewRenderPass() = default;

			bool IsCompiled() const { return m_PipelineState != nullptr; }

			BufferBinding* GetBufferBinding(const std::string& name)
			{
				if (auto ptr = m_BufferBindings.m_Name_To_Binding.find(name); ptr != m_BufferBindings.m_Name_To_Binding.end())
				{
					return &m_BufferBindings.m_Bindings.at(ptr->second);
				}
				return nullptr;
			}

			ConstantBinding* GetConstantBinding(const std::string& name)
			{
				if (auto ptr = m_ConstantBindings.m_Name_To_Binding.find(name); ptr != m_ConstantBindings.m_Name_To_Binding.end())
				{
					return &m_ConstantBindings.m_Bindings.at(ptr->second);
				}
				return nullptr;
			}
		};
	}
}