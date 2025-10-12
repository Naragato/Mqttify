#include "Packets/MqttifyPubRelPacket.h"

namespace Mqttify
{
#pragma region MQTT 3.1.1
	void TMqttifyPubRelPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Encode(FMemoryWriter& InWriter)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("[Encode][PubRel]"));
		InWriter.SetByteSwapping(true);
		FixedHeader.Encode(InWriter);
		InWriter << PacketIdentifier;
	}

	void TMqttifyPubRelPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Decode(FArrayReader& InReader)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("[Decode][PubRel]"));
		InReader.SetByteSwapping(true);
		if (FixedHeader.GetPacketType() != EMqttifyPacketType::PubRel)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[PubRel] %s %s"),
				MqttifyPacketType::InvalidPacketType,
				EnumToTCharString(FixedHeader.GetPacketType()));
			bIsValid = false;
		}

		InReader << PacketIdentifier;
	}
#pragma endregion MQTT 3.1.1

#pragma region MQTT 5

	uint32 TMqttifyPubRelPacket<EMqttifyProtocolVersion::Mqtt_5>::GetLength() const
	{
		uint32 PayloadLength = sizeof(PacketIdentifier);

		if (ReasonCode != EMqttifyReasonCode::Success)
		{
			PayloadLength += sizeof(ReasonCode);
			PayloadLength += Properties.GetLength();
		}

		return PayloadLength;
	}

	void TMqttifyPubRelPacket<EMqttifyProtocolVersion::Mqtt_5>::Encode(FMemoryWriter& InWriter)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("[Encode][PubRel]"));
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

	void TMqttifyPubRelPacket<EMqttifyProtocolVersion::Mqtt_5>::Decode(FArrayReader& InReader)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("[Decode][PubRel]"));
		InReader.SetByteSwapping(true);
		if (FixedHeader.GetPacketType() != EMqttifyPacketType::PubRel)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[PubRel] %s %s"),
				MqttifyPacketType::InvalidPacketType,
				EnumToTCharString(FixedHeader.GetPacketType()));
			ReasonCode = EMqttifyReasonCode::UnspecifiedError;
			bIsValid = false;
		}

		InReader << PacketIdentifier;

		if (FixedHeader.GetRemainingLength() == 2)
		{
			ReasonCode = EMqttifyReasonCode::Success;
			return;
		}

		uint8 RawReturnCode = 0;
		InReader << RawReturnCode;

		switch (RawReturnCode)
		{
			case static_cast<uint8>(EMqttifyReasonCode::Success):
				ReasonCode = EMqttifyReasonCode::Success;
				break;
			case static_cast<uint8>(EMqttifyReasonCode::PacketIdentifierNotFound):
				ReasonCode = EMqttifyReasonCode::PacketIdentifierNotFound;
				break;
			default:
				ReasonCode = EMqttifyReasonCode::UnspecifiedError;
				bIsValid = false;
				LOG_MQTTIFY(
					Error,
					TEXT("[PubRel] %s %s"),
					MqttifyReasonCode::InvalidReasonCode,
					EnumToTCharString(ReasonCode));
				break;
		}

		Properties.Decode(InReader);
	}

#pragma endregion MQTT 5
} // namespace Mqttify
