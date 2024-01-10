#pragma once

#include "Mqtt/MqttifyMessage.h"
#include "Mqtt/MqttifyProtocolVersion.h"
#include "Mqtt/MqttifyQualityOfService.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Packets/Interface/MqttifyControlPacket.h"
#include "Properties/MqttifyProperties.h"

namespace Mqttify
{
	struct FMqttifyPublishPacketBase : TMqttifyControlPacket<EMqttifyPacketType::Publish>
	{
	public:
		explicit FMqttifyPublishPacketBase(const FMqttifyFixedHeader& InFixedHeader)
			: TMqttifyControlPacket{InFixedHeader}, PacketIdentifier{}
		{
		}

		explicit FMqttifyPublishPacketBase(
			TArray<uint8>&& InPayload,
			FString&& InTopicName,
			const uint16 InPacketIdentifier)
			: TopicName{MoveTemp(InTopicName)}, PacketIdentifier{InPacketIdentifier}, Payload{MoveTemp(InPayload)}
		{
		}

	public:
		/**
		 * @brief Get the is duplicate flag.
		 * @return The is duplicate flag.
		 */
		bool GetIsDuplicate() const
		{
			return GetQualityOfService() != EMqttifyQualityOfService::AtMostOnce
				&& (FixedHeader.GetFlags() & 0x08) == 0x08;
		}

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
		const FString& GetTopicName() const { return TopicName; }

		/**
		 * @brief Get the packet identifier.
		 * @return The packet identifier.
		 */
		uint16 GetPacketIdentifier() const { return PacketIdentifier; }

		/**
		 * @brief Get the payload.
		 * @return The payload.
		 */
		const TArray<uint8>& GetPayload() const { return Payload; }

		static FMqttifyMessage ToMqttifyMessage(TUniquePtr<FMqttifyPublishPacketBase>&& InPacket)
		{
			return FMqttifyMessage{
				MoveTemp(InPacket->TopicName),
				MoveTemp(InPacket->Payload),
				InPacket->GetShouldRetain(),
				InPacket->GetQualityOfService()
			};
		}

	protected:
		FString TopicName;
		uint16 PacketIdentifier;
		TArray<uint8> Payload;

		// The protected token allows only members or friends to call MakeShared.
		struct FPrivateToken
		{
			explicit FPrivateToken() = default;
		};
	};

	template <EMqttifyProtocolVersion InProtocolVersion>
	struct TMqttifyPublishPacket;

	template <>
	struct TMqttifyPublishPacket<EMqttifyProtocolVersion::Mqtt_3_1_1> final : FMqttifyPublishPacketBase
	{
	public:
		explicit TMqttifyPublishPacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifyPublishPacketBase{InFixedHeader}
		{
			Decode(InReader);
		}

		explicit TMqttifyPublishPacket(FMqttifyMessage&& InMessage, const uint16 InPacketIdentifier)
			: FMqttifyPublishPacketBase{MoveTemp(InMessage.Payload), MoveTemp(InMessage.Topic), InPacketIdentifier}
		{
			const uint32 PayloadSize = GetLength(
				GetTopicName().Len(),
				GetPayload().Num(),
				InMessage.GetQualityOfService());

			FixedHeader = FMqttifyFixedHeader::Create(
				this,
				PayloadSize,
				InMessage.GetIsRetained(),
				InMessage.GetQualityOfService(),
				false);
		}

		static uint32 GetLength(
			uint32 InTopicNameLength,
			uint32 InPayloadLength,
			EMqttifyQualityOfService InQualityOfService);
		uint32 GetLength() const override;
		void Encode(FMemoryWriter& InWriter) override;
		void Decode(FArrayReader& InReader) override;

		/**
		 * @brief Get a duplicate of the packet. Used when QoS > 0
		 * and we need to resend the packet.
		 */
		static TSharedRef<TMqttifyPublishPacket> GetDuplicate(TSharedRef<TMqttifyPublishPacket>&& InPublishPacket)
		{
			return MakeShareable<TMqttifyPublishPacket>(
				new TMqttifyPublishPacket{
					FPrivateToken{},
					MoveTemp(InPublishPacket->Payload),
					MoveTemp(InPublishPacket->TopicName),
					InPublishPacket->PacketIdentifier,
					InPublishPacket->GetQualityOfService(),
					InPublishPacket->GetShouldRetain(),
					true
				});
		}

	private:
		explicit TMqttifyPublishPacket(
			FPrivateToken,
			TArray<uint8>&& InPayload,
			FString&& InTopicName,
			const uint16 InPacketIdentifier,
			const EMqttifyQualityOfService InQualityOfService,
			const bool bInShouldRetain,
			const bool bInIsDuplicate)
			: FMqttifyPublishPacketBase{MoveTemp(InPayload), MoveTemp(InTopicName), InPacketIdentifier}
		{
			const uint32 PayloadSize = GetLength(TopicName.Len(), Payload.Num(), InQualityOfService);
			FixedHeader = FMqttifyFixedHeader::Create(
				this,
				PayloadSize,
				bInShouldRetain,
				InQualityOfService,
				bInIsDuplicate);
		}
	};

	template <>
	struct TMqttifyPublishPacket<EMqttifyProtocolVersion::Mqtt_5> final : FMqttifyPublishPacketBase
	{
	public:
		explicit TMqttifyPublishPacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifyPublishPacketBase{InFixedHeader}
		{
			Decode(InReader);
		}

		explicit TMqttifyPublishPacket(
			FMqttifyMessage&& InMessage,
			const uint16 InPacketIdentifier,
			const FMqttifyProperties& InProperties = FMqttifyProperties{})
			: FMqttifyPublishPacketBase{MoveTemp(InMessage.Payload), MoveTemp(InMessage.Topic), InPacketIdentifier},
			Properties{InProperties}
		{
			const uint32 PayloadSize = GetLength(
				GetTopicName().Len(),
				GetPayload().Num(),
				InProperties.GetLength(),
				InMessage.GetQualityOfService());
			FixedHeader = FMqttifyFixedHeader::Create(
				this,
				PayloadSize,
				InMessage.GetIsRetained(),
				InMessage.GetQualityOfService(),
				false);
		}

		static uint32 GetLength(
			uint32 InTopicNameLength,
			uint32 InPayloadLength,
			uint32 InPropertiesLength,
			EMqttifyQualityOfService InQualityOfService);
		uint32 GetLength() const override;

		void Encode(FMemoryWriter& InWriter) override;
		void Decode(FArrayReader& InReader) override;

		/**
		 * @brief Get the properties.
		 * @return The properties.
		 */
		const FMqttifyProperties& GetProperties() const { return Properties; }

		/**
		 * @brief Get a duplicate of the packet. Used when QoS > 0
		 * and we need to resend the packet.
		 */
		static TSharedRef<TMqttifyPublishPacket> GetDuplicate(TSharedRef<TMqttifyPublishPacket>&& InPublishPacket)
		{
			return MakeShareable<TMqttifyPublishPacket>(
				new TMqttifyPublishPacket{
					FPrivateToken{},
					MoveTemp(InPublishPacket->Payload),
					MoveTemp(InPublishPacket->TopicName),
					InPublishPacket->PacketIdentifier,
					InPublishPacket->GetQualityOfService(),
					InPublishPacket->GetShouldRetain(),
					true,
					MoveTemp(InPublishPacket->Properties)
				});
		}

	private:
		explicit TMqttifyPublishPacket(
			FPrivateToken,
			TArray<uint8>&& InPayload,
			FString&& InTopicName,
			const uint16 InPacketIdentifier,
			const EMqttifyQualityOfService InQualityOfService,
			const bool bInShouldRetain,
			const bool bInIsDuplicate,
			FMqttifyProperties&& InProperties)
			: FMqttifyPublishPacketBase{MoveTemp(InPayload), MoveTemp(InTopicName), InPacketIdentifier},
			Properties{MoveTemp(InProperties)}
		{
			const uint32 PayloadSize = GetLength(
				TopicName.Len(),
				Payload.Num(),
				Properties.GetLength(),
				InQualityOfService);
			FixedHeader = FMqttifyFixedHeader::Create(
				this,
				PayloadSize,
				bInShouldRetain,
				InQualityOfService,
				bInIsDuplicate);
		}

	private:
		FMqttifyProperties Properties;
	};

	using FMqttifyPublishPacket3 = TMqttifyPublishPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>;
	using FMqttifyPublishPacket5 = TMqttifyPublishPacket<EMqttifyProtocolVersion::Mqtt_5>;
} // namespace Mqttify
