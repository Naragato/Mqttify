#pragma once

#include "Mqtt/MqttifyProtocolVersion.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Packets/Interface/MqttifyControlPacket.h"

namespace Mqttify
{
	struct FMqttifyPingReqPacket final : TMqttifyControlPacket<EMqttifyPacketType::PingReq>
	{
	public:
		explicit FMqttifyPingReqPacket(const FMqttifyFixedHeader& InFixedHeader)
			: TMqttifyControlPacket{ InFixedHeader } {}

		explicit FMqttifyPingReqPacket()
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		void Encode(FMemoryWriter& InWriter) override;
		void Decode(FArrayReader& InReader) override;
		uint32 GetLength() const override { return 0; }
	};
} // namespace Mqttify
