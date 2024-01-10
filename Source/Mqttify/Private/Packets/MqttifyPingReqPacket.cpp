#include "Packets/MqttifyPingReqPacket.h"

namespace Mqttify
{

	void FMqttifyPingReqPacket::Encode(FMemoryWriter& InWriter)
	{
		InWriter.SetByteSwapping(true);
		FixedHeader.Encode(InWriter);
	}

	void FMqttifyPingReqPacket::Decode(FArrayReader& InReader)
	{
		InReader.SetByteSwapping(true);
		// Nothing to decode
	}
} // namespace Mqttify
