#include "Packets/MqttifySubAckPacket.h"

#include "Mqtt/MqttifyErrorStrings.h"

namespace Mqttify
{
#pragma region MQTT 3.1.1
	uint32 TMqttifySubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::GetLength() const
	{
		uint32 PayloadLength = sizeof(PacketIdentifier); // 2 for identifier, 1 byte for reason code
		PayloadLength += ReturnCodes.Num();

		return PayloadLength;
	}

	void TMqttifySubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Encode(FMemoryWriter& InWriter)
	{
		InWriter.SetByteSwapping(true);
		FixedHeader.Encode(InWriter);
		InWriter << PacketIdentifier;

		for (EMqttifySubscribeReturnCode& ReturnCode : ReturnCodes)
		{
			InWriter << ReturnCode;
		}
	}

	void TMqttifySubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Decode(FArrayReader& InReader)
	{
		InReader.SetByteSwapping(true);
		if (FixedHeader.GetPacketType() != EMqttifyPacketType::SubAck)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[SubAck] %s %s."),
				MqttifyPacketType::InvalidPacketType,
				EnumToTCharString(FixedHeader.GetPacketType()));
			bIsValid = false;
		}

		InReader << PacketIdentifier;

		uint32 PayloadSize = FixedHeader.GetRemainingLength() - sizeof(PacketIdentifier);

		int32 ReaderPos = InReader.Tell();
		while (PayloadSize > 0)
		{
			EMqttifySubscribeReturnCode ReturnCode;
			InReader << ReturnCode;
			if (ReaderPos == InReader.Tell())
			{
				LOG_MQTTIFY(Error, TEXT("[SubAck] %s"), ErrorStrings::ArchiveTooSmall);
				bIsValid = false;
				return;
			}
			ReaderPos = InReader.Tell();
			ReturnCodes.Add(ReturnCode);
			PayloadSize -= sizeof(EMqttifySubscribeReturnCode);
		}
	}
#pragma endregion MQTT 3.1.1

#pragma region MQTT 5

	uint32 TMqttifySubAckPacket<EMqttifyProtocolVersion::Mqtt_5>::GetLength() const
	{
		const uint32 PayloadLength = sizeof(PacketIdentifier) + ReasonCodes.Num() + Properties.GetLength();
		return PayloadLength;
	}

	void TMqttifySubAckPacket<EMqttifyProtocolVersion::Mqtt_5>::Encode(FMemoryWriter& InWriter)
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

	void TMqttifySubAckPacket<EMqttifyProtocolVersion::Mqtt_5>::Decode(FArrayReader& InReader)
	{
		InReader.SetByteSwapping(true);
		if (FixedHeader.GetPacketType() != EMqttifyPacketType::SubAck)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[SubAck] %s %s."),
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
				LOG_MQTTIFY(Error, TEXT("[SubAck] %s"), ErrorStrings::ArchiveTooSmall);
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
