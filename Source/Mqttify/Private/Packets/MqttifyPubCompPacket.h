#pragma once

#include "Mqtt/MqttifyProtocolVersion.h"
#include "Mqtt/MqttifyReasonCode.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Packets/Interface/MqttifyControlPacket.h"
#include "Properties/MqttifyProperties.h"

namespace Mqttify
{
	struct FMqttifyPubCompPacketBase : TMqttifyControlPacket<EMqttifyPacketType::PubComp>
	{
	public:
		explicit FMqttifyPubCompPacketBase(const FMqttifyFixedHeader& InFixedHeader)
			: TMqttifyControlPacket{InFixedHeader}
			, PacketIdentifier{0} {}

		explicit FMqttifyPubCompPacketBase(const uint16 InPacketIdentifier)
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
	struct TMqttifyPubCompPacket;

	template <>
	struct TMqttifyPubCompPacket<EMqttifyProtocolVersion::Mqtt_3_1_1> final : FMqttifyPubCompPacketBase
	{
		explicit TMqttifyPubCompPacket(const uint16 InPacketIdentifier)
			: FMqttifyPubCompPacketBase{InPacketIdentifier}
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		explicit TMqttifyPubCompPacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifyPubCompPacketBase{InFixedHeader}
		{
			Decode(InReader);
		}

		virtual uint32 GetLength() const override { return sizeof(PacketIdentifier); }
		virtual void Encode(FMemoryWriter& InWriter) override;
		virtual void Decode(FArrayReader& InReader) override;
	};

	template <>
	struct TMqttifyPubCompPacket<EMqttifyProtocolVersion::Mqtt_5> final : FMqttifyPubCompPacketBase
	{
	public:
		explicit TMqttifyPubCompPacket(const uint16 InPacketIdentifier,
		                               const EMqttifyReasonCode InReasonCode,
		                               const FMqttifyProperties& InProperties = FMqttifyProperties{})
			: FMqttifyPubCompPacketBase{InPacketIdentifier}
			, ReasonCode{InReasonCode}
			, Properties{InProperties}
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		explicit TMqttifyPubCompPacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifyPubCompPacketBase{InFixedHeader}
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

	using FMqttifyPubCompPacket3 = TMqttifyPubCompPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>;
	using FMqttifyPubCompPacket5 = TMqttifyPubCompPacket<EMqttifyProtocolVersion::Mqtt_5>;
} // namespace Mqttify
