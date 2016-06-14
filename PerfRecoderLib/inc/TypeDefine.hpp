#pragma once
#include <string>
#include <cstdint>

struct TotalGPUsageData
{
	std::wstring name;
	float usage;
	uint64_t dedicatedUsage;
	uint64_t sharedUsage;
	uint64_t sharedLimit;
	uint64_t dedicatedLimit;
};
