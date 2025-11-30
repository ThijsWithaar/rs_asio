#pragma once
#include "../stdafx.h"

//#include <mmdeviceapi.h>
#include "../ComBaseUnknown.h"

#include "Descriptor.h"


/**
Devices for running under WINE, direct connection to USB-audio devices.

This skips ASIO and on linux JACK/Pipewire.
Should have the lowest latency of all options
*/
class RSWineUsbDevice : public ComBaseUnknown<IMMDevice>
{
public:
	struct ID
	{
		std::string vendor, product, serial;
		std::string devNode;
	};

	struct Config
	{
		std::string devName;
	};

	RSWineUsbDevice(Config cfg);

	~RSWineUsbDevice();

	HRESULT STDMETHODCALLTYPE Activate(const IID&, DWORD, PROPVARIANT*, void**) override;
	HRESULT STDMETHODCALLTYPE OpenPropertyStore(DWORD, IPropertyStore**) override;
	HRESULT STDMETHODCALLTYPE GetId(WCHAR**) override;
	HRESULT STDMETHODCALLTYPE GetState(DWORD*) override;

private:
	std::vector<EndPoint> ListEndPoints();

	void SetInterface(int itf, int altsetting);
	void ClaimInteface(unsigned int itf);
	void ReleaseInterface(unsigned int itf);
	void IoctlStreams(int code, int nStreams, std::span<unsigned char> endpoints);
	void AllocStreams(int nStreams, std::span<unsigned char> endpoints);
	void FreeStreams(std::span<unsigned char> endpoints);
	void SubmitIso();

	void StartStreaming();

	int m_devHandle;
};

// List all currently present USB devices
std::vector<RSWineUsbDevice::ID> ListWineUsbDevice();

std::string uniqueName(RSWineUsbDevice::ID);

bool contains(std::vector<RSWineUsbDevice::ID>, std::string desc);
