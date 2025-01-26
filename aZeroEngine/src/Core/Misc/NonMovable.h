#pragma once

namespace aZero
{
	class NonMovable
	{
	public:
		NonMovable() = default;
		NonMovable(const NonMovable&) = delete;
		NonMovable& operator=(const NonMovable&) = delete;
	};
}