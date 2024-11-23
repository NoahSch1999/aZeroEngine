#pragma once
#include "Core/D3D12Core.h"

namespace aZero
{
	namespace Asset
	{
		template<typename RenderData>
		class MaterialTemplate
		{
		private:
			int m_UniqueID = 0;
			std::string m_Name;

			RenderData m_RenderData;
		public:
			MaterialTemplate() = default;
			MaterialTemplate(const std::string& Name, const RenderData& InitRenderData)
			{
				this->Init(Name, InitRenderData);
			}

			void Init(const std::string& Name, const RenderData& InitRenderData)
			{
				m_Name = Name;
				m_RenderData = InitRenderData;
			}

			void SetRenderData(const RenderData& Data) { m_RenderData = Data; }
			const RenderData& GetRenderData() const { return m_RenderData; }

			int GetID() const { return m_UniqueID; }
			void SetID(int NewID) { m_UniqueID = NewID; }
			const std::string& GetName() const { return m_Name; }
		};

		struct BasicMaterialRenderData
		{
			int m_DiffuseTextureIndex = 0;
			int m_NormalMapTextureIndex = 0;
		};
	}
}