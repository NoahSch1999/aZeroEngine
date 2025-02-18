#pragma once

namespace aZero
{
	class NonMovable
	{
	public:
		NonMovable() = default;
		NonMovable(NonMovable&&) = delete;
		NonMovable& operator=(NonMovable&&) = delete;
	};
}