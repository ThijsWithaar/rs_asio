#include "LibUsbDeviceEnum.h"

#include <libusb-1.0/libusb.h>

#include "RSWineUsbDevice.h"


struct Context
{
	Context()
	{
		libusb_init(&ctx);
	}

	~Context()
	{
		libusb_exit(ctx);
	}

	libusb_context *ctx = nullptr;
};

libusb_context* GetUsbContext()
{
	static Context ctx;
	return ctx.ctx;
}

void LibUsbDeviceEnum::UpdateAvailableDevices()
{
	libusb_device **devlist;
	ssize_t N = libusb_get_device_list(NULL, &devlist);
	for(int n=0; n < N; n++)
	{
		libusb_device_descriptor desc;
		libusb_get_device_descriptor(devlist[n], &desc);
		rslog::info_ts() << __FUNCTION__ << " Vendor " << desc.idVendor << " Product " << desc.idProduct << std::endl;

		// ToDo: Check if it has an audio configuration

		// Todo: add to m_RenderDevices and m_CaptureDevices
		RSWineUsbDevice::Config config;
		//config.devName = m_Config.output.asioDriverName;
		auto device = new RSWineUsbDevice(config);
		m_RenderDevices.AddDevice(device);
		device->Release();
	}
	libusb_free_device_list(devlist, 1);

	m_DeviceListNeedsUpdate = false;
}
