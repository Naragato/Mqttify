#include "Packets/MqttifyUnsubAckPacket.h"

#include "Mqtt/MqttifyErrorStrings.h"

namespace Mqttify
{
#pragma region MQTT 3.1.1
	uint32 TMqttifyUnsubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::GetLength() const
	{
		return sizeof(PacketIdentifier);
	}

	void TMqttifyUnsubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Encode(FMemoryWriter& InWriter)
	{
		InWriter.SetByteSwapping(true);
		FixedHeader.Encode(InWriter);
		InWriter << PacketIdentifier;
	}

	void TMqttifyUnsubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Decode(FArrayReader& InReader)
	{
		InReader.SetByteSwapping(true);
		if (FixedHeader.GetPacketType() != EMqttifyPacketType::UnsubAck)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[UnsubAck] %s %s."),
				MqttifyPacketType::InvalidPacketType,
				EnumToTCharString(FixedHeader.GetPacketType()));
			bIsValid = false;
		}

		InReader << PacketIdentifier;
	}
#pragma endregion MQTT 3.1.1

#pragma region MQTT 5

	uint32 TMqttifyUnsubAckPacket<EMqttifyProtocolVersion::Mqtt_5>::GetLength() const
	{
		const uint32 PayloadLength = sizeof(PacketIdentifier) + ReasonCodes.Num() + Properties.GetLength();
		return PayloadLength;
	}

	void TMqttifyUnsubAckPacket<EMqttifyProtocolVersion::Mqtt_5>::Encode(FMemoryWriter& InWriter)
	{
		InWriter.SetByteSwapping(true);
		FixedHeader.Encode(InWriter);

		InWriter << PacketIdentifier;
		Properties.Encode(InWriter);

		for (EMqttifyReasonCode& ReasonCode : ReasonCodes)
		{
			InWriter << ReasonCode;
		}
	}

	void TMqttifyUnsubAckPacket<EMqttifyProtocolVersion::Mqtt_5>::Decode(FArrayReader& InReader)
	{
		InReader.SetByteSwapping(true);
		if (FixedHeader.GetPacketType() != EMqttifyPacketType::UnsubAck)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[UnsubAck] %s %s."),
				MqttifyPacketType::InvalidPacketType,
				EnumToTCharString(FixedHeader.GetPacketType()));
			bIsValid = false;
		}

		InReader << PacketIdentifier;
		Properties.Decode(InReader);

		// - 2 for Packet Identifier
		uint32 PayloadSize = FixedHeader.GetRemainingLength() - Properties.GetLength() - sizeof(PacketIdentifier);

		int32 ReaderPos = InReader.Tell();
		while (PayloadSize > 0)
		{
			EMqttifyReasonCode ReasonCode;
			InReader << ReasonCode;
			if (ReaderPos == InReader.Tell())
			{
				LOG_MQTTIFY(Error, TEXT("[UnsubAck] %s"), ErrorStrings::ArchiveTooSmall);
				bIsValid = false;
				return;
			}
			ReaderPos = InReader.Tell();
			ReasonCodes.Add(ReasonCode);
			PayloadSize -= sizeof(EMqttifyReasonCode);
		}
	}

#pragma endregion MQTT 5
} // namespace Mqttify
