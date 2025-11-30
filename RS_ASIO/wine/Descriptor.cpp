#include "Descriptor.h"

#include <cassert>

#include <endian.h>


using namespace Descriptors;


template<typename T> struct Type;
template<> struct Type<DeviceDescriptor>
{	static constexpr DescriptorType type = DescriptorType::Device; };
template<> struct Type<ConfigurationDescriptor>
{	static constexpr DescriptorType type = DescriptorType::Configuration; };
template<> struct Type<InterfaceDescriptor>
{	static constexpr DescriptorType type = DescriptorType::Interface; };
template<> struct Type<EndPointDescriptor>
{	static constexpr DescriptorType type = DescriptorType::EndPoint; };
template<> struct Type<AudioStreamingGeneralDescriptor>
{	static constexpr DescriptorType type = DescriptorType::AudioStreaming; };
template<> struct Type<AudioStreamingFormatDescriptor>
{	static constexpr DescriptorType type = DescriptorType::AudioStreaming; };


std::uint16_t toHost(int16_t v)
{
	return le16toh(v);
}

std::uint32_t toHost(uint24_t v)
{
	return v.val[0] | (v.val[1]<<8) | (v.val[2] << 16);
}


template<typename T>
T& ensure(std::optional<T>& opt)
{
	if(!opt.has_value())
		opt = T{};
	return opt.value();
}


template<typename Descriptor>
const Descriptor* ConsumeDescriptor(std::span<const std::byte>& buf)
{
	assert(buf.size() >= 2);	// Need lengt & type fields

	auto pDesc = reinterpret_cast<const Descriptor*>(buf.data());
	buf = buf.subspan(pDesc->bLength);
	while(!buf.empty() && Type<Descriptor>::type != pDesc->bDescriptorType)
	{
		pDesc = reinterpret_cast<const Descriptor*>(buf.data());
		buf = buf.subspan(pDesc->bLength);
	}

	if(Type<Descriptor>::type != pDesc->bDescriptorType)
		return nullptr;
	return pDesc;
}


void ConsumeAudioDescriptor(std::span<const std::byte>& buf, std::optional<AudioFormat>& fmt, const InterfaceDescriptor& itf)
{
	assert(itf.bInterfaceClass == InterfaceClass::Audio);
	auto pHdr = reinterpret_cast<const DescriptorHeader*>(buf.data());
	if(itf.bInterfaceSubClass == 2) // Streaming
	{
		switch(pHdr->bDescriptorSubtype)
		{
			case 1: // AS_GENERAL
				{
					auto pAsGeneral = ConsumeDescriptor<AudioStreamingGeneralDescriptor>(buf);
					// https://stackoverflow.com/q/47229082
					ensure(fmt).format = toHost(pAsGeneral->wFormatTag);
				}
				break;
			case 2: // FORMAT_TYPE
				{
					auto pAudioStream = ConsumeDescriptor<AudioStreamingFormatDescriptor>(buf);
					// https://stackoverflow.com/q/47229082
					ensure(fmt).bitResolution = pAudioStream->bBitResolution;
					for(int i=0; i < pAudioStream->bSampFreqType; i++)
					{
						fmt->sampleRates.push_back(toHost(pAudioStream->tSamFreq[i]));
					}
				}
				break;
			default:
				buf = buf.subspan(pHdr->bLength);
		}
	}
	else if(itf.bInterfaceSubClass == 1) // Control
	{
		buf = buf.subspan(pHdr->bLength);
	}
	else
	{
		buf = buf.subspan(pHdr->bLength);
	}
}


std::vector<EndPoint> ParseEndPoints(std::span<const std::byte> rawDescriptor)
{
	auto pDev = ConsumeDescriptor<DeviceDescriptor>(rawDescriptor);

	std::vector<EndPoint> rv;
	for(int i = 0; i < pDev->bNumConfigurations; i++)
	{
		auto pConfig = ConsumeDescriptor<ConfigurationDescriptor>(rawDescriptor);
		std::optional<AudioFormat> audio_format;
		for(int j = 0; j < pConfig->bNumInterfaces; j++)
		{
			auto pItf = ConsumeDescriptor<InterfaceDescriptor>(rawDescriptor);
			auto pNext = reinterpret_cast<const DescriptorHeader*>(rawDescriptor.data());
			switch(pNext->bDescriptorType)
			{
			case DescriptorType::AudioStreaming:
				ConsumeAudioDescriptor(rawDescriptor, audio_format, *pItf);
				break;
			case DescriptorType::EndPoint:
				for(int k = 0; k < pItf->bNumEndpoints; k++)
				{
					auto pEnd = ConsumeDescriptor<EndPointDescriptor>(rawDescriptor);
					EndPoint ep;
					ep.interfaceNr = pItf->bInterfaceNumber;
					ep.altsetting = pItf->bAlternateSetting;
					ep.interfaceClass = pItf->bInterfaceClass;
					ep.endpointAddress = pEnd->bEndpointAddress;
					ep.maxPacketSize = toHost(pEnd->wMaxPacketSize);
					ep.audioFormat = audio_format;
					rv.push_back(ep);
				} // endpoints
				break;
			default:
				rawDescriptor = rawDescriptor.subspan(pNext->bLength);
			}
		} // interfaces
	} // configurations
	return rv;
}
