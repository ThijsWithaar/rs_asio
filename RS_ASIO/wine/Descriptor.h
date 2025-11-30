#pragma once
/**
Parsing of USB Device Descriptors.

This tells on what interface/configuration/endpoint the audio devices is,
and what sample-format and -rate it has.
*/

#include <cstdint>
#include <span>
#include <vector>
#include <optional>

namespace Descriptors {

struct uint24_t
{
	uint8_t val[3];
};

enum class DescriptorType: uint8_t
{
	Device = 1,
	Configuration = 2,
	Interface = 4,
	EndPoint = 5,
	AudioStreaming = 36,
	AudioControl = 37,
	Hub = 41,
};

enum class InterfaceClass: uint8_t
{
	Audio = 1,
	Comm = 2,
	HID = 3,
	Physical = 5,
	Printer = 7,
	Hub = 9,
	Video = 0x0E,
	Wireless = 0xE0,
	VendorSpecific = 0xFF
};

enum class InterfaceProtocol: uint8_t
{
	Keyboard = 1,
	Mouse = 2,
};

#pragma pack(push, 1)

struct DescriptorHeader
{
	uint8_t bLength;
	DescriptorType bDescriptorType;
	uint8_t bDescriptorSubtype;
};

/// section 9.6.1 of the USB 3.0 specificatio
struct DeviceDescriptor
{
	uint8_t  bLength;
	DescriptorType  bDescriptorType;
	uint16_t bcdUSB;
	uint8_t  bDeviceClass;
	uint8_t  bDeviceSubClass;
	uint8_t  bDeviceProtocol;
	uint8_t  bMaxPacketSize0;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t  iManufacturer;
	uint8_t  iProduct;
	uint8_t  iSerialNumber;
	uint8_t  bNumConfigurations;
};

struct ConfigurationDescriptor
{
	uint8_t  bLength;
	DescriptorType  bDescriptorType;
	uint16_t wTotalLength;
	uint8_t bNumInterfaces;
	uint8_t bConfiguration;
	uint8_t bmAttributes;
	uint8_t bMaxPower;
};

struct InterfaceDescriptor
{
	uint8_t bLength;
	DescriptorType bDescriptorType;
	uint8_t bInterfaceNumber;
	uint8_t bAlternateSetting;
	uint8_t bNumEndpoints;
	InterfaceClass bInterfaceClass; // 1: Audio
	uint8_t bInterfaceSubClass; // 1: Control, 2: Streaming
	InterfaceProtocol bInterfaceProtocol;
};

struct AudioStreamingGeneralDescriptor: DescriptorHeader
{
	uint8_t bTerminalLink;
	uint8_t bDelay;
	int16_t wFormatTag;
};

struct AudioStreamingFormatDescriptor: public DescriptorHeader
{
	uint8_t bFormatType;
	uint8_t bNrChannels;
	uint8_t bSubFrameSize;
	uint8_t bBitResolution;
	uint8_t bSampFreqType;
	uint24_t tSamFreq[];
};

struct EndPointAddress
{
	uint8_t value;

	bool IsInput() const
	{
		return (value & 0x80) != 0;
	}

	uint8_t Address() const
	{
		return value & 0x0F;
	}
};

// Section 9.6.6 of the USB 3.0 specification
struct EndPointDescriptor
{
	uint8_t bLength;
	DescriptorType bDescriptorType;
	EndPointAddress bEndpointAddress;	// 0x82: EP2_IN, 0x01: EP1_OUT
	uint8_t bmAttributes;
	int16_t wMaxPacketSize;
	uint8_t bInterval;
	uint8_t bRefresh;
	uint8_t bSynchAddress;
};
static_assert(sizeof(EndPointDescriptor) == 9);

#pragma pack(pop)

} // namespace Descriptors


struct AudioFormat
{
	uint8_t format;	// 1: PCM
	uint8_t bitResolution;
	std::vector<int> sampleRates;
};

struct EndPoint
{
	int interfaceNr, altsetting;
	Descriptors::InterfaceClass interfaceClass;
	Descriptors::EndPointAddress endpointAddress;
	int maxPacketSize;
	std::optional<AudioFormat> audioFormat;
};

/**
Minimal parsing of the binary descriptor data.

Assumes it is well-formatted and ordered.
*/
std::vector<EndPoint> ParseEndPoints(std::span<const std::byte> rawDescriptor);
