#pragma once
#include <string>
#include <unordered_map>
#include <optional>

#include "Core/Renderer/D3D12Wrap/Pipeline/Shader.h"

#define SHADER_OBJECT_DIRECTORY "../x64/Debug/"

namespace aZero
{
	class ShaderCache
	{
	private:
		std::unordered_map<std::string, D3D12::Shader> m_shaders;

	public:
		ShaderCache()
		{

		}

		~ShaderCache()
		{

		}

		D3D12::Shader* GetShader(const std::string& shaderSourceFileName)
		{
			if (auto foundShader = m_shaders.find(shaderSourceFileName); foundShader != m_shaders.end())
			{
				return &foundShader->second;
			}

			return nullptr;
		}

		// TODO - Change so it parses (add params etc), compiles and stores the shader
		// TODO - Remove macro and TYPE spec
		void CompileAndStore(const std::string& shaderSourceFileName)
		{
			if (m_shaders.count(shaderSourceFileName) == 0)
			{
				const std::string absolutePath(SHADER_OBJECT_DIRECTORY + shaderSourceFileName + ".cso"); // TODO - Change extension to .pso
				D3D12::Shader newShader(absolutePath);
				m_shaders.emplace(shaderSourceFileName, std::move(newShader));
			}
		}

		void Remove(const std::string& shaderSourceFileName)
		{
			m_shaders.erase(shaderSourceFileName);
		}

		// TODO - Implement move and copy constructors / operators
		ShaderCache(const ShaderCache&) = delete;
		ShaderCache(ShaderCache&&) = delete;
		ShaderCache operator=(const ShaderCache&) = delete;
		ShaderCache operator=(ShaderCache&&) = delete;
	};
}