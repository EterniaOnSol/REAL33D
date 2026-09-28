#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <unordered_set>

namespace Real33D
{
// UI metadata only, read after the existing ClientCore objects.srv validator.
// MULTIUSE is defined by Fusion32, not inferred from names or item ids.
// Server CUseObject rejects MULTIUSE; CUseTwoObjects is its existing target route.
inline std::unordered_set<std::uint16_t> ReadMultiUseTypeIds(const std::string& text)
{
	const auto trim = [](const std::string& value)
	{
		const auto first = value.find_first_not_of(" \t\r");
		return first == std::string::npos ? std::string{}
			: value.substr(first, value.find_last_not_of(" \t\r") - first + 1);
	};
	std::unordered_set<std::uint16_t> result;
	std::istringstream lines(text);
	std::string line;
	unsigned type = 0;
	bool haveType = false;
	while (std::getline(lines, line))
	{
		bool quoted = false;
		for (std::size_t i = 0; i < line.size(); ++i)
		{
			if (line[i] == '"') quoted = !quoted;
			else if (line[i] == '#' && !quoted) { line.resize(i); break; }
		}
		const auto equals = line.find('=');
		if (equals == std::string::npos) continue;
		const std::string key = trim(line.substr(0, equals));
		const std::string value = trim(line.substr(equals + 1));
		if (key == "TypeID")
		{
			type = 0;
			haveType = !value.empty();
			for (const char digit : value)
			{
				if (digit < '0' || digit > '9') { haveType = false; break; }
				type = type * 10 + unsigned(digit - '0');
				if (type >= 0xff00) { haveType = false; break; }
			}
		}
		else if (key == "Flags" && haveType && value.size() >= 2
			&& value.front() == '{' && value.back() == '}')
		{
			std::istringstream flags(value.substr(1, value.size() - 2));
			std::string flag;
			while (std::getline(flags, flag, ','))
				if (trim(flag) == "MultiUse") result.insert(static_cast<std::uint16_t>(type));
		}
	}
	return result;
}
}
