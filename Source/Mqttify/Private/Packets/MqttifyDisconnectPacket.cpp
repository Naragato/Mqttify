#include "Packets/MqttifyDisconnectPacket.h"

namespace Mqttify
{
#pragma region MQTT 3.1.1
	void TMqttifyDisconnectPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Encode(FMemoryWriter& InWriter)
	{
		InWriter.SetByteSwapping(true);
		FixedHeader.Encode(InWriter);
	}

	void TMqttifyDisconnectPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Decode(FArrayReader& InReader)
	{
		InReader.SetByteSwapping(true);
		if (FixedHeader.GetPacketType() != EMqttifyPacketType::Disconnect)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[Disconnect] %s %s."),
				MqttifyPacketType::InvalidPacketType,
				EnumToTCharString(FixedHeader.GetPacketType()));
			bIsValid = false;
		}
	}
#pragma endregion MQTT 3.1.1

#pragma region MQTT 5

	uint32 TMqttifyDisconnectPacket<EMqttifyProtocolVersion::Mqtt_5>::GetLength() const
	{
		if (ReasonCode != EMqttifyReasonCode::Success)
		{
			return sizeof(ReasonCode) + Properties.GetLength();
		}

		return 0;
	}

	void TMqttifyDisconnectPacket<EMqttifyProtocolVersion::Mqtt_5>::Encode(FMemoryWriter& InWriter)
	{
		InWriter.SetByteSwapping(true);
		FixedHeader.Encode(InWriter);

		if (ReasonCode != EMqttifyReasonCode::Success)
		{
			InWriter << ReasonCode;
			Properties.Encode(InWriter);
		}
	}

	void TMqttifyDisconnectPacket<EMqttifyProtocolVersion::Mqtt_5>::Decode(FArrayReader& InReader)
	{
		InReader.SetByteSwapping(true);
		if (FixedHeader.GetPacketType() != EMqttifyPacketType::Disconnect)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[Disconnect] %s %s."),
				MqttifyPacketType::InvalidPacketType,
				EnumToTCharString(FixedHeader.GetPacketType()));
			ReasonCode = EMqttifyReasonCode::UnspecifiedError;
			bIsValid   = false;
		}

		if (FixedHeader.GetRemainingLength() == 0)
		{
			ReasonCode = EMqttifyReasonCode::Success;
			return;
		}

		InReader << ReasonCode;

		if (!IsReasonCodeValid(ReasonCode))
		{
			bIsValid = false;
			LOG_MQTTIFY(
				Error,
				TEXT("[Disconnect] %s %s."),
				MqttifyReasonCode::InvalidReasonCode,
				EnumToTCharString(ReasonCode));
		}

		Properties.Decode(InReader);
	}

	bool TMqttifyDisconnectPacket<EMqttifyProtocolVersion::Mqtt_5>::IsReasonCodeValid(
		const EMqttifyReasonCode InReasonCode)
	{
		switch (InReasonCode)
		{
			case EMqttifyReasonCode::NormalDisconnection:
			case EMqttifyReasonCode::DisconnectWithWillMessage:
			case EMqttifyReasonCode::UnspecifiedError:
			case EMqttifyReasonCode::MalformedPacket:
			case EMqttifyReasonCode::ProtocolError:
			case EMqttifyReasonCode::ImplementationSpecificError:
			case EMqttifyReasonCode::NotAuthorized:
			case EMqttifyReasonCode::ServerBusy:
			case EMqttifyReasonCode::ServerShuttingDown:
			case EMqttifyReasonCode::KeepAliveTimeout:
			case EMqttifyReasonCode::SessionTakenOver:
			case EMqttifyReasonCode::TopicFilterInvalid:
			case EMqttifyReasonCode::TopicNameInvalid:
			case EMqttifyReasonCode::ReceiveMaximumExceeded:
			case EMqttifyReasonCode::TopicAliasInvalid:
			case EMqttifyReasonCode::PacketTooLarge:
			case EMqttifyReasonCode::MessageRateTooHigh:
			case EMqttifyReasonCode::QuotaExceeded:
			case EMqttifyReasonCode::AdministrativeAction:
			case EMqttifyReasonCode::PayloadFormatInvalid:
			case EMqttifyReasonCode::RetainNotSupported:
			case EMqttifyReasonCode::QoSNotSupported:
			case EMqttifyReasonCode::UseAnotherServer:
			case EMqttifyReasonCode::ServerMoved:
			case EMqttifyReasonCode::SharedSubscriptionsNotSupported:
			case EMqttifyReasonCode::ConnectionRateExceeded:
			case EMqttifyReasonCode::MaximumConnectTime:
			case EMqttifyReasonCode::SubscriptionIdentifiersNotSupported:
			case EMqttifyReasonCode::WildcardSubscriptionsNotSupported:
				return true;
			default:
				return false;
		}
	}

#pragma endregion MQTT 5
} // namespace Mqttify
