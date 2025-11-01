#include "renderer/render_graph/RenderGraph.hpp"

namespace aZero
{
	namespace Pipeline
	{
		class PipelineManager
		{
			ID3D12Device* m_Device;
			CComPtr<IDxcCompiler3> m_Compiler;
			std::string m_ShaderFolderPath;
			std::unordered_map<std::string, std::shared_ptr<D3D12::Shader>> m_Shaders;

		public:
			PipelineManager() = default;

			PipelineManager(ID3D12Device* device, const std::string& shaderFolderPath)
			{
				m_Device = device;
				m_ShaderFolderPath = shaderFolderPath;
				DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_Compiler));
			}

			std::weak_ptr<D3D12::Shader> LoadShader(const std::string& fileName)
			{
				std::shared_ptr<D3D12::Shader> shader;
				if (m_Shaders.find(fileName) == m_Shaders.end())
				{
					m_Shaders[fileName] = std::make_shared<D3D12::Shader>(D3D12::Shader());
					shader = m_Shaders[fileName];
				}
				else
				{
					shader = m_Shaders.at(fileName);
				}

				if (!shader->CompileFromFile(*m_Compiler.p, m_ShaderFolderPath + fileName))
				{
					return std::weak_ptr<D3D12::Shader>();
				}
				
				return shader;
			}

			void RemoveShader(const std::string& fileName)
			{
				if (m_Shaders.find(fileName) != m_Shaders.end())
				{
					m_Shaders.erase(fileName);
				}
			}

			std::weak_ptr<D3D12::Shader> GetShader(const std::string& fileName)
			{
				if (m_Shaders.find(fileName) != m_Shaders.end())
				{
					return m_Shaders.at(fileName);
				}
				return std::weak_ptr<D3D12::Shader>();
			}
		};
	}
}