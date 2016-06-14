#include "stdafx.h"
#include "GPUResourceUsage.hpp"
#include "NvidiaGPUResourceUsage.hpp"

GPUResourceUsage::GPUResourceUsage()
{
	m_vendors.push_back(std::make_shared<NvidiaGPUResourceUsage>());
}

GPUResourceUsage::~GPUResourceUsage()
{

}

GPUResourceUsage & GPUResourceUsage::getInstance()
{
	static std::auto_ptr<GPUResourceUsage> instance;
	if (instance.get() == nullptr)
		instance.reset(new GPUResourceUsage);
	return *instance;
}

std::vector<std::shared_ptr<GPUResourceUsageData>> GPUResourceUsage::getUsages()
{
	std::vector<std::shared_ptr<GPUResourceUsageData>> result;
	for (auto &vendor : m_vendors) {
		auto usages = vendor->getUsages();
		result.insert(result.end(), usages.begin(), usages.end());
	}
	return result;
}
