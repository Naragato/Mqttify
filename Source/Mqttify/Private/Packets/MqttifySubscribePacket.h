#pragma once

#include "Mqtt/MqttifyProtocolVersion.h"
#include "Mqtt/MqttifyQualityOfService.h"
#include "Mqtt/MqttifyTopicFilter.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Packets/Interface/MqttifyControlPacket.h"
#include "Properties/MqttifyProperties.h"


namespace Mqttify
{
	struct FMqttifySubscribePacketBase : TMqttifyControlPacket<EMqttifyPacketType::Subscribe>
	{
	public:
		explicit FMqttifySubscribePacketBase(const FMqttifyFixedHeader& InFixedHeader)
			: TMqttifyControlPacket{ InFixedHeader }
			, PacketIdentifier{} {}

		explicit FMqttifySubscribePacketBase(const TArray<FMqttifyTopicFilter>& InTopicFilters,
											const uint16 InPacketIdentifier)
			: PacketIdentifier{ InPacketIdentifier }
			, TopicFilters{ InTopicFilters } {}

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
	struct TMqttifySubscribePacket;

	template <>
	struct TMqttifySubscribePacket<EMqttifyProtocolVersion::Mqtt_3_1_1> final : FMqttifySubscribePacketBase
	{
		explicit TMqttifySubscribePacket(const TArray<FMqttifyTopicFilter>& InTopicFilters,
										const uint16 InPacketIdentifier)
			: FMqttifySubscribePacketBase{ InTopicFilters, InPacketIdentifier }
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		explicit TMqttifySubscribePacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifySubscribePacketBase{ InFixedHeader }
		{
			Decode(InReader);
		}

		virtual uint32 GetLength() const override;
		virtual void Encode(FMemoryWriter& InWriter) override;
		virtual void Decode(FArrayReader& InReader) override;
	};

	template <>
	struct TMqttifySubscribePacket<EMqttifyProtocolVersion::Mqtt_5> final : FMqttifySubscribePacketBase
	{
	public:
		explicit TMqttifySubscribePacket(const TArray<FMqttifyTopicFilter>& InTopicFilters,
										const uint16 InPacketIdentifier,
										const FMqttifyProperties& InProperties = FMqttifyProperties{})
			: FMqttifySubscribePacketBase{ InTopicFilters, InPacketIdentifier }
			, Properties{ InProperties }
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		explicit TMqttifySubscribePacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifySubscribePacketBase{ InFixedHeader }
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

		static constexpr TCHAR InvalidSubscribeOptions[] = TEXT("Invalid Subscribe Options.");

	protected:
		FMqttifyProperties Properties;
	};

	using FMqttifySubscribePacket3 = TMqttifySubscribePacket<EMqttifyProtocolVersion::Mqtt_3_1_1>;
	using FMqttifySubscribePacket5 = TMqttifySubscribePacket<EMqttifyProtocolVersion::Mqtt_5>;
} // namespace Mqttify
