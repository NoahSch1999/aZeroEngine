#pragma once
#include "ShaderPassBase.hpp"
#include "pipeline/shader/PixelShader.hpp"
#include "misc/SparseMappedVector.hpp"

namespace aZero
{
	namespace Pipeline
	{
		struct MultiShaderPassDesc
		{
			struct DepthStencil
			{
				DXGI_FORMAT m_Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
			};

			struct RenderTarget
			{
				DXGI_FORMAT m_Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
				std::string m_Name;
			};

			std::vector<RenderTarget> m_RenderTargets;
			DepthStencil m_DepthStencil;
		};

		class MultiShaderPass : public ShaderPassBase
		{
		protected:
			DataStructures::SparseMappedVector<std::string, RenderAPI::Descriptor*> m_RenderTargets;
			std::optional<RenderAPI::Descriptor*> m_DepthStencilTarget;

			void Reset()
			{
				m_RenderTargets = DataStructures::SparseMappedVector<std::string, RenderAPI::Descriptor*>();
				m_DepthStencilTarget.reset();
			}

			template<typename ShaderType, typename DescriptionType>
			void GenerateBindings(BindingCombo<BufferBinding>& bufferBindings,
				BindingCombo<ConstantBinding>& constantBindings, DataStructures::SparseMappedVector<std::string, RenderAPI::Descriptor*>& renderTargets, const DescriptionType& description,
				const ShaderType& vertexGenerationShader, std::optional<Pipeline::PixelShader*> pixelShader) const
			{
				if (pixelShader.has_value())
				{
					Pipeline::PixelShader& pixelShaderRef = *pixelShader.value();
					ShaderPassBase::GenerateBindings(bufferBindings, constantBindings, vertexGenerationShader, pixelShaderRef);
				}
				else
				{
					ShaderPassBase::GenerateBindings(bufferBindings, constantBindings, vertexGenerationShader);
				}

				this->GenerateOutputBindings<MultiShaderPassDesc::RenderTarget>(description, renderTargets);
			}

			void PostCompile(DataStructures::SparseMappedVector<std::string, RenderAPI::Descriptor*>&& renderTargets,
				Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState,
				Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature,
				BindingCombo<BufferBinding>&& bufferBindings,
				BindingCombo<ConstantBinding>&& constantBindings
			)
			{
				ShaderPassBase::PostCompile(
					pipelineState,
					rootSignature,
					std::move(bufferBindings),
					std::move(constantBindings)
				);
				m_RenderTargets = std::move(renderTargets);
				m_DepthStencilTarget.reset();
			}

			template<typename T, typename DescriptionType>
			bool GenerateOutputBindings(const DescriptionType& description, DataStructures::SparseMappedVector<std::string, RenderAPI::Descriptor*>& outTargets) const
			{
				for (int32_t rtvIndex = 0; rtvIndex < description.m_RenderTargets.size(); rtvIndex++)
				{
					const T& rtv = description.m_RenderTargets.at(rtvIndex);
					if (rtv.m_Name.empty())
					{
						DEBUG_PRINT("Render target at index " + std::to_string(rtvIndex) + " didn't have a name");
						return false;
					}
					if (outTargets.Contains(rtv.m_Name))
					{
						DEBUG_PRINT("Render target at index " + std::to_string(rtvIndex) + " with name " + rtv.m_Name + " already exist.");
						return false;
					}

					outTargets.AddOrUpdate(rtv.m_Name, nullptr);
				}
				return true;
			}

			template<typename ShaderType, typename DescriptionType>
			bool CreateRootSignature(ID3D12DeviceX* device, const DescriptionType& description, const ShaderType& vertexGenerationShader, std::optional<Pipeline::PixelShader*> pixelShader, Microsoft::WRL::ComPtr<ID3D12RootSignature>& rootSignature) const
			{
				if (pixelShader.has_value())
				{
					Pipeline::PixelShader& pixelShaderRef = *pixelShader.value();
					if (!ShaderPassBase::CreateRootSignature(device, description, rootSignature, vertexGenerationShader, pixelShaderRef))
					{
						DEBUG_PRINT("Failed to create root signature.");
						return false;
					}
				}
				else
				{
					if (!ShaderPassBase::CreateRootSignature(device, description, rootSignature, vertexGenerationShader))
					{
						DEBUG_PRINT("Failed to create root signature.");
						return false;
					}
				}
				return true;
			}

			template<typename T>
			bool ValidatePassInputs(const T& description) const
			{
				if (description.m_DepthStencil.m_Format == DXGI_FORMAT::DXGI_FORMAT_UNKNOWN && description.m_RenderTargets.empty())
				{
					DEBUG_PRINT("Need the pass to write to atleast an rtv or dsv.");
					return false;
				}

				if (description.m_RenderTargets.size() > 8)
				{
					DEBUG_PRINT("Pass doesn't allow more than 8 rtvs.");
					return false;
				}

				return true;
			}

		public:
			MultiShaderPass() = default;

			void Bind(RenderAPI::CommandList& cmdList) const;
		};
	}
}