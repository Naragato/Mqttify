#include "Packets/MqttifyPublishPacket.h"

namespace Mqttify
{
#pragma region MQTT 3.1.1
	uint32 TMqttifyPublishPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::GetLength(
		const uint32 InTopicNameLength,
		const uint32 InPayloadLength,
		const EMqttifyQualityOfService InQualityOfService)
	{
		uint32 PayloadLength = InPayloadLength + 2 + InTopicNameLength; // 2 bytes for topic name length

		if (static_cast<uint8>(InQualityOfService) > static_cast<uint8>(EMqttifyQualityOfService::AtMostOnce))
		{
			PayloadLength += 2; // 2 bytes for packet identifier
		}

		return PayloadLength;
	}

	uint32 TMqttifyPublishPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::GetLength() const
	{
		return GetLength(TopicName.Len(), Payload.Num(), GetQualityOfService());
	}

	void TMqttifyPublishPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Encode(FMemoryWriter& InWriter)
	{
		InWriter.SetByteSwapping(true);
		FixedHeader.Encode(InWriter);

		Data::EncodeString(TopicName, InWriter);
		if (static_cast<uint8>(GetQualityOfService()) > static_cast<uint8>(EMqttifyQualityOfService::AtMostOnce))
		{
			InWriter << PacketIdentifier;
		}

		Data::EncodePayload(Payload, InWriter);
	}

	void TMqttifyPublishPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Decode(FArrayReader& InReader)
	{
		InReader.SetByteSwapping(true);
		if (FixedHeader.GetPacketType() != EMqttifyPacketType::Publish)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[Publish] %s %s."),
				MqttifyPacketType::InvalidPacketType,
				EnumToTCharString(FixedHeader.GetPacketType()));
			bIsValid = false;
		}

		int32 PayloadSize = FixedHeader.GetRemainingLength();
		PayloadSize -= 2; // Topic length size field
		TopicName = Data::DecodeString(InReader);
		PayloadSize -= TopicName.Len();

		if (static_cast<uint8>(GetQualityOfService()) > static_cast<uint8>(EMqttifyQualityOfService::AtMostOnce))
		{
			InReader << PacketIdentifier;
			PayloadSize -= 2;
		}

		Payload = Data::DecodePayload(InReader, PayloadSize);
	}
#pragma endregion MQTT 3.1.1

#pragma region MQTT 5

	uint32 TMqttifyPublishPacket<EMqttifyProtocolVersion::Mqtt_5>::GetLength(const uint32 InTopicNameLength,
																			const uint32 InPayloadLength,
																			const uint32 InPropertiesLength,
																			EMqttifyQualityOfService
																			InQualityOfService)
	{
		uint32 PayloadLength = StringLengthFieldSize + InTopicNameLength + InPropertiesLength + InPayloadLength;

		if (static_cast<uint8>(InQualityOfService) > static_cast<uint8>(EMqttifyQualityOfService::AtMostOnce))
		{
			PayloadLength += sizeof(PacketIdentifier);
		}

		return PayloadLength;
	}

	uint32 TMqttifyPublishPacket<EMqttifyProtocolVersion::Mqtt_5>::GetLength() const
	{
		return GetLength(TopicName.Len(), Payload.Num(), Properties.GetLength(), GetQualityOfService());
	}

	void TMqttifyPublishPacket<EMqttifyProtocolVersion::Mqtt_5>::Encode(FMemoryWriter& InWriter)
	{
		InWriter.SetByteSwapping(true);
		FixedHeader.Encode(InWriter);

		Data::EncodeString(TopicName, InWriter);
		if (static_cast<uint8>(GetQualityOfService()) > static_cast<uint8>(EMqttifyQualityOfService::AtMostOnce))
		{
			InWriter << PacketIdentifier;
		}

		Properties.Encode(InWriter);
		Data::EncodePayload(Payload, InWriter);
	}

	void TMqttifyPublishPacket<EMqttifyProtocolVersion::Mqtt_5>::Decode(FArrayReader& InReader)
	{
		InReader.SetByteSwapping(true);
		if (FixedHeader.GetPacketType() != EMqttifyPacketType::Publish)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[Publish] %s %s."),
				MqttifyPacketType::InvalidPacketType,
				EnumToTCharString(FixedHeader.GetPacketType()));
			bIsValid = false;
		}

		int32 PayloadSize = FixedHeader.GetRemainingLength();
		PayloadSize -= 2; // Topic length size field
		TopicName = Data::DecodeString(InReader);
		PayloadSize -= TopicName.Len();

		if (static_cast<uint8>(GetQualityOfService()) > static_cast<uint8>(EMqttifyQualityOfService::AtMostOnce))
		{
			InReader << PacketIdentifier;
			PayloadSize -= 2;
		}

		Properties.Decode(InReader);
		PayloadSize -= Properties.GetLength();

		Payload = Data::DecodePayload(InReader, PayloadSize);
	}

#pragma endregion MQTT 5
} // namespace Mqttify
