#pragma once

#include "Mqtt/MqttifyProtocolVersion.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Packets/Interface/MqttifyControlPacket.h"

namespace Mqttify
{
	struct FMqttifyPingRespPacket final : TMqttifyControlPacket<EMqttifyPacketType::PingResp>
	{
	public:
		explicit FMqttifyPingRespPacket(const FMqttifyFixedHeader& InFixedHeader)
			: TMqttifyControlPacket{ InFixedHeader } {}

		explicit FMqttifyPingRespPacket()
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		void Encode(FMemoryWriter& InWriter) override;
		void Decode(FArrayReader& InReader) override;
		uint32 GetLength() const override { return 0; }
	};
} // namespace Mqttify
