#pragma once
#ifdef RUN_TESTS
#include "aZeroEngine/Engine.hpp"

inline bool CreateRenderPasses(const aZero::Engine& engine)
{
	using namespace aZero;
	Pipeline::VertexShader vs;
	bool vsCompRes = vs.CompileFromFile(engine.GetCompiler(), std::string(TEST_PATH) + "content/shaders/test.vs.hlsl");

	Pipeline::MeshShader ms;
	bool msCompRes = ms.CompileFromFile(engine.GetCompiler(), std::string(TEST_PATH) + "content/shaders/test.ms.hlsl");

	Pipeline::PixelShader ps;
	bool psCompRes = ps.CompileFromFile(engine.GetCompiler(), std::string(TEST_PATH) + "content/shaders/test.ps.hlsl");

	Pipeline::VertexShaderPass vsPass;
	Pipeline::VertexShaderPass::Description vsPassDesc;
	vsPassDesc.m_RenderTargets = { {DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, "Color"}};
	vsPassDesc.m_TopologyType = Pipeline::VertexShaderPass::TopologyType::TRIANGLE;
	bool vsPassRes = vsPass.Compile(engine.GetDevice(), vsPassDesc, vs, &ps);

	Pipeline::MeshShaderPass msPass;
	Pipeline::MeshShaderPass::Description msPassDesc;
	msPassDesc.m_RenderTargets = { {DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, "Color"}};
	bool msPassRes = msPass.Compile(engine.GetDevice(), msPassDesc, {}, ms, &ps);

	return (vsCompRes && msCompRes && psCompRes && vsPassRes && msPassRes);
}

inline void RunTests(const aZero::Engine& engine)
{
	if (!CreateRenderPasses(engine))
		std::cout << "CreateRenderPasses failed\n";
}
#endif