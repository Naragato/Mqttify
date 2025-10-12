#include "Packets/MqttifyConnAckPacket.h"

namespace Mqttify
{
#pragma region MQTT 3.1.1
	void TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Encode(FMemoryWriter& InWriter)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("[Encode][ConnAck]"));
		InWriter.SetByteSwapping(true);
		FixedHeader.Encode(InWriter);
		uint8 Flags = bSessionPresent ? 0x1 : 0;
		InWriter << Flags;

		InWriter << ReturnCode;
	}

	void TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Decode(FArrayReader& InReader)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("[Decode][ConnAck]"));
		InReader.SetByteSwapping(true);
		if (FixedHeader.GetPacketType() != EMqttifyPacketType::ConnAck)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[ConnAck] %s %s"),
				MqttifyPacketType::InvalidPacketType,
				EnumToTCharString(FixedHeader.GetPacketType()));
			ReturnCode = EMqttifyConnectReturnCode::RefusedProtocolVersion;
			bIsValid = false;
		}

		uint8 Flags = 0;
		InReader << Flags;

		bSessionPresent = (Flags & 0x1) == 0x1;

		InReader << ReturnCode;

		switch (ReturnCode)
		{
			case EMqttifyConnectReturnCode::Accepted:
				ReturnCode = EMqttifyConnectReturnCode::Accepted;
				break;
			case EMqttifyConnectReturnCode::RefusedBadUserNameOrPassword:
				ReturnCode = EMqttifyConnectReturnCode::RefusedBadUserNameOrPassword;
				break;
			case EMqttifyConnectReturnCode::RefusedIdentifierRejected:
				ReturnCode = EMqttifyConnectReturnCode::RefusedIdentifierRejected;
				break;
			case EMqttifyConnectReturnCode::RefusedServerUnavailable:
				ReturnCode = EMqttifyConnectReturnCode::RefusedServerUnavailable;
				break;
			case EMqttifyConnectReturnCode::RefusedProtocolVersion:
				ReturnCode = EMqttifyConnectReturnCode::RefusedProtocolVersion;
			case EMqttifyConnectReturnCode::RefusedNotAuthorized:
				ReturnCode = EMqttifyConnectReturnCode::RefusedNotAuthorized;
				break;
			default:
				bIsValid = false;
				LOG_MQTTIFY(
					Error,
					TEXT("[ConnAck] %s %s."),
					MqttifyConnectReturnCode::InvalidReturnCode,
					EnumToTCharString(ReturnCode));
				break;
		}
	}
#pragma endregion MQTT 3.1.1

#pragma region MQTT 5

	uint32 TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_5>::GetLength() const
	{
		uint32 PayloadLength = sizeof(ReasonCode) + sizeof(uint8); // 1 byte for flags
		PayloadLength += Properties.GetLength();

		return PayloadLength;
	}

	void TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_5>::Encode(FMemoryWriter& InWriter)
	{
		InWriter.SetByteSwapping(true);
		FixedHeader.Encode(InWriter);
		uint8 Flags = bSessionPresent ? 0x1 : 0;
		InWriter << Flags;

		InWriter << ReasonCode;

		Properties.Encode(InWriter);
	}

	void TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_5>::Decode(FArrayReader& InReader)
	{
		InReader.SetByteSwapping(true);
		if (FixedHeader.GetPacketType() != EMqttifyPacketType::ConnAck)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[ConnAck] %s %s."),
				MqttifyPacketType::InvalidPacketType,
				EnumToTCharString(FixedHeader.GetPacketType()));
			ReasonCode = EMqttifyReasonCode::UnspecifiedError;
			bIsValid = false;
		}

		uint8 Flags = 0;
		InReader << Flags;

		bSessionPresent = (Flags & 0x1) == 0x1;

		InReader << ReasonCode;

		if (!IsReasonCodeValid(ReasonCode))
		{
			bIsValid = false;
			LOG_MQTTIFY(
				Error,
				TEXT("[ConnAck] %s %s."),
				MqttifyReasonCode::InvalidReasonCode,
				EnumToTCharString(ReasonCode));
			ReasonCode = EMqttifyReasonCode::UnspecifiedError;
		}

		Properties.Decode(InReader);
	}

	bool TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_5>::IsReasonCodeValid(
		const EMqttifyReasonCode InReasonCode)
	{
		switch (InReasonCode)
		{
			case EMqttifyReasonCode::Success:
			case EMqttifyReasonCode::UnspecifiedError:
			case EMqttifyReasonCode::MalformedPacket:
			case EMqttifyReasonCode::ProtocolError:
			case EMqttifyReasonCode::ImplementationSpecificError:
			case EMqttifyReasonCode::UnsupportedProtocolVersion:
			case EMqttifyReasonCode::ClientIdentifierNotValid:
			case EMqttifyReasonCode::BadUsernameOrPassword:
			case EMqttifyReasonCode::NotAuthorized:
			case EMqttifyReasonCode::ServerUnavailable:
			case EMqttifyReasonCode::ServerBusy:
			case EMqttifyReasonCode::Banned:
			case EMqttifyReasonCode::BadAuthenticationMethod:
			case EMqttifyReasonCode::TopicNameInvalid:
			case EMqttifyReasonCode::PacketTooLarge:
			case EMqttifyReasonCode::QuotaExceeded:
			case EMqttifyReasonCode::PayloadFormatInvalid:
			case EMqttifyReasonCode::RetainNotSupported:
			case EMqttifyReasonCode::QoSNotSupported:
			case EMqttifyReasonCode::UseAnotherServer:
			case EMqttifyReasonCode::ServerMoved:
			case EMqttifyReasonCode::ConnectionRateExceeded:
				return true;
			default:
				return false;
		}

	}
#pragma endregion MQTT 5
} // namespace Mqttify
