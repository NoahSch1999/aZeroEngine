#include "pipeline/VertexShader.hpp"
#include "pipeline/PixelShader.hpp"
#include "pipeline/ComputeShader.hpp"

namespace aZero
{
	namespace Pipeline
	{
		// TODO: Remove and simply provide the renderpass, rendergraph and shader-subclass interfaces?
		class PipelineManager
		{
			ID3D12Device* m_Device;
			CComPtr<IDxcCompiler3> m_Compiler;
			std::string m_ShaderFolderPath;
			std::unordered_map<std::string, std::shared_ptr<Pipeline::VertexShader>> m_VertexShaders;

		public:
			PipelineManager() = default;

			PipelineManager(ID3D12Device* device, const std::string& shaderFolderPath)
			{
				m_Device = device;
				m_ShaderFolderPath = shaderFolderPath;
				DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_Compiler));
			}

			IDxcCompiler3& GetCompiler() { return *m_Compiler.p; }
			const std::string& GetShaderFolderPath() const { return m_ShaderFolderPath; }

			std::weak_ptr<Pipeline::VertexShader> LoadShader(const std::string& fileName)
			{
				std::shared_ptr<Pipeline::VertexShader> shader;
				if (m_VertexShaders.find(fileName) == m_VertexShaders.end())
				{
					m_VertexShaders[fileName] = std::make_shared<Pipeline::VertexShader>(Pipeline::VertexShader());
					shader = m_VertexShaders[fileName];
				}
				else
				{
					shader = m_VertexShaders.at(fileName);
				}

				if (!shader->CompileFromFile(*m_Compiler.p, m_ShaderFolderPath + fileName))
				{
					return std::weak_ptr<Pipeline::VertexShader>();
				}
				
				return shader;
			}

			void RemoveShader(const std::string& fileName)
			{
				if (m_VertexShaders.find(fileName) != m_VertexShaders.end())
				{
					m_VertexShaders.erase(fileName);
				}
			}

			std::weak_ptr<Pipeline::VertexShader> GetShader(const std::string& fileName)
			{
				if (m_VertexShaders.find(fileName) != m_VertexShaders.end())
				{
					return m_VertexShaders.at(fileName);
				}
				return std::weak_ptr<Pipeline::VertexShader>();
			}
		};
	}
}