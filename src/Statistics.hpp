#pragma once
#include <chrono>
#include <deque>

namespace aZero::Editor
{
	struct Statistics
	{
		struct FPSStatistics
		{
			float Value = 0.f;
			uint32_t FrameCount = 0u;
			std::chrono::high_resolution_clock::time_point LastTime =
				std::chrono::high_resolution_clock::now();
		};
		FPSStatistics FPS;

		enum class ERenderStat { Scene, Wireframe, EditorGUI, ResolveSwapChain, Present };

		struct RenderStatistic
		{
			std::deque<float> Samples;
			float Value = 0.f;
			std::chrono::high_resolution_clock::time_point StartTime =
				std::chrono::high_resolution_clock::now();
		};

		void CalcFPS()
		{
			auto now = std::chrono::high_resolution_clock::now();
			float elapsed =
				std::chrono::duration<float>(now - FPS.LastTime).count();

			if (elapsed >= 1.0f)
			{
				FPS.Value = FPS.FrameCount / elapsed;
				FPS.FrameCount = 0;
				FPS.LastTime = now;
			}
			FPS.FrameCount++;
		}

		void PreRenderCalc(ERenderStat stat)
		{
			RenderStatistics[stat].StartTime = std::chrono::high_resolution_clock::now();
		}

		void PostRenderCalc(ERenderStat stat)
		{
			auto end = std::chrono::high_resolution_clock::now();

			RenderStatistics[stat].Samples.push_back(std::chrono::duration<float, std::milli>(end - RenderStatistics[stat].StartTime).count());
		}

		float GetRenderStat(ERenderStat stat) {
			if (RenderStatistics[stat].Samples.size() > 60)
			{
				RenderStatistics[stat].Samples.pop_front();
			}

			float sum = 0.f;
			for (float x : RenderStatistics[stat].Samples)
				sum += x;

			if (sum != 0.f)
			{
				RenderStatistics[stat].Value = sum / RenderStatistics[stat].Samples.size();
			}

			return RenderStatistics[stat].Value; 
		} // Non-const since we wanna zero-init with [] if fetched the first time before any calculations

		std::unordered_map<ERenderStat, RenderStatistic> RenderStatistics;
	};
}