#pragma once
#include <vector>
#include <functional>
#include <mutex>

namespace aZero
{
	class CallbackExecutor
	{
	private:
		std::vector<std::function<void()>> m_Callbacks;
		std::mutex m_Lock;
	public:
		CallbackExecutor() = default;

		void Append(std::function<void()>&& callback)
		{
			std::unique_lock<std::mutex> lock(m_Lock);
			m_Callbacks.emplace_back(std::move(callback));
		}

		void Execute()
		{
			std::unique_lock<std::mutex> lock(m_Lock);
			for (auto& callback : m_Callbacks)
			{
				callback();
			}
			m_Callbacks.clear();
		}
	};
}