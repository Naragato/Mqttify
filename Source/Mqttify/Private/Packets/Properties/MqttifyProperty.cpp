#include "Packets/Properties/MqttifyProperty.h"

#include "LogMqttify.h"
#include "Serialization/MqttifyFArchiveEncodeDecode.h"

namespace Mqttify
{
	EMqttifyPropertyIdentifier FMqttifyProperty::DecodePropertyIdentifier(FArrayReader& InReader)
	{
		uint8 IntCode;
		InReader << IntCode;

		const bool bIsIdentifierValid = MqttifyPropertyIdentifier::IsValid(IntCode);

		if (!bIsIdentifierValid)
		{
			return EMqttifyPropertyIdentifier::Unknown;
		}

		return static_cast<EMqttifyPropertyIdentifier>(IntCode);
	}


	void FMqttifyProperty::Encode(FMemoryWriter& InWriter)
	{
		InWriter.SetByteSwapping(true);
		uint8 IntCode = static_cast<uint8>(Identifier);
		InWriter << IntCode;
		switch (Identifier)
		{
			case EMqttifyPropertyIdentifier::SubscriptionIdentifier:
				Data::EncodeVariableByteInteger(Data.GetSubtype<uint32>(), InWriter);
				break;
			case EMqttifyPropertyIdentifier::AuthenticationData:
			case EMqttifyPropertyIdentifier::CorrelationData:
			{
				uint16 DataLength = Data.GetSubtype<TArray<uint8>>().Num();
				InWriter << DataLength;
				InWriter.Serialize(Data.GetSubtype<TArray<uint8>>().GetData(), Data.GetSubtype<TArray<uint8>>().Num());
				break;
			}
			case EMqttifyPropertyIdentifier::ContentType:
			case EMqttifyPropertyIdentifier::ResponseTopic:
			case EMqttifyPropertyIdentifier::AssignedClientIdentifier:
			case EMqttifyPropertyIdentifier::AuthenticationMethod:
			case EMqttifyPropertyIdentifier::ResponseInformation:
			case EMqttifyPropertyIdentifier::ServerReference:
			case EMqttifyPropertyIdentifier::ReasonString:
				Data::EncodeString(Data.GetSubtype<FString>(), InWriter);
				break;
			case EMqttifyPropertyIdentifier::ServerKeepAlive:
			case EMqttifyPropertyIdentifier::ReceiveMaximum:
			case EMqttifyPropertyIdentifier::TopicAliasMaximum:
			case EMqttifyPropertyIdentifier::TopicAlias:
				InWriter << Data.GetSubtype<uint16>();
				break;
			case EMqttifyPropertyIdentifier::UserProperty:
				Data::EncodeString(Data.GetSubtype<TTuple<FString, FString>>().Key, InWriter);
				Data::EncodeString(Data.GetSubtype<TTuple<FString, FString>>().Value, InWriter);
				break;
			case EMqttifyPropertyIdentifier::MessageExpiryInterval:
			case EMqttifyPropertyIdentifier::SessionExpiryInterval:
			case EMqttifyPropertyIdentifier::WillDelayInterval:
			case EMqttifyPropertyIdentifier::MaximumPacketSize:
				InWriter << Data.GetSubtype<uint32>();
				break;
			case EMqttifyPropertyIdentifier::PayloadFormatIndicator:
			case EMqttifyPropertyIdentifier::RequestProblemInformation:
			case EMqttifyPropertyIdentifier::RequestResponseInformation:
			case EMqttifyPropertyIdentifier::MaximumQoS:
			case EMqttifyPropertyIdentifier::RetainAvailable:
			case EMqttifyPropertyIdentifier::WildcardSubscriptionAvailable:
			case EMqttifyPropertyIdentifier::SubscriptionIdentifierAvailable:
			case EMqttifyPropertyIdentifier::SharedSubscriptionAvailable:
				InWriter << Data.GetSubtype<uint8>();
				break;
			case EMqttifyPropertyIdentifier::Max:
			case EMqttifyPropertyIdentifier::Unknown:
			default:
				LOG_MQTTIFY(Warning, TEXT("[Property] %s"), InvalidPropertyIdentifier);
				break;
		}
	}

	void FMqttifyProperty::Decode(FArrayReader& InReader)
	{
		InReader.SetByteSwapping(true);
		Identifier = DecodePropertyIdentifier(InReader);

		switch (Identifier)
		{
			case EMqttifyPropertyIdentifier::PayloadFormatIndicator:
			case EMqttifyPropertyIdentifier::RequestProblemInformation:
			case EMqttifyPropertyIdentifier::RequestResponseInformation:
			case EMqttifyPropertyIdentifier::MaximumQoS:
			case EMqttifyPropertyIdentifier::RetainAvailable:
			case EMqttifyPropertyIdentifier::WildcardSubscriptionAvailable:
			case EMqttifyPropertyIdentifier::SubscriptionIdentifierAvailable:
			case EMqttifyPropertyIdentifier::SharedSubscriptionAvailable:
			{
				uint8 Value;
				InReader << Value;
				Data.SetSubtype<uint8>(Value);
			}
			break;
			case EMqttifyPropertyIdentifier::ServerKeepAlive:
			case EMqttifyPropertyIdentifier::ReceiveMaximum:
			case EMqttifyPropertyIdentifier::TopicAliasMaximum:
			case EMqttifyPropertyIdentifier::TopicAlias:
			{
				uint16 Value;
				InReader << Value;
				Data.SetSubtype<uint16>(Value);
			}
			break;
			case EMqttifyPropertyIdentifier::MessageExpiryInterval:
			case EMqttifyPropertyIdentifier::SessionExpiryInterval:
			case EMqttifyPropertyIdentifier::WillDelayInterval:
			case EMqttifyPropertyIdentifier::MaximumPacketSize:
			{
				uint32 Value;
				InReader << Value;
				Data.SetSubtype<uint32>(Value);
			}
			break;
			case EMqttifyPropertyIdentifier::SubscriptionIdentifier:
				Data.SetSubtype<uint32>(Data::DecodeVariableByteInteger(InReader));
				break;
			case EMqttifyPropertyIdentifier::CorrelationData:
			case EMqttifyPropertyIdentifier::AuthenticationData:
			{
				uint16 DataLength;
				TArray<uint8> Value;
				InReader << DataLength;
				Value.AddUninitialized(DataLength);
				for (uint16 i = 0; i < DataLength; ++i)
				{
					InReader << Value[i];
				}
				Data.SetSubtype<TArray<uint8>>(Value);
			}
			break;
			case EMqttifyPropertyIdentifier::ContentType:
			case EMqttifyPropertyIdentifier::ResponseTopic:
			case EMqttifyPropertyIdentifier::AssignedClientIdentifier:
			case EMqttifyPropertyIdentifier::ResponseInformation:
			case EMqttifyPropertyIdentifier::AuthenticationMethod:
			case EMqttifyPropertyIdentifier::ServerReference:
			case EMqttifyPropertyIdentifier::ReasonString:
				Data.SetSubtype<FString>(Data::DecodeString(InReader));
				break;
			case EMqttifyPropertyIdentifier::UserProperty:
			{
				TTuple<FString, FString> Value;
				Value.Key   = Data::DecodeString(InReader);
				Value.Value = Data::DecodeString(InReader);
				Data.SetSubtype<TTuple<FString, FString>>(Value);
			}
			break;
			case EMqttifyPropertyIdentifier::Max:
			case EMqttifyPropertyIdentifier::Unknown:
				Data.Reset();
				LOG_MQTTIFY(Warning, TEXT("[Property] %s"), InvalidPropertyIdentifier);
				break;
		}
	}

	uint32 FMqttifyProperty::GetLength() const
	{
		switch (Identifier)
		{
			case EMqttifyPropertyIdentifier::PayloadFormatIndicator:
			case EMqttifyPropertyIdentifier::RequestProblemInformation:
			case EMqttifyPropertyIdentifier::RequestResponseInformation:
			case EMqttifyPropertyIdentifier::MaximumQoS:
			case EMqttifyPropertyIdentifier::RetainAvailable:
			case EMqttifyPropertyIdentifier::WildcardSubscriptionAvailable:
			case EMqttifyPropertyIdentifier::SubscriptionIdentifierAvailable:
			case EMqttifyPropertyIdentifier::SharedSubscriptionAvailable:
				return sizeof(uint8) + 1;
			case EMqttifyPropertyIdentifier::ServerKeepAlive:
			case EMqttifyPropertyIdentifier::ReceiveMaximum:
			case EMqttifyPropertyIdentifier::TopicAliasMaximum:
			case EMqttifyPropertyIdentifier::TopicAlias:
				return sizeof(uint16) + 1;
			case EMqttifyPropertyIdentifier::MessageExpiryInterval:
			case EMqttifyPropertyIdentifier::SessionExpiryInterval:
			case EMqttifyPropertyIdentifier::WillDelayInterval:
			case EMqttifyPropertyIdentifier::MaximumPacketSize:
				return sizeof(uint32) + 1;
			case EMqttifyPropertyIdentifier::SubscriptionIdentifier:
				return Data::VariableByteIntegerSize(Data.GetSubtype<uint32>()) + 1;
			case EMqttifyPropertyIdentifier::CorrelationData:
			case EMqttifyPropertyIdentifier::AuthenticationData:
				return sizeof(uint16) + Data.GetSubtype<TArray<uint8>>().Num() + 1;
			case EMqttifyPropertyIdentifier::ContentType:
			case EMqttifyPropertyIdentifier::ResponseTopic:
			case EMqttifyPropertyIdentifier::AssignedClientIdentifier:
			case EMqttifyPropertyIdentifier::AuthenticationMethod:
			case EMqttifyPropertyIdentifier::ResponseInformation:
			case EMqttifyPropertyIdentifier::ServerReference:
			case EMqttifyPropertyIdentifier::ReasonString:
				return sizeof(uint16) + Data.GetSubtype<FString>().Len() + 1;
			case EMqttifyPropertyIdentifier::UserProperty:
				return sizeof(uint16) + Data.GetSubtype<TTuple<FString, FString>>().Key.Len() +
					sizeof(uint16) + Data.GetSubtype<TTuple<FString, FString>>().Value.Len() + 1;
			case EMqttifyPropertyIdentifier::Max:
			case EMqttifyPropertyIdentifier::Unknown:
			default:
				LOG_MQTTIFY(Warning, TEXT("[Property] %s"), InvalidPropertyIdentifier);
				return 0;
		}
	}

	bool FMqttifyProperty::TryGetValue(uint8& OutValue) const
	{
		if (Data.HasSubtype<uint8>())
		{
			OutValue = Data.GetSubtype<uint8>();
			return true;
		}
		return false;
	}

	bool FMqttifyProperty::TryGetValue(uint16& OutValue) const
	{
		if (Data.HasSubtype<uint16>())
		{
			OutValue = Data.GetSubtype<uint16>();
			return true;
		}
		return false;
	}

	bool FMqttifyProperty::TryGetValue(uint32& OutValue) const
	{
		if (Data.HasSubtype<uint32>())
		{
			OutValue = Data.GetSubtype<uint32>();
			return true;
		}
		return false;
	}

	bool FMqttifyProperty::TryGetValue(TArray<uint8>& OutValue) const
	{
		if (Data.HasSubtype<TArray<uint8>>())
		{
			OutValue = Data.GetSubtype<TArray<uint8>>();
			return true;
		}
		return false;
	}

	bool FMqttifyProperty::TryGetValue(FString& OutValue) const
	{
		if (Data.HasSubtype<FString>())
		{
			OutValue = Data.GetSubtype<FString>();
			return true;
		}
		return false;
	}

	bool FMqttifyProperty::TryGetValue(TTuple<FString, FString>& OutValue) const
	{
		if (Data.HasSubtype<TTuple<FString, FString>>())
		{
			OutValue = Data.GetSubtype<TTuple<FString, FString>>();
			return true;
		}
		return false;
	}
} // Mqttify
