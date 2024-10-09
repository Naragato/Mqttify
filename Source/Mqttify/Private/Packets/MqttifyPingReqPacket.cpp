#include "Packets/MqttifyPingReqPacket.h"

namespace Mqttify
{
	void FMqttifyPingReqPacket::Encode(FMemoryWriter& InWriter)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("[Encode][PingReq]"));
		InWriter.SetByteSwapping(true);
		FixedHeader.Encode(InWriter);
	}

	void FMqttifyPingReqPacket::Decode(FArrayReader& InReader)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("[Decode][PingReq]"));
		InReader.SetByteSwapping(true);
		// Nothing to decode
	}
} // namespace Mqttify
