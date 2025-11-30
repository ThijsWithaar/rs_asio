#include "RSWineUsbDevice.h"

#include <cassert>
#include <string>
#include <fstream>
#include <format>

#include <libudev.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

// Windows defines this somewhere, usbdevice_fs has it as struct member name
#define ITF_BACK interface
#undef interface
#include <linux/usbdevice_fs.h>
#define interface ITF_BACK

constexpr std::string_view sysfs_path = "/sys/bus/usb";


std::vector<std::byte> ReadFile(std::string fname)
{
	std::ifstream file(fname, std::ios::binary | std::ios::ate);
	file.exceptions(std::ios::badbit);

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<std::byte> buffer(size);
	file.read(reinterpret_cast<char*>(buffer.data()), size);
	return buffer;
}


struct InterfaceSetting
{
	int itf;
	int alt;
	int address;
};


InterfaceSetting GetSetting(std::span<EndPoint> endpoints, bool input)
{
	for(auto ep: endpoints)
	{
		if(ep.endpointAddress.IsInput() == input)
		{
			return InterfaceSetting{ep.interfaceNr, ep.altsetting, ep.endpointAddress.Address()};
		}
	}
	return {0,0, 0};
}



/// Only {vendor:product}, to allow matching not specifying the serial number
std::string productName(RSWineUsbDevice::ID id)
{
	return std::format("wineusb-{}:{}", id.vendor, id.product);
}


/// Unique name for the device, should be consistent accross reboots.
/// When using multiple of the same USB devices, use this to distuingish them.
std::string uniqueName(RSWineUsbDevice::ID id)
{
	return std::format("wineusb-{}:{}:{}", id.vendor, id.product, id.serial);
}


bool contains(std::vector<RSWineUsbDevice::ID> ids, std::string desc)
{
	auto it = std::find_if(begin(ids), end(ids), [&desc](const RSWineUsbDevice::ID& id){
		return (uniqueName(id) == desc) || (productName(id) == desc);
	});
	return it != end(ids);
}


std::vector<RSWineUsbDevice::ID> ListWineUsbDevice()
{
	std::vector<RSWineUsbDevice::ID> r;

	struct udev* p_udev = udev_new();

	auto p_udev_enum = udev_enumerate_new(p_udev);
	udev_enumerate_add_match_subsystem(p_udev_enum, "usb");
	// udev_monitor_filter_add_match_subsystem_devtype(mon, "sound", "usb_device");
	udev_enumerate_scan_devices(p_udev_enum);

	for(udev_list_entry* le = udev_enumerate_get_list_entry(p_udev_enum); le != nullptr; le = udev_list_entry_get_next(le))
	{
		auto le_name = udev_list_entry_get_name(le);
		auto dev = udev_device_new_from_syspath(p_udev, le_name);

		RSWineUsbDevice::ID id;
		id.vendor = udev_device_get_sysattr_value(dev, "idVendor");
		id.product = udev_device_get_sysattr_value(dev, "idProduct");
		id.serial = udev_device_get_sysattr_value(dev, "serial");
		//id.serial = udev_device_get_property_value(dev, "ID_SERIAL"));
		id.devNode = udev_device_get_devnode(dev);
		udev_device_unref(dev);

		rslog::info_ts() << std::format("{}: {} at {}\n", __FUNCTION__, uniqueName(id), id.devNode) << std::flush;
		r.push_back(id);
	}

	udev_enumerate_unref(p_udev_enum);
	udev_unref(p_udev);

	return r;
}


//-- RSWineUsbDevice --
	

RSWineUsbDevice::RSWineUsbDevice(Config cfg):
	m_devHandle(-1)
{
	auto devList = ListWineUsbDevice();
	auto itDev = std::find_if(begin(devList), end(devList), [&cfg](const RSWineUsbDevice::ID& id){
		return (uniqueName(id) == cfg.devName) || (productName(id) == cfg.devName);
	});
	if(itDev == std::end(devList))
	{
		rslog::info_ts() << __FUNCTION__  << " Device not found" << std::endl;
		return;
	}

	auto node = itDev->devNode;
	m_devHandle = open(node.c_str(), O_RDWR);
}


RSWineUsbDevice::~RSWineUsbDevice()
{
	rslog::info_ts() << __FUNCTION__  << std::endl;
	close(m_devHandle);
}


HRESULT STDMETHODCALLTYPE RSWineUsbDevice::Activate(const IID&, DWORD, PROPVARIANT*, void**)
{
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE RSWineUsbDevice::OpenPropertyStore(DWORD, IPropertyStore**)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE RSWineUsbDevice::GetId(WCHAR**)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE RSWineUsbDevice::GetState(DWORD*)
{
	return E_NOTIMPL;
}


std::vector<EndPoint> RSWineUsbDevice::ListEndPoints()
{
	usbdevfs_conninfo_ex cie;
	int r = ioctl(m_devHandle, USBDEVFS_CONNINFO_EX(0), &cie);

	// Use SYSFS to get the device descriptor:
	std::string fnDescriptors = std::format("{}/devices/{}-{}/descriptors", sysfs_path, cie.devnum, cie.busnum);
	auto buf = ReadFile(fnDescriptors);
	auto rv = ParseEndPoints(buf);
	return rv;
}


void RSWineUsbDevice::SetInterface(int interface, int altsetting)
{
	struct usbdevfs_setinterface setintf;
	setintf.interface = interface;
	setintf.altsetting = altsetting;
	int r = ioctl(m_devHandle, USBDEVFS_SETINTERFACE, &setintf);
}


void RSWineUsbDevice::ClaimInteface(unsigned int itf)
{
	int r = ioctl(m_devHandle, USBDEVFS_CLAIMINTERFACE, &itf);
}


void RSWineUsbDevice::ReleaseInterface(unsigned int itf)
{
	int r = ioctl(m_devHandle, USBDEVFS_RELEASEINTERFACE, &itf);
}


void RSWineUsbDevice::IoctlStreams(int code, int nStreams, std::span<unsigned char> endpoints)
{
	std::vector<std::byte> streamData(sizeof(usbdevfs_streams) + endpoints.size());
	usbdevfs_streams* streams = reinterpret_cast<usbdevfs_streams*>(streamData.data());
	streams->num_streams = nStreams;
	streams->num_eps = endpoints.size();
	memcpy(streams->eps, endpoints.data(), endpoints.size());

	int r = ioctl(m_devHandle, code, streams);
}


void RSWineUsbDevice::AllocStreams(int nStreams, std::span<unsigned char> endpoints)
{
	IoctlStreams(USBDEVFS_ALLOC_STREAMS, nStreams, endpoints);
}


void RSWineUsbDevice::FreeStreams(std::span<unsigned char> endpoints)
{
	IoctlStreams(USBDEVFS_FREE_STREAMS, 0, endpoints);
}


void RSWineUsbDevice::SubmitIso()
{
	usbdevfs_urb urb;
	urb.type = USBDEVFS_URB_TYPE_ISO;
	urb.flags = USBDEVFS_URB_ISO_ASAP;
	int r = ioctl(m_devHandle, USBDEVFS_SUBMITURB, &urb);
}


void RSWineUsbDevice::StartStreaming()
{
// https://stackoverflow.com/a/41635610
	int itf, alt;

	auto ep = ListEndPoints();
	auto ifs = GetSetting(ep, true);

	ClaimInteface(ifs.itf);
	SetInterface(ifs.itf, ifs.alt);
	// AllocStreams() ?
	SubmitIso();

	ReleaseInterface(itf);
}
