#pragma once

#include "../RSBaseDeviceEnum.h"

#include <optional>

class PipeWireDeviceEnum : public RSBaseDeviceEnum
{
public:
	//void SetConfig(const RSAsioConfig& config);

protected:
	void UpdateAvailableDevices() override;

	//RSAsioConfig m_Config;
};
