#include "ShaderCompiler.h"

void ShaderCompiler::CompileShaderFromFile(const std::string& path, ShaderType shaderType, ShaderData& outShaderData, const std::vector<std::pair<std::wstring, std::wstring>>& defines)
{
	std::ifstream file(path, std::ios::binary);
	if (!file)
		return;

	file.seekg(0, std::ios::end);
	size_t size = (size_t)file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> shaderData(size);
	file.read(shaderData.data(), size);

	HRESULT hr;

	IDxcUtils* dxcUtils;
	IDxcCompiler* compiler;
	IDxcIncludeHandler* defaultIncludeHandler;
	IDxcBlobEncoding* sourceBlob;
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));

	hr = dxcUtils->CreateDefaultIncludeHandler(&defaultIncludeHandler);

	if (FAILED(hr))
	{
		return;
	}

	hr = dxcUtils->CreateBlob(shaderData.data(), shaderData.size(), 0, &sourceBlob);

	if (FAILED(hr))
	{
		return;
	}

	DxcBuffer sourceBuffer;
	sourceBuffer.Ptr = shaderData.data();
	sourceBuffer.Size = shaderData.size();
	sourceBuffer.Encoding = DXC_CP_ACP;

	const char* shaderTypeChar;
	switch (shaderType)
	{
	case ShaderType::Vertex: {
		shaderTypeChar = "vs_6_6";
		break;
	}
	case ShaderType::Pixel: {
		shaderTypeChar = "ps_6_6";
		break;
	}
	case ShaderType::Compute: {
		shaderTypeChar = "cs_6_6";
		break;
	}
	case ShaderType::Hull: {
		shaderTypeChar = "hs_6_6";
		break;
	}
	case ShaderType::Domain: {
		shaderTypeChar = "ds_6_6";
		break;
	}
	}

	wchar_t wideTarget[512];
	swprintf_s(wideTarget, 512, L"%hs", shaderTypeChar);

	std::string entryPoint = "main";
	wchar_t wideEntry[512];
	swprintf_s(wideEntry, 512, L"%hs", entryPoint.c_str());

	LPCWSTR args[] = {
		L"-Zs",
		L"-Fd",
		L"-Fre"
	};

	std::vector<DxcDefine> dxcDefines;
	for (const auto& def : defines)
	{
		dxcDefines.push_back({ def.first.c_str(), def.second.c_str() });
	}

	IDxcOperationResult* result;

	hr = compiler->Compile(
		sourceBlob,
		L"Shader",
		wideEntry,
		wideTarget,
		args,
		ARRAYSIZE(args),
		dxcDefines.empty() ? nullptr : dxcDefines.data(),
		dxcDefines.empty() ? 0 : (UINT32)dxcDefines.size(),
		defaultIncludeHandler,
		&result);

	if (FAILED(hr))
	{
		return;
	}

	IDxcBlobEncoding* errors;
	result->GetErrorBuffer(&errors);

	if (errors && errors->GetBufferSize() != 0)
	{
		IDxcBlobUtf8* pErrorsU8;
		errors->QueryInterface(IID_PPV_ARGS(&pErrorsU8));
		std::string error((char*)pErrorsU8->GetStringPointer());
		std::cout << error << std::endl;
		return;
	}

	HRESULT status;
	result->GetStatus(&status);

	IDxcBlob* shaderBlob;
	result->GetResult(&shaderBlob);

	outShaderData.bytecode.resize(shaderBlob->GetBufferSize() / sizeof(uint32_t));
	memcpy(outShaderData.bytecode.data(), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());
}