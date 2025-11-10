#pragma once
#include "Core.h"

enum class ShaderType
{
	Vertex,
	Pixel,
	Compute,
	Hull,
	Domain
};

struct ShaderData
{
	std::vector<uint32_t> bytecode;
};

class ShaderCompiler 
{
public:
	static void CompileShaderFromFile(const std::string& path, ShaderType shaderType, ShaderData& outShaderData, const std::vector<std::pair<std::wstring, std::wstring>>& defines = {});
};