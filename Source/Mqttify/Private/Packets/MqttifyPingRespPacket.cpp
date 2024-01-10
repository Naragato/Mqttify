#include "Packets/MqttifyPingRespPacket.h"

namespace Mqttify
{

	void FMqttifyPingRespPacket::Encode(FMemoryWriter& InWriter)
	{
		InWriter.SetByteSwapping(true);
		FixedHeader.Encode(InWriter);
	}

	void FMqttifyPingRespPacket::Decode(FArrayReader& InReader)
	{
		InReader.SetByteSwapping(true);
		// Nothing to decode
	}
} // namespace Mqttify
