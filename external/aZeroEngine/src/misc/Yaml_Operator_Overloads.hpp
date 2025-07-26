#pragma once

#include "graphics_api/D3D12Include.hpp"
#include "yaml-cpp/yaml.h"

inline YAML::Emitter& operator<<(YAML::Emitter& out, const DXM::Vector3& vec)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << vec.x << vec.y << vec.z << YAML::EndSeq;
	return out;
}

inline YAML::Emitter& operator<<(YAML::Emitter& out, const DXM::Vector4& vec)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << vec.x << vec.y << vec.z << vec.w << YAML::EndSeq;
	return out;
}

inline YAML::Emitter& operator<<(YAML::Emitter& out, const DXM::Matrix& matrix)
{
	out << YAML::Flow;
	out << YAML::BeginSeq 
		<< DXM::Vector4(matrix.m[0][0], matrix.m[0][1], matrix.m[0][2], matrix.m[0][3])
		<< DXM::Vector4(matrix.m[1][0], matrix.m[1][1], matrix.m[1][2], matrix.m[1][3])
		<< DXM::Vector4(matrix.m[2][0], matrix.m[2][1], matrix.m[2][2], matrix.m[2][3])
		<< DXM::Vector4(matrix.m[3][0], matrix.m[3][1], matrix.m[3][2], matrix.m[3][3])
		<< YAML::EndSeq;
	return out;
}