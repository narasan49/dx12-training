#include "stdafx.h"
#include "PMDUtil.h"
#include <tchar.h>
#include <DirectXTex.h>
#include <d3d12.h>
#include <vector>

string GetTexturePathFromModelAndTexPath(const string& modelPath, const char* texPath)
{
	auto folderPath = modelPath.substr(0, modelPath.rfind('/') + 1);
	return folderPath + texPath;
}

wstring GetWideStringFromString(const string& str)
{
	// åƒÇ—èoÇµ1âÒñ⁄
	auto num1 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		nullptr,
		0
	);

	std::wstring wstr;
	wstr.resize(num1);

	auto num2 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		&wstr[0],
		num1
	);
	;
	assert(num1 == num2);
	return wstr;
}

std::string GetExtention(const std::string& path)
{
	int idx = path.rfind('.');
	return path.substr(idx + 1, path.length() - idx - 1);
}

std::vector<string> SplitFileName(const string& path, const char splitter)
{
	int offset = 0;
	std::vector<string> ret;
	while (true)
	{
		int idx = path.find(splitter, offset);
		if (idx == string::npos)
		{
			ret.push_back(path.substr(offset));
			break;
		}
		ret.push_back(path.substr(offset, idx - offset));
		offset += idx + 1;
	}
	return ret;
}