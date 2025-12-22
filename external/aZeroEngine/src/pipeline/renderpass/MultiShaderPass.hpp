#pragma once
#include "NewRenderPass.hpp"
#include "pipeline/PixelShader.hpp"

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

		class MultiShaderPass : public NewRenderPass
		{
		protected:
			std::unordered_map<std::string, Rendering::RenderSurface*> m_RenderTargets;
			std::optional<Rendering::RenderSurface*> m_DepthStencilTarget;
			void Reset()
			{
				m_RenderTargets.clear();
				m_DepthStencilTarget.reset();
			}

			template<typename ShaderType, typename DescriptionType>
			void GenerateBindings(BindingCombo<BufferBinding>& bufferBindings,
				BindingCombo<ConstantBinding>& constantBindings, std::unordered_map<std::string, Rendering::RenderSurface*>& renderTargets, const DescriptionType& description,
				const ShaderType& vertexGenerationShader, std::optional<Pipeline::PixelShader*> pixelShader) const
			{
				if (pixelShader.has_value())
				{
					Pipeline::PixelShader& pixelShaderRef = *pixelShader.value();
					NewRenderPass::GenerateBindings(bufferBindings, constantBindings, vertexGenerationShader, pixelShaderRef);
				}
				else
				{
					NewRenderPass::GenerateBindings(bufferBindings, constantBindings, vertexGenerationShader);
				}

				this->GenerateOutputBindings<MultiShaderPassDesc::RenderTarget>(description, renderTargets, description.m_RenderTargets);
			}

			void PostCompile(std::unordered_map<std::string, Rendering::RenderSurface*>&& renderTargets,
				Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState,
				Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature,
				BindingCombo<BufferBinding>&& bufferBindings,
				BindingCombo<ConstantBinding>&& constantBindings
			)
			{
				NewRenderPass::PostCompile(
					pipelineState,
					rootSignature,
					std::move(bufferBindings),
					std::move(constantBindings)
				);
				m_RenderTargets = std::move(renderTargets);
				m_DepthStencilTarget.reset();
			}

			template<typename T, typename DescriptionType>
			bool GenerateOutputBindings(const DescriptionType& description, std::unordered_map<std::string, Rendering::RenderSurface*>& outTargets, const std::vector<T>& inTargets) const
			{
				for (int32_t rtvIndex = 0; rtvIndex < description.m_RenderTargets.size(); rtvIndex++)
				{
					const T& rtv = description.m_RenderTargets.at(rtvIndex);
					if (rtv.m_Name.empty())
					{
						DEBUG_PRINT("Render target at index " + std::to_string(rtvIndex) + " didn't have a name");
						return false;
					}

					if (outTargets.contains(rtv.m_Name))
					{
						DEBUG_PRINT("Render target at index " + std::to_string(rtvIndex) + " with name " + rtv.m_Name + " already exist.");
						return false;
					}

					outTargets[rtv.m_Name] = nullptr;
				}
				return true;
			}

			template<typename ShaderType, typename DescriptionType>
			bool CreatePipeline(ID3D12Device* device, const DescriptionType& description, const ShaderType& vertexGenerationShader, std::optional<Pipeline::PixelShader*> pixelShader, ID3D12PipelineState* pipelineState, ID3D12RootSignature* rootSignature) const
			{
				if (pixelShader.has_value())
				{
					Pipeline::PixelShader& pixelShaderRef = *pixelShader.value();
					if (!NewRenderPass::CreateRootSignature(device, description, rootSignature, vertexGenerationShader, pixelShaderRef))
					{
						DEBUG_PRINT("Failed to create root signature.");
						return false;
					}
				}
				else
				{
					if (!NewRenderPass::CreateRootSignature(device, description, rootSignature, vertexGenerationShader))
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