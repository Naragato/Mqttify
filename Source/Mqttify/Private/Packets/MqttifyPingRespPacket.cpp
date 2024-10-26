#include "Packets/MqttifyPingRespPacket.h"

namespace Mqttify
{

	void FMqttifyPingRespPacket::Encode(FMemoryWriter& InWriter)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("[Encode][PingResp]"));
		InWriter.SetByteSwapping(true);
		FixedHeader.Encode(InWriter);
	}

	void FMqttifyPingRespPacket::Decode(FArrayReader& InReader)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("[Decode][PingResp]"));
		InReader.SetByteSwapping(true);
		// Nothing to decode
	}
} // namespace Mqttify
