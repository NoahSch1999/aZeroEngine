#pragma once
#include <vector>
#include <functional>

namespace aZero
{
	class CallbackExecutor
	{
	private:
		std::vector<std::function<void()>> m_Callbacks;

	public:
		CallbackPlayer() = default;

		void Append(std::function<void()>&& callback)
		{
			m_Callbacks.emplace_back(callback);
		}

		void Execute()
		{
			for (auto& callback : m_Callbacks)
			{
				callback();
			}
			m_Callbacks.clear();
		}
	};
}