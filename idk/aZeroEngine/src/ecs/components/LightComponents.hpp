#pragma once
#include "LightData.hpp"

namespace aZero
{
	namespace ECS
	{
		template<typename LightData>
		class LightComponent
		{
		private:
			LightData m_Data;

		public:
			LightComponent() = default;

			LightComponent(const LightData& Data)
				:m_Data(Data)
			{ }

			const LightData& GetData() const { return m_Data; }
			void SetData(const LightData& Data) { m_Data = Data; }
		};

		using DirectionalLightComponent = LightComponent<DirectionalLightData>;
		using PointLightComponent = LightComponent<PointLightData>;
		using SpotLightComponent = LightComponent<SpotLightData>;
	}
}