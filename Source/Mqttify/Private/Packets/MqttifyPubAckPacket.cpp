#include "Packets/MqttifyPubAckPacket.h"

namespace Mqttify
{
#pragma region MQTT 3.1.1
	void TMqttifyPubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Encode(FMemoryWriter& InWriter)
	{
		InWriter.SetByteSwapping(true);
		FixedHeader.Encode(InWriter);
		InWriter << PacketIdentifier;
	}

	void TMqttifyPubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Decode(FArrayReader& InReader)
	{
		InReader.SetByteSwapping(true);
		if (FixedHeader.GetPacketType() != EMqttifyPacketType::PubAck)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[PubAck] %s %s."),
				MqttifyPacketType::InvalidPacketType,
				EnumToTCharString(FixedHeader.GetPacketType()));
			bIsValid = false;
		}

		InReader << PacketIdentifier;
	}
#pragma endregion MQTT 3.1.1

#pragma region MQTT 5

	uint32 TMqttifyPubAckPacket<EMqttifyProtocolVersion::Mqtt_5>::GetLength() const
	{
		uint32 PayloadLength = sizeof(PacketIdentifier);

		if (ReasonCode != EMqttifyReasonCode::Success)
		{
			PayloadLength += sizeof(ReasonCode);
			PayloadLength += Properties.GetLength();
		}

		return PayloadLength;
	}

	void TMqttifyPubAckPacket<EMqttifyProtocolVersion::Mqtt_5>::Encode(FMemoryWriter& InWriter)
	{
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

	void TMqttifyPubAckPacket<EMqttifyProtocolVersion::Mqtt_5>::Decode(FArrayReader& InReader)
	{
		InReader.SetByteSwapping(true);
		if (FixedHeader.GetPacketType() != EMqttifyPacketType::PubAck)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[PubAck] %s %s."),
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

		uint8 RawReturnCode = 0;
		InReader << RawReturnCode;

		switch (RawReturnCode)
		{
			case static_cast<uint8>(EMqttifyReasonCode::Success):
				ReasonCode = EMqttifyReasonCode::Success;
				break;
			case static_cast<uint8>(EMqttifyReasonCode::NoMatchingSubscribers):
				ReasonCode = EMqttifyReasonCode::NoMatchingSubscribers;
				break;
			case static_cast<uint8>(EMqttifyReasonCode::UnspecifiedError):
				ReasonCode = EMqttifyReasonCode::UnspecifiedError;
				break;
			case static_cast<uint8>(EMqttifyReasonCode::ImplementationSpecificError):
				ReasonCode = EMqttifyReasonCode::ImplementationSpecificError;
				break;
			case static_cast<uint8>(EMqttifyReasonCode::NotAuthorized):
				ReasonCode = EMqttifyReasonCode::NotAuthorized;
				break;
			case static_cast<uint8>(EMqttifyReasonCode::TopicNameInvalid):
				ReasonCode = EMqttifyReasonCode::TopicNameInvalid;
				break;
			case static_cast<uint8>(EMqttifyReasonCode::PacketIdentifierInUse):
				ReasonCode = EMqttifyReasonCode::PacketIdentifierInUse;
				break;
			case static_cast<uint8>(EMqttifyReasonCode::QuotaExceeded):
				ReasonCode = EMqttifyReasonCode::QuotaExceeded;
				break;
			case static_cast<uint8>(EMqttifyReasonCode::PayloadFormatInvalid):
				ReasonCode = EMqttifyReasonCode::PayloadFormatInvalid;
				break;
			default:
				ReasonCode = EMqttifyReasonCode::UnspecifiedError;
				bIsValid = false;
				LOG_MQTTIFY(
					Error,
					TEXT("[PubAck] %s %s."),
					MqttifyReasonCode::InvalidReasonCode,
					EnumToTCharString(ReasonCode));
		}

		Properties.Decode(InReader);
	}

#pragma endregion MQTT 5
} // namespace Mqttify
