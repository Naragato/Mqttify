#pragma once

#include "Mqtt/MqttifyProtocolVersion.h"
#include "Mqtt/MqttifyReasonCode.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Packets/Interface/MqttifyControlPacket.h"
#include "Properties/MqttifyProperties.h"

namespace Mqttify
{
	struct FMqttifyUnsubAckPacketBase : TMqttifyControlPacket<EMqttifyPacketType::UnsubAck>
	{
	public:
		explicit FMqttifyUnsubAckPacketBase(const FMqttifyFixedHeader& InFixedHeader)
			: TMqttifyControlPacket{ InFixedHeader }
			, PacketIdentifier{ 0 } {}

		explicit FMqttifyUnsubAckPacketBase(const uint16 InPacketIdentifier)
			: PacketIdentifier{ InPacketIdentifier } {}

	public:
		/**
		 * @brief Get the packet identifier.
		 * @return The packet identifier.
		 */
		uint16 GetPacketIdentifier() const { return PacketIdentifier; }

	protected:
		uint16 PacketIdentifier;
	};

	template <EMqttifyProtocolVersion InProtocolVersion>
	struct TMqttifyUnsubAckPacket;

	template <>
	struct TMqttifyUnsubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1> final : FMqttifyUnsubAckPacketBase
	{
		explicit TMqttifyUnsubAckPacket(const uint16 InPacketIdentifier)
			: FMqttifyUnsubAckPacketBase{ InPacketIdentifier }
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		explicit TMqttifyUnsubAckPacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifyUnsubAckPacketBase{ InFixedHeader }
		{
			Decode(InReader);
		}

		uint32 GetLength() const override;
		void Encode(FMemoryWriter& InWriter) override;
		void Decode(FArrayReader& InReader) override;
	};

	template <>
	struct TMqttifyUnsubAckPacket<
			EMqttifyProtocolVersion::Mqtt_5> final : FMqttifyUnsubAckPacketBase
	{
	public:
		explicit TMqttifyUnsubAckPacket(const uint16 InPacketIdentifier,
										const TArray<EMqttifyReasonCode>& InReasonCodes,
										const FMqttifyProperties& InProperties = FMqttifyProperties{})
			: FMqttifyUnsubAckPacketBase{ InPacketIdentifier }
			, ReasonCodes{ InReasonCodes }
			, Properties{ InProperties }
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		explicit TMqttifyUnsubAckPacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifyUnsubAckPacketBase{ InFixedHeader }
		{
			Decode(InReader);
		}

		uint32 GetLength() const override;

		void Encode(FMemoryWriter& InWriter) override;
		void Decode(FArrayReader& InReader) override;

		/**
		 * @brief Get the reason codes for each topic filter.
		 * Ordered by the order of the topic filters in the
		 * subscribe packet.
		 * @return The reason codes for each topic filter.
		 */
		const TArray<EMqttifyReasonCode>& GetReasonCodes() const { return ReasonCodes; }

		/**
		 * @brief Get the properties.
		 * @return The properties.
		 */
		const FMqttifyProperties& GetProperties() const { return Properties; }

	private:
		TArray<EMqttifyReasonCode> ReasonCodes;
		FMqttifyProperties Properties;
	};

	using FMqttifyUnsubAckPacket3 = TMqttifyUnsubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>;
	using FMqttifyUnsubAckPacket5 = TMqttifyUnsubAckPacket<EMqttifyProtocolVersion::Mqtt_5>;
} // namespace Mqttify
