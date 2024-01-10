#include "Packets/MqttifyAuthPacket.h"

namespace Mqttify
{
#pragma region MQTT 5
	uint32 FMqttifyAuthPacket::GetLength() const
	{
		if (ReasonCode != EMqttifyReasonCode::Success)
		{
			return sizeof(ReasonCode) + Properties.GetLength();
		}

		return 0;
	}

	void FMqttifyAuthPacket::Encode(FMemoryWriter& InWriter)
	{
		InWriter.SetByteSwapping(true);
		FixedHeader.Encode(InWriter);

		if (ReasonCode != EMqttifyReasonCode::Success)
		{
			InWriter << ReasonCode;
			Properties.Encode(InWriter);
		}
	}

	void FMqttifyAuthPacket::Decode(FArrayReader& InReader)
	{
		InReader.SetByteSwapping(true);
		if (FixedHeader.GetPacketType() != EMqttifyPacketType::Auth)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[Auth] %s %s."),
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
			LOG_MQTTIFY(
				Error,
				TEXT("[Auth] %s %s."),
				MqttifyReasonCode::InvalidReasonCode,
				EnumToTCharString(ReasonCode));
			bIsValid = false;
		}

		Properties.Decode(InReader);

#if !UE_BUILD_SHIPPING
		if (!ArePropertiesValid(Properties))
		{
			LOG_MQTTIFY(Error, TEXT("[Auth] %s."), FMqttifyProperty::FailedToReadPropertyError);
			bIsValid = false;
		}
#endif // !UE_BUILD_SHIPPING
	}

	bool FMqttifyAuthPacket::ArePropertiesValid(
		const FMqttifyProperties& InProperties)
	{
		for (const FMqttifyProperty& Property : InProperties.GetProperties())
		{
			switch (Property.GetIdentifier())
			{
				case EMqttifyPropertyIdentifier::AuthenticationMethod:
				case EMqttifyPropertyIdentifier::AuthenticationData:
				case EMqttifyPropertyIdentifier::ReasonString:
				case EMqttifyPropertyIdentifier::UserProperty:
					return true;
				default:
					LOG_MQTTIFY(
						Error,
						TEXT("[Auth] %s %s."),
						FMqttifyProperty::FailedToReadPropertyError,
						EnumToTCharString(Property.GetIdentifier()));
					return false;
			}
		}
		return true;
	}

	bool FMqttifyAuthPacket::IsReasonCodeValid(
		const EMqttifyReasonCode InReasonCode)
	{
		switch (InReasonCode)
		{
			case EMqttifyReasonCode::Success:
			case EMqttifyReasonCode::ContinueAuthentication:
			case EMqttifyReasonCode::ReAuthenticate:
				return true;
			default:
				return false;
		}
	}

#pragma endregion MQTT 5
} // namespace Mqttify
