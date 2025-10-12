#pragma once

#include "Mqtt/MqttifyProtocolVersion.h"
#include "Mqtt/MqttifyReasonCode.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Packets/Interface/MqttifyControlPacket.h"
#include "Properties/MqttifyProperties.h"

namespace Mqttify
{
	struct FMqttifyPubRelPacketBase : TMqttifyControlPacket<EMqttifyPacketType::PubRel>
	{
	public:
		explicit FMqttifyPubRelPacketBase(const FMqttifyFixedHeader& InFixedHeader)
			: TMqttifyControlPacket{InFixedHeader}
			, PacketIdentifier{0} {}

		explicit FMqttifyPubRelPacketBase(const uint16 InPacketIdentifier)
			: PacketIdentifier{InPacketIdentifier} {}

	public:
		/**
		 * @brief Get the packet identifier.
		 * @return The packet identifier.
		 */
		virtual uint16 GetPacketId() const override { return PacketIdentifier; }

	protected:
		uint16 PacketIdentifier;
	};

	template <EMqttifyProtocolVersion InProtocolVersion>
	struct TMqttifyPubRelPacket;

	template <>
	struct TMqttifyPubRelPacket<EMqttifyProtocolVersion::Mqtt_3_1_1> final : FMqttifyPubRelPacketBase
	{
		explicit TMqttifyPubRelPacket(const uint16 InPacketIdentifier)
			: FMqttifyPubRelPacketBase{InPacketIdentifier}
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		explicit TMqttifyPubRelPacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifyPubRelPacketBase{InFixedHeader}
		{
			Decode(InReader);
		}

		virtual uint32 GetLength() const override { return sizeof(PacketIdentifier); }
		virtual void Encode(FMemoryWriter& InWriter) override;
		virtual void Decode(FArrayReader& InReader) override;
	};

	template <>
	struct TMqttifyPubRelPacket<EMqttifyProtocolVersion::Mqtt_5> final : FMqttifyPubRelPacketBase
	{
	public:
		explicit TMqttifyPubRelPacket(const uint16 InPacketIdentifier,
		                              const EMqttifyReasonCode InReasonCode = EMqttifyReasonCode::Success,
		                              const FMqttifyProperties& InProperties = FMqttifyProperties{})
			: FMqttifyPubRelPacketBase{InPacketIdentifier}
			, ReasonCode{InReasonCode}
			, Properties{InProperties}
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		explicit TMqttifyPubRelPacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifyPubRelPacketBase{InFixedHeader}
			, ReasonCode{EMqttifyReasonCode::Success}
		{
			Decode(InReader);
		}

		virtual uint32 GetLength() const override;

		virtual void Encode(FMemoryWriter& InWriter) override;
		virtual void Decode(FArrayReader& InReader) override;

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

	using FMqttifyPubRelPacket3 = TMqttifyPubRelPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>;
	using FMqttifyPubRelPacket5 = TMqttifyPubRelPacket<EMqttifyProtocolVersion::Mqtt_5>;
} // namespace Mqttify
