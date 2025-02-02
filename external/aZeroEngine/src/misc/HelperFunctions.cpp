#include "HelperFunctions.hpp"

std::string aZero::Helper::GetProjectDirectory()
{
	std::vector<wchar_t> Path(200);
	GetModuleFileNameW(NULL, Path.data(), Path.size());
	std::wstring ProjectDir(Path.data());
	std::replace(ProjectDir.begin(), ProjectDir.end(), '\\', '/');
	{
		const size_t LastSlash = ProjectDir.find_last_of('/');
		ProjectDir = ProjectDir.substr(0, LastSlash);
	}

	if (ProjectDir.ends_with(L"/x64-Debug"))
	{
		const size_t LastSlash = ProjectDir.find_last_of('/');
		ProjectDir = ProjectDir.substr(0, LastSlash);
		ProjectDir += L"/x64-Release";
	}

	return std::string(ProjectDir.begin(), ProjectDir.end());
}
#ifdef _DEBUG
std::string aZero::Helper::GetDebugProjectDirectory()
{
	std::vector<wchar_t> Path(200);
	GetModuleFileNameW(NULL, Path.data(), Path.size());
	std::wstring ProjectDir(Path.data());
	std::replace(ProjectDir.begin(), ProjectDir.end(), '\\', '/');
	{
		const size_t LastSlash = ProjectDir.find_last_of('/');
		ProjectDir = ProjectDir.substr(0, LastSlash);
	}

	return std::string(ProjectDir.begin(), ProjectDir.end());
}
#endif