#include "Packets/MqttifySubscribePacket.h"

#include "Mqtt/MqttifyErrorStrings.h"

#pragma region MQTT 3.1.1


uint32 Mqttify::TMqttifySubscribePacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::GetLength() const
{
	uint32 Length = sizeof(PacketIdentifier);

	for (auto TopicFilter : TopicFilters)
	{
		Length += TopicFilter.GetFilter().Len() + StringLengthFieldSize + sizeof(EMqttifyQualityOfService);
	}

	return Length;
}

void Mqttify::TMqttifySubscribePacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Encode(FMemoryWriter& InWriter)
{
	LOG_MQTTIFY(VeryVerbose, TEXT("[Encode][Subscribe]"));
	InWriter.SetByteSwapping(true);
	FixedHeader.Encode(InWriter);

	InWriter << PacketIdentifier;

	for (auto TopicFilter : TopicFilters)
	{
		Data::EncodeString(TopicFilter.GetFilter(), InWriter);
		uint8 QualityOfService = static_cast<uint8>(TopicFilter.GetQualityOfService());
		InWriter << QualityOfService;
	}
}

void Mqttify::TMqttifySubscribePacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Decode(FArrayReader& InReader)
{
	LOG_MQTTIFY(VeryVerbose, TEXT("[Decode][Subscribe]"));
	InReader.SetByteSwapping(true);

	if (FixedHeader.GetPacketType() != EMqttifyPacketType::Subscribe)
	{
		LOG_MQTTIFY(
			Error,
			TEXT("[Subscribe] %s %s."),
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
		FString TopicFilter                       = Data::DecodeString(InReader);
		EMqttifyQualityOfService QualityOfService = EMqttifyQualityOfService::AtMostOnce;
		InReader << QualityOfService;
		if (ReaderPos == InReader.Tell())
		{
			LOG_MQTTIFY(Error, TEXT("[Subscribe] %s"), ErrorStrings::ArchiveTooSmall);
			bIsValid = false;
			return;
		}
		ReaderPos = InReader.Tell();
		PayloadSize -= TopicFilter.Len() + 3;

		switch (QualityOfService)
		{
			case EMqttifyQualityOfService::AtMostOnce:
			case EMqttifyQualityOfService::AtLeastOnce:
			case EMqttifyQualityOfService::ExactlyOnce:
				TopicFilters.Add(FMqttifyTopicFilter{ TopicFilter, QualityOfService });
			default:
				LOG_MQTTIFY(
					Error,
					TEXT("[Subscribe] %s %d"),
					MqttifyQualityOfService::InvalidQualityOfService,
					QualityOfService);
				bIsValid = false;
		}
	}
}
#pragma endregion MQTT 3.1.1

#pragma region MQTT 5


uint32 Mqttify::TMqttifySubscribePacket<EMqttifyProtocolVersion::Mqtt_5>::GetLength() const
{
	uint32 Length = sizeof(PacketIdentifier);

	for (auto TopicFilter : TopicFilters)
	{
		Length += TopicFilter.GetFilter().Len() + StringLengthFieldSize + 1; // 1 byte for Subscribe Options
	}

	Length += Properties.GetLength();

	return Length;
}

void Mqttify::TMqttifySubscribePacket<EMqttifyProtocolVersion::Mqtt_5>::Encode(FMemoryWriter& InWriter)
{
	LOG_MQTTIFY(VeryVerbose, TEXT("[Encode][Subscribe]"));
	InWriter.SetByteSwapping(true);
	FixedHeader.Encode(InWriter);
	InWriter << PacketIdentifier;
	Properties.Encode(InWriter);

	for (auto TopicFilter : TopicFilters)
	{
		Data::EncodeString(TopicFilter.GetFilter(), InWriter);
		uint8 SubscribeOptions = static_cast<uint8>(TopicFilter.GetQualityOfService());
		SubscribeOptions |= TopicFilter.GetIsNoLocal() ? 0x4 : 0;
		SubscribeOptions |= TopicFilter.GetIsRetainAsPublished() ? 0x8 : 0;
		const uint8 RetainHandling = static_cast<uint8>(TopicFilter.GetRetainHandlingOptions());
		SubscribeOptions |= RetainHandling << 4;
		InWriter << SubscribeOptions;
	}
}

void Mqttify::TMqttifySubscribePacket<EMqttifyProtocolVersion::Mqtt_5>::Decode(FArrayReader& InReader)
{
	LOG_MQTTIFY(VeryVerbose, TEXT("[Decode][Subscribe]"));
	InReader.SetByteSwapping(true);
	if (FixedHeader.GetPacketType() != EMqttifyPacketType::Subscribe)
	{
		LOG_MQTTIFY(
			Error,
			TEXT("[Subscribe] %s %s."),
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
		FString TopicFilter    = Data::DecodeString(InReader);
		uint8 SubscribeOptions = 0;
		InReader << SubscribeOptions;

		// If we didn't read anything, we're at the end of the stream and we should break out of the loop to avoid
		// infinite looping here
		if (ReaderPos == InReader.Tell())
		{
			LOG_MQTTIFY(Error, TEXT("[Subscribe] %s"), ErrorStrings::ArchiveTooSmall);
			bIsValid = false;
			return;
		}
		ReaderPos = InReader.Tell();

		PayloadSize -= TopicFilter.Len() + 3;
		// 2 bytes to store the length of the topic filter utf8 string and 1 byte for Subscribe Options
		const EMqttifyQualityOfService QualityOfService = static_cast<EMqttifyQualityOfService>(SubscribeOptions & 0x3);
		const bool bIsNoLocal = (SubscribeOptions & 0x4) != 0;
		const bool bIsRetainAsPublished = (SubscribeOptions & 0x8) != 0;
		const EMqttifyRetainHandlingOptions RetainHandling =
			static_cast<EMqttifyRetainHandlingOptions>((SubscribeOptions & 0x30) >> 4);

		if ((SubscribeOptions & 0xC0) == 0)
		{
			TopicFilters.Add(FMqttifyTopicFilter{ TopicFilter,
												QualityOfService,
												bIsNoLocal,
												bIsRetainAsPublished,
												RetainHandling });
		}
		else
		{
			LOG_MQTTIFY(Error, TEXT("[Subscribe] %s %x"), InvalidSubscribeOptions, SubscribeOptions);
			bIsValid = false;
		}
	}
}

#pragma endregion MQTT 5
