#include "Packets/MqttifyPubCompPacket.h"

namespace Mqttify
{
#pragma region MQTT 3.1.1
	void TMqttifyPubCompPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Encode(FMemoryWriter& InWriter)
	{
		InWriter.SetByteSwapping(true);
		FixedHeader.Encode(InWriter);
		InWriter << PacketIdentifier;
	}

	void TMqttifyPubCompPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Decode(FArrayReader& InReader)
	{
		InReader.SetByteSwapping(true);
		if (FixedHeader.GetPacketType() != EMqttifyPacketType::PubComp)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[PubComp] %s %s"),
				MqttifyPacketType::InvalidPacketType,
				EnumToTCharString(FixedHeader.GetPacketType()));
			bIsValid = false;
		}

		InReader << PacketIdentifier;
	}
#pragma endregion MQTT 3.1.1

#pragma region MQTT 5

	uint32 TMqttifyPubCompPacket<EMqttifyProtocolVersion::Mqtt_5>::GetLength() const
	{
		uint32 PayloadLength = sizeof(PacketIdentifier);

		if (ReasonCode != EMqttifyReasonCode::Success)
		{
			PayloadLength += sizeof(ReasonCode); // Byte for Reason Code
			PayloadLength += Properties.GetLength();
		}

		return PayloadLength;
	}

	void TMqttifyPubCompPacket<EMqttifyProtocolVersion::Mqtt_5>::Encode(FMemoryWriter& InWriter)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("[Encode][PubComp]"));
		InWriter.SetByteSwapping(true);
		FixedHeader.Encode(InWriter);

		InWriter << PacketIdentifier;

		if (ReasonCode != EMqttifyReasonCode::Success)
		{
			uint8 RawReasonCode = static_cast<uint8>(ReasonCode);
			InWriter << RawReasonCode;
			Properties.Encode(InWriter);
		}
	}

	void TMqttifyPubCompPacket<EMqttifyProtocolVersion::Mqtt_5>::Decode(FArrayReader& InReader)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("[Decode][PubComp]"));
		InReader.SetByteSwapping(true);
		if (FixedHeader.GetPacketType() != EMqttifyPacketType::PubComp)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[PubComp] %s %s"),
				MqttifyPacketType::InvalidPacketType,
				EnumToTCharString(FixedHeader.GetPacketType()));
			ReasonCode = EMqttifyReasonCode::UnspecifiedError;
			bIsValid   = false;
		}

		InReader << PacketIdentifier;

		if (FixedHeader.GetRemainingLength() == 2)
		{
			ReasonCode = EMqttifyReasonCode::Success;
			return;
		}

		InReader << ReasonCode;

		switch (ReasonCode)
		{
			case EMqttifyReasonCode::Success:
				ReasonCode = EMqttifyReasonCode::Success;
				break;
			case EMqttifyReasonCode::PacketIdentifierNotFound:
				ReasonCode = EMqttifyReasonCode::PacketIdentifierNotFound;
				break;
			default:
				ReasonCode = EMqttifyReasonCode::UnspecifiedError;
				bIsValid = false;
				LOG_MQTTIFY(
					Error,
					TEXT("[PubComp] %s %s"),
					MqttifyReasonCode::InvalidReasonCode,
					EnumToTCharString(ReasonCode));
		}

		Properties.Decode(InReader);
	}

#pragma endregion MQTT 5
} // namespace Mqttify
