#pragma once
#include "graphics_api/D3D12Include.hpp"
#include <algorithm>

namespace aZero
{
	namespace Helper
	{
		// todo These functions aren't nice. Re-think if and how they are needed and refactor...
		std::string GetProjectDirectory();
#if USE_DEBUG
		std::string GetDebugProjectDirectory();
#endif
		//

		template<typename ValueType>
		std::string HandleNameCollision(const std::string& name, const std::unordered_map<std::string, ValueType>& map)
		{
			std::string tempName;
			while (map.count(name) > 0)
			{
				uint32_t incr = 0;
				tempName = name + "_" + std::to_string(incr);
				incr++;
			}

			if (tempName.length() > 0)
			{
				return tempName;
			}

			return name;
		}

		inline std::string GetPathSuffix(const std::string& filePath)
		{
			const size_t lastDot = filePath.find_last_of('.');
			return filePath.substr(lastDot + 1, filePath.length() - (filePath.length() - lastDot));
		}

		inline std::string GetFilenameFromPath(const std::string& filePath)
		{
			const size_t lastSlash = filePath.find_last_of('/');
			if (lastSlash != std::wstring::npos)
			{
				return filePath.substr(lastSlash + 1, filePath.length() - lastSlash);
			}
			return "";
		}

		inline std::string StripSuffixFromFilePath(const std::string& filePath)
		{
			const size_t lastDot = filePath.find_last_of('.');
			return filePath.substr(0, lastDot);
		}

		inline uint32_t Pack8To32(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
		{
			return 
				(static_cast<uint32_t>(d) << 24) |
				(static_cast<uint32_t>(c) << 16) |
				(static_cast<uint32_t>(b) << 8) |
				(static_cast<uint32_t>(a));
		}

		inline std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> Unpack32To8(uint32_t value32Bit)
		{
			return std::make_tuple(static_cast<uint8_t>(value32Bit & 0xFFFF), static_cast<uint8_t>((value32Bit >> 8) & 0xFFFF), static_cast<uint8_t>((value32Bit >> 16) & 0xFFFF), static_cast<uint8_t>((value32Bit >> 24) & 0xFFFF));
		}

		inline uint32_t Pack16To32(uint16_t low, uint16_t high) {
			return (static_cast<uint32_t>(high) << 16) | low;
		}

		inline std::tuple<uint16_t, uint16_t> Unpack32To16(uint32_t value32Bit)
		{
			return std::make_tuple(static_cast<uint16_t>(value32Bit & 0xFFFF), static_cast<uint16_t>((value32Bit >> 16) & 0xFFFF));
		}
	}
}