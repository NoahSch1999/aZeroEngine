#pragma once
#include <variant>

#include "graphics_api/resource/texture/Texture2D.hpp"
#include "graphics_api/descriptor/Descriptor.hpp"

namespace aZero
{
	class Engine;
	namespace Rendering
	{
		class RenderSurface : public NonCopyable
		{
		public:
			struct DepthClearValue {
				float depthValue;
				uint8_t stencilValue;
			};

			struct ColorClearValue {
				DXM::Vector4 colorValue;
			};

			RenderSurface() = default;

			template<typename ClearValue>
			RenderSurface(RenderAPI::Descriptor* descriptor, const ClearValue& clearValue, bool shouldClear)
				:m_Descriptor(descriptor), m_ShouldClear(shouldClear) {
				this->SetClearState(clearValue);
			}

			/*void SetTexture(RenderAPI::Texture2D* texture) {
				m_Texture = texture;
			}*/

			void SetDescriptor(RenderAPI::Descriptor* descriptor) {
				m_Descriptor = descriptor;
			}

			template<typename ClearValue>
			void SetClearValue(const ClearValue& clearValue) {
				m_ClearValue.emplace(clearValue);
			}

			void SetClearState(bool shouldClear) {
				m_ShouldClear = shouldClear;
			}

		private:
			//RenderAPI::Texture2D* m_Texture = nullptr;
			RenderAPI::Descriptor* m_Descriptor = nullptr;
			std::variant<DepthClearValue, ColorClearValue> m_ClearValue;
			bool m_ShouldClear = false;
		};
	}
}