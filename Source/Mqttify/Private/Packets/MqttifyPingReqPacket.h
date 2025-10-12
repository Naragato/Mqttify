#pragma once

#include "Packets/MqttifyFixedHeader.h"
#include "Packets/Interface/MqttifyControlPacket.h"

namespace Mqttify
{
	struct FMqttifyPingReqPacket final : TMqttifyControlPacket<EMqttifyPacketType::PingReq>
	{
	public:
		explicit FMqttifyPingReqPacket(const FMqttifyFixedHeader& InFixedHeader)
			: TMqttifyControlPacket{InFixedHeader} {}

		explicit FMqttifyPingReqPacket()
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		virtual void Encode(FMemoryWriter& InWriter) override;
		virtual void Decode(FArrayReader& InReader) override;
		virtual uint32 GetLength() const override { return 0; }
	};
} // namespace Mqttify
