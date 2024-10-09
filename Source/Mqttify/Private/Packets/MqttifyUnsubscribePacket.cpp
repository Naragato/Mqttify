#include "Packets/MqttifyUnsubscribePacket.h"

#include "Mqtt/MqttifyErrorStrings.h"

namespace Mqttify
{
#pragma region MQTT 3.1.1
	uint32 TMqttifyUnsubscribePacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::GetLength() const
	{
		uint32 Length = sizeof(PacketIdentifier);

		for (auto TopicFilter : TopicFilters)
		{
			Length += TopicFilter.GetFilter().Len() + StringLengthFieldSize;
		}

		return Length;
	}

	void TMqttifyUnsubscribePacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Encode(FMemoryWriter& InWriter)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("[Encode][Unsubscribe]"));
		InWriter.SetByteSwapping(true);
		FixedHeader.Encode(InWriter);

		InWriter << PacketIdentifier;

		for (auto TopicFilter : TopicFilters)
		{
			Data::EncodeString(TopicFilter.GetFilter(), InWriter);
		}
	}

	void TMqttifyUnsubscribePacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Decode(FArrayReader& InReader)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("[Decode][Unsubscribe]"));
		InReader.SetByteSwapping(true);

		if (FixedHeader.GetPacketType() != EMqttifyPacketType::Unsubscribe)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[Unsubscribe] %s %s."),
				MqttifyPacketType::InvalidPacketType,
				EnumToTCharString(FixedHeader.GetPacketType()));
			bIsValid = false;
		}

		int32 PayloadSize = FixedHeader.GetRemainingLength();

		InReader << PacketIdentifier;
		PayloadSize -= sizeof(PacketIdentifier);

		int32 ReaderPos = InReader.Tell();
		while (PayloadSize > 0)
		{
			FString TopicFilter = Data::DecodeString(InReader);
			if (ReaderPos == InReader.Tell())
			{
				LOG_MQTTIFY(Error, TEXT("[Unsubscribe] %s"), ErrorStrings::ArchiveTooSmall);
				bIsValid = false;
				return;
			}
			ReaderPos = InReader.Tell();
			TopicFilters.Add(FMqttifyTopicFilter{ TopicFilter });
			PayloadSize -= TopicFilter.Len() + StringLengthFieldSize;
		}
	}
#pragma endregion MQTT 3.1.1

#pragma region MQTT 5


	uint32 TMqttifyUnsubscribePacket<EMqttifyProtocolVersion::Mqtt_5>::GetLength() const
	{
		uint32 Length = sizeof(PacketIdentifier);

		for (auto TopicFilter : TopicFilters)
		{
			Length += TopicFilter.GetFilter().Len() + StringLengthFieldSize;
		}

		Length += Properties.GetLength();

		return Length;
	}

	void TMqttifyUnsubscribePacket<EMqttifyProtocolVersion::Mqtt_5>::Encode(FMemoryWriter& InWriter)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("[Encode][Unsubscribe]"));
		InWriter.SetByteSwapping(true);
		FixedHeader.Encode(InWriter);
		InWriter << PacketIdentifier;

		Properties.Encode(InWriter);
		for (auto TopicFilter : TopicFilters)
		{
			Data::EncodeString(TopicFilter.GetFilter(), InWriter);
		}
	}

	void TMqttifyUnsubscribePacket<EMqttifyProtocolVersion::Mqtt_5>::Decode(FArrayReader& InReader)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("[Decode][Unsubscribe]"));
		InReader.SetByteSwapping(true);
		if (FixedHeader.GetPacketType() != EMqttifyPacketType::Unsubscribe)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[Unsubscribe] %s %s."),
				MqttifyPacketType::InvalidPacketType,
				EnumToTCharString(FixedHeader.GetPacketType()));
			bIsValid = false;
		}

		int32 PayloadSize = FixedHeader.GetRemainingLength();
		InReader << PacketIdentifier;
		Properties.Decode(InReader);

		PayloadSize -= sizeof(PacketIdentifier);
		PayloadSize -= Properties.GetLength();

		int32 ReaderPos = InReader.Tell();
		while (PayloadSize > 0)
		{
			FString TopicFilter = Data::DecodeString(InReader);
			if (ReaderPos == InReader.Tell())
			{
				LOG_MQTTIFY(Error, TEXT("[Unsubscribe] %s"), ErrorStrings::ArchiveTooSmall);
				bIsValid = false;
				return;
			}
			ReaderPos = InReader.Tell();
			TopicFilters.Add(FMqttifyTopicFilter{ TopicFilter });
			PayloadSize -= TopicFilter.Len() + StringLengthFieldSize;
		}
	}

#pragma endregion MQTT 5
} // namespace Mqttify
