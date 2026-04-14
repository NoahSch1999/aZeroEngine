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
		public:
			MultiShaderPass() = default;
			MultiShaderPass(MultiShaderPass&& other) noexcept;
			MultiShaderPass& operator=(MultiShaderPass&& other) noexcept;

			void Begin(RenderAPI::CommandList& cmdList, const RenderAPI::DescriptorHeap& resourceHeap, const RenderAPI::DescriptorHeap& samplerHeap, const std::vector<RenderAPI::Descriptor*>& renderTargets, const std::optional<RenderAPI::Descriptor*>& depthStencilTarget) const;

		protected:

			template<typename ShaderType, typename DescriptionType>
			void GenerateBindings(BindingCombo<BufferBinding>& bufferBindings,
				BindingCombo<ConstantBinding>& constantBindings, const DescriptionType& description,
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
			}

			void PostCompile(
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
			}

			template<typename DescriptionType, typename ...Args>
			bool CreateRootSignature(ID3D12DeviceX* device, const DescriptionType& description, Microsoft::WRL::ComPtr<ID3D12RootSignature>& rootSignature, const Args&... args) const
			{
				if (!ShaderPassBase::CreateRootSignature(device, description, rootSignature, args...))
				{
					DEBUG_PRINT("Failed to create root signature.");
					return false;
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
		};
	}
}