#pragma once

#include "Mqtt/MqttifyProtocolVersion.h"
#include "Mqtt/MqttifyReasonCode.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Packets/Interface/MqttifyControlPacket.h"
#include "Properties/MqttifyProperties.h"

namespace Mqttify
{
	struct FMqttifyPubRecPacketBase : TMqttifyControlPacket<EMqttifyPacketType::PubRec>
	{
	public:
		explicit FMqttifyPubRecPacketBase(const FMqttifyFixedHeader& InFixedHeader)
			: TMqttifyControlPacket{ InFixedHeader }
			, PacketIdentifier{ 0 } {}

		explicit FMqttifyPubRecPacketBase(const uint16 InPacketIdentifier)
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
	struct TMqttifyPubRecPacket;

	template <>
	struct TMqttifyPubRecPacket<EMqttifyProtocolVersion::Mqtt_3_1_1> final : FMqttifyPubRecPacketBase
	{
		explicit TMqttifyPubRecPacket(const uint16 InPacketIdentifier)
			: FMqttifyPubRecPacketBase{ InPacketIdentifier }
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		explicit TMqttifyPubRecPacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifyPubRecPacketBase{ InFixedHeader }
		{
			Decode(InReader);
		}

		uint32 GetLength() const override { return sizeof(PacketIdentifier); }
		void Encode(FMemoryWriter& InWriter) override;
		void Decode(FArrayReader& InReader) override;
	};

	template <>
	struct TMqttifyPubRecPacket<
			EMqttifyProtocolVersion::Mqtt_5> final : FMqttifyPubRecPacketBase
	{
	public:
		explicit TMqttifyPubRecPacket(const uint16 InPacketIdentifier,
									const EMqttifyReasonCode InReasonCode  = EMqttifyReasonCode::Success,
									const FMqttifyProperties& InProperties = FMqttifyProperties{})
			: FMqttifyPubRecPacketBase{ InPacketIdentifier }
			, ReasonCode{ InReasonCode }
			, Properties{ InProperties }
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		explicit TMqttifyPubRecPacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifyPubRecPacketBase{ InFixedHeader }
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

	using FMqttifyPubRecPacket3 = TMqttifyPubRecPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>;
	using FMqttifyPubRecPacket5 = TMqttifyPubRecPacket<EMqttifyProtocolVersion::Mqtt_5>;
} // namespace Mqttify
