#pragma once

#include "Mqtt/MqttifyProtocolVersion.h"
#include "Mqtt/MqttifyQualityOfService.h"
#include "Mqtt/MqttifyTopicFilter.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Packets/Interface/MqttifyControlPacket.h"
#include "Properties/MqttifyProperties.h"

namespace Mqttify
{
	struct FMqttifyUnsubscribePacketBase : TMqttifyControlPacket<EMqttifyPacketType::Unsubscribe>
	{
	public:
		explicit FMqttifyUnsubscribePacketBase(const FMqttifyFixedHeader& InFixedHeader)
			: TMqttifyControlPacket{InFixedHeader}
			, PacketIdentifier{} {}

		explicit FMqttifyUnsubscribePacketBase(const TArray<FMqttifyTopicFilter>& InTopicFilters,
		                                       const uint16 InPacketIdentifier)
			: PacketIdentifier{InPacketIdentifier}
			, TopicFilters{InTopicFilters} {}

	public:
		/**
		 * @brief Get the is duplicate flag.
		 * @return The is duplicate flag.
		 */
		bool GetIsDuplicate() const { return (FixedHeader.GetFlags() & 0x08) == 0x08; }

		/**
		 * @brief Get the should retain flag.
		 * @return The should retain flag.
		 */
		bool GetShouldRetain() const { return (FixedHeader.GetFlags() & 0x01) == 0x01; }

		/**
		 * @brief Get the quality of service.
		 * @return The quality of service.
		 */
		EMqttifyQualityOfService GetQualityOfService() const
		{
			return static_cast<EMqttifyQualityOfService>((FixedHeader.GetFlags() & 0x06) >> 1);
		}

		/**
		 * @brief Get the topic name.
		 * @return The topic name.
		 */
		const TArray<FMqttifyTopicFilter>& GetTopicFilters() const { return TopicFilters; }

		/**
		 * @brief Get the packet identifier.
		 * @return The packet identifier.
		 */
		virtual uint16 GetPacketId() const override { return PacketIdentifier; }

	protected:
		uint16 PacketIdentifier;
		TArray<FMqttifyTopicFilter> TopicFilters;
	};

	template <EMqttifyProtocolVersion InProtocolVersion>
	struct TMqttifyUnsubscribePacket;

	template <>
	struct TMqttifyUnsubscribePacket<EMqttifyProtocolVersion::Mqtt_3_1_1> final : FMqttifyUnsubscribePacketBase
	{
		explicit TMqttifyUnsubscribePacket(const TArray<FMqttifyTopicFilter>& InTopicFilters,
		                                   const uint16 InPacketIdentifier)
			: FMqttifyUnsubscribePacketBase{InTopicFilters, InPacketIdentifier}
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		explicit TMqttifyUnsubscribePacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifyUnsubscribePacketBase{InFixedHeader}
		{
			Decode(InReader);
		}

		virtual uint32 GetLength() const override;
		virtual void Encode(FMemoryWriter& InWriter) override;
		virtual void Decode(FArrayReader& InReader) override;
	};

	template <>
	struct TMqttifyUnsubscribePacket<EMqttifyProtocolVersion::Mqtt_5> final : FMqttifyUnsubscribePacketBase
	{
	public:
		explicit TMqttifyUnsubscribePacket(const TArray<FMqttifyTopicFilter>& InTopicFilters,
		                                   const uint16 InPacketIdentifier,
		                                   const FMqttifyProperties& InProperties = FMqttifyProperties{})
			: FMqttifyUnsubscribePacketBase{InTopicFilters, InPacketIdentifier}
			, Properties{InProperties}
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		explicit TMqttifyUnsubscribePacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifyUnsubscribePacketBase{InFixedHeader}
		{
			Decode(InReader);
		}

		virtual uint32 GetLength() const override;

		virtual void Encode(FMemoryWriter& InWriter) override;
		virtual void Decode(FArrayReader& InReader) override;

		/**
		 * @brief Get the properties.
		 * @return The properties.
		 */
		const FMqttifyProperties& GetProperties() const { return Properties; }

	protected:
		FMqttifyProperties Properties;
	};

	using FMqttifyUnsubscribePacket3 = TMqttifyUnsubscribePacket<EMqttifyProtocolVersion::Mqtt_3_1_1>;
	using FMqttifyUnsubscribePacket5 = TMqttifyUnsubscribePacket<EMqttifyProtocolVersion::Mqtt_5>;
} // namespace Mqttify
