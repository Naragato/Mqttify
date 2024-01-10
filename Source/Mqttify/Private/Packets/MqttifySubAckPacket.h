#pragma once

#include "Mqtt/MqttifyProtocolVersion.h"
#include "Mqtt/MqttifyReasonCode.h"
#include "Mqtt/MqttifySubscribeReturnCode.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Packets/Interface/MqttifyControlPacket.h"
#include "Properties/MqttifyProperties.h"

namespace Mqttify
{
	struct FMqttifySubAckPacketBase : TMqttifyControlPacket<EMqttifyPacketType::SubAck>
	{
	public:
		explicit FMqttifySubAckPacketBase(const FMqttifyFixedHeader& InFixedHeader)
			: TMqttifyControlPacket{ InFixedHeader }
			, PacketIdentifier{ 0 } {}

		explicit FMqttifySubAckPacketBase(const uint16 InPacketIdentifier)
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
	struct TMqttifySubAckPacket;

	template <>
	struct TMqttifySubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1> final : FMqttifySubAckPacketBase
	{
		explicit TMqttifySubAckPacket(const uint16 InPacketIdentifier,
									const TArray<EMqttifySubscribeReturnCode>& InReturnCodes)
			: FMqttifySubAckPacketBase{ InPacketIdentifier }
			, ReturnCodes{ InReturnCodes }
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		explicit TMqttifySubAckPacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifySubAckPacketBase{ InFixedHeader }
		{
			Decode(InReader);
		}

		uint32 GetLength() const override;
		void Encode(FMemoryWriter& InWriter) override;
		void Decode(FArrayReader& InReader) override;

		/**
		 * @brief Get the return codes for each topic filter.
		 * @return Return codes for each topic filter.
		 */
		TArray<EMqttifySubscribeReturnCode> GetReturnCodes() const { return ReturnCodes; }

	private:
		TArray<EMqttifySubscribeReturnCode> ReturnCodes;
	};

	template <>
	struct TMqttifySubAckPacket<
			EMqttifyProtocolVersion::Mqtt_5> final : FMqttifySubAckPacketBase
	{
	public:
		explicit TMqttifySubAckPacket(const uint16 InPacketIdentifier,
									const TArray<EMqttifyReasonCode>& InReasonCodes,
									const FMqttifyProperties& InProperties = FMqttifyProperties{})
			: FMqttifySubAckPacketBase{ InPacketIdentifier }
			, ReasonCodes{ InReasonCodes }
			, Properties{ InProperties }
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		explicit TMqttifySubAckPacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifySubAckPacketBase{ InFixedHeader }
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

	using FMqttifySubAckPacket3 = TMqttifySubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>;
	using FMqttifySubAckPacket5 = TMqttifySubAckPacket<EMqttifyProtocolVersion::Mqtt_5>;
} // namespace Mqttify
