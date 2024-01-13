#pragma once

#include "Mqtt/MqttifyProtocolVersion.h"
#include "Mqtt/MqttifyReasonCode.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Packets/Interface/MqttifyControlPacket.h"
#include "Properties/MqttifyProperties.h"

namespace Mqttify
{
	struct FMqttifyPubAckPacketBase : TMqttifyControlPacket<EMqttifyPacketType::PubAck>
	{
	public:
		explicit FMqttifyPubAckPacketBase(const FMqttifyFixedHeader& InFixedHeader)
			: TMqttifyControlPacket{ InFixedHeader }
			, PacketIdentifier{ 0 } {}

		explicit FMqttifyPubAckPacketBase(const uint16 InPacketIdentifier)
			: PacketIdentifier{ InPacketIdentifier } {}

	public:
		/**
		 * @brief Get the packet identifier.
		 * @return The packet identifier.
		 */
		uint16 GetPacketId() const override { return PacketIdentifier; }

	protected:
		uint16 PacketIdentifier;
	};

	template <EMqttifyProtocolVersion InProtocolVersion>
	struct TMqttifyPubAckPacket;

	template <>
	struct TMqttifyPubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1> final : FMqttifyPubAckPacketBase
	{
		explicit TMqttifyPubAckPacket(const uint16 InPacketIdentifier)
			: FMqttifyPubAckPacketBase{ InPacketIdentifier }
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		explicit TMqttifyPubAckPacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifyPubAckPacketBase{ InFixedHeader }
		{
			Decode(InReader);
		}

		uint32 GetLength() const override { return 2; }
		void Encode(FMemoryWriter& InWriter) override;
		void Decode(FArrayReader& InReader) override;
	};

	template <>
	struct TMqttifyPubAckPacket<
			EMqttifyProtocolVersion::Mqtt_5> final : FMqttifyPubAckPacketBase
	{
	public:
		explicit TMqttifyPubAckPacket(const uint16 InPacketIdentifier,
									const EMqttifyReasonCode InReasonCode  = EMqttifyReasonCode::Success,
									const FMqttifyProperties& InProperties = FMqttifyProperties{})
			: FMqttifyPubAckPacketBase{ InPacketIdentifier }
			, ReasonCode{ InReasonCode }
			, Properties{ InProperties }
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		explicit TMqttifyPubAckPacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifyPubAckPacketBase{ InFixedHeader }
			, ReasonCode{ EMqttifyReasonCode::Success }
		{
			Decode(InReader);
		}

		uint32 GetLength() const override;

		void Encode(FMemoryWriter& InWriter) override;
		void Decode(FArrayReader& InReader) override;

		/**
		 * @brief Get the reason code.
		 * @return The reason code.
		 */
		EMqttifyReasonCode GetReasonCode() const { return ReasonCode; }

		/**
		 * @brief Get the properties.
		 * @return The properties.
		 */
		const FMqttifyProperties& GetProperties() const { return Properties; }

	protected:
		EMqttifyReasonCode ReasonCode;
		FMqttifyProperties Properties;
	};

	using FMqttifyPubAckPacket3 = TMqttifyPubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>;
	using FMqttifyPubAckPacket5 = TMqttifyPubAckPacket<EMqttifyProtocolVersion::Mqtt_5>;
} // namespace Mqttify
