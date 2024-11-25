#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Packets/MqttifyDisconnectPacket.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Tests/Packets/PacketComparision.h"

using namespace Mqttify;

BEGIN_DEFINE_SPEC(
	MqttifyDisconnectPacket,
	"Mqttify.Automation.MqttifyDisconnectPacket",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ServerContext |
	EAutomationTestFlags::CommandletContext  | EAutomationTestFlags::
	ProductFilter)


	const TArray<uint8> ValidMqtt3Packet = {
		0xE0,
		// Packet type (Disconnect)
		0x00
	};

	const TArray<uint8> InvalidMqtt3Packet = {
		0xE0,
		// Packet type (Disconnect)
		0x01
	};

	static TArray<uint8> SerializeDisconnectMqtt5(EMqttifyReasonCode ReasonCode)
	{
		TArray<uint8> DisconnectPacket = {
			0xE0,
			// Packet type
			2,
			// Size of Reason Code plus 1 byte for Properties Length of 0
			static_cast<uint8>(ReasonCode),
			// Reason code
			0 // Properties Length
		};
		return DisconnectPacket;
	}

	static TArray<uint8> SerializeDisconnectMqtt5(EMqttifyReasonCode ReasonCode, const FMqttifyProperties& Properties)
	{
		TArray<uint8> DisconnectPacket = {
			0xE0,
			// Packet type
		};

		Data::EmplaceVariableByteInteger(1 + Properties.GetLength(true), DisconnectPacket); // Remaining Length
		DisconnectPacket.Emplace(static_cast<uint8>(ReasonCode));
		Data::EmplaceVariableByteInteger(Properties.GetLength(false), DisconnectPacket);

		for (const FMqttifyProperty& Property : Properties.GetProperties())
		{
			DisconnectPacket.Add(static_cast<uint8>(Property.GetIdentifier()));

			if (Property.GetIdentifier() == EMqttifyPropertyIdentifier::SubscriptionIdentifier)
			{
				uint32 Value;
				Property.TryGetValue(Value);

				Data::EmplaceVariableByteInteger(Value, DisconnectPacket);
				continue;
			}

			uint8 Value;
			if (Property.TryGetValue(Value))
			{
				DisconnectPacket.Add(Value);
				continue;
			}

			uint16 Value16;
			if (Property.TryGetValue(Value16))
			{
				DisconnectPacket.Add(Value16 >> 8);
				DisconnectPacket.Add(Value16 & 0xFF);
				continue;
			}

			uint32 Value32;
			if (Property.TryGetValue(Value32))
			{
				DisconnectPacket.Add(Value32 >> 24);
				DisconnectPacket.Add((Value32 >> 16) & 0xFF);
				DisconnectPacket.Add((Value32 >> 8) & 0xFF);
				DisconnectPacket.Add(Value32 & 0xFF);
				continue;
			}

			FString ValueString;
			if (Property.TryGetValue(ValueString))
			{
				const uint16 StringLength = ValueString.Len();
				DisconnectPacket.Add(StringLength >> 8);
				DisconnectPacket.Add(StringLength & 0xFF);
				const auto String = StringCast<UTF8CHAR>(*ValueString);
				DisconnectPacket.Append(reinterpret_cast<const uint8*>(String.Get()), StringLength);
			}

			TTuple<FString, FString> ValueTuple;
			if (Property.TryGetValue(ValueTuple))
			{
				const uint16 StringLength = ValueTuple.Key.Len();
				DisconnectPacket.Add(StringLength >> 8);
				DisconnectPacket.Add(StringLength & 0xFF);
				const auto String = StringCast<UTF8CHAR>(*ValueTuple.Key);
				DisconnectPacket.Append(reinterpret_cast<const uint8*>(String.Get()), StringLength);

				const uint16 StringLength2 = ValueTuple.Value.Len();
				DisconnectPacket.Add(StringLength2 >> 8);
				DisconnectPacket.Add(StringLength2 & 0xFF);
				const auto String2 = StringCast<UTF8CHAR>(*ValueTuple.Value);
				DisconnectPacket.Append(reinterpret_cast<const uint8*>(String2.Get()), StringLength2);
			}

			TArray<uint8> ValueArray;
			if (Property.TryGetValue(ValueArray))
			{
				const uint16 ArrayLen = static_cast<uint16>(ValueArray.Num());
				DisconnectPacket.Add(ArrayLen >> 8);
				DisconnectPacket.Add(ArrayLen & 0xFF);
				DisconnectPacket.Append(ValueArray);
			}
		}

		return DisconnectPacket;
	}

	TSet<EMqttifyReasonCode> ValidReasonCodes = {
		EMqttifyReasonCode::DisconnectWithWillMessage,
		EMqttifyReasonCode::UnspecifiedError,
		EMqttifyReasonCode::MalformedPacket,
		EMqttifyReasonCode::ProtocolError,
		EMqttifyReasonCode::ImplementationSpecificError,
		EMqttifyReasonCode::NotAuthorized,
		EMqttifyReasonCode::ServerBusy,
		EMqttifyReasonCode::ServerShuttingDown,
		EMqttifyReasonCode::KeepAliveTimeout,
		EMqttifyReasonCode::SessionTakenOver,
		EMqttifyReasonCode::TopicFilterInvalid,
		EMqttifyReasonCode::ReceiveMaximumExceeded,
		EMqttifyReasonCode::TopicAliasInvalid,
		EMqttifyReasonCode::PacketTooLarge,
		EMqttifyReasonCode::MessageRateTooHigh,
		EMqttifyReasonCode::QuotaExceeded,
		EMqttifyReasonCode::AdministrativeAction,
		EMqttifyReasonCode::PayloadFormatInvalid,
		EMqttifyReasonCode::RetainNotSupported,
		EMqttifyReasonCode::QoSNotSupported,
		EMqttifyReasonCode::UseAnotherServer,
		EMqttifyReasonCode::ServerMoved,
		EMqttifyReasonCode::SharedSubscriptionsNotSupported,
		EMqttifyReasonCode::ConnectionRateExceeded,
		EMqttifyReasonCode::MaximumConnectTime,
		EMqttifyReasonCode::SubscriptionIdentifiersNotSupported,
		EMqttifyReasonCode::WildcardSubscriptionsNotSupported
	};

	TSet<EMqttifyReasonCode> InvalidReasonCodes = {
		EMqttifyReasonCode::GrantedQualityOfService1,
		EMqttifyReasonCode::GrantedQualityOfService2,
		EMqttifyReasonCode::NoMatchingSubscribers,
		EMqttifyReasonCode::NoSubscriptionExisted,
		EMqttifyReasonCode::ContinueAuthentication,
		EMqttifyReasonCode::ReAuthenticate,
		EMqttifyReasonCode::UnsupportedProtocolVersion,
		EMqttifyReasonCode::ClientIdentifierNotValid,
		EMqttifyReasonCode::BadUsernameOrPassword,
		EMqttifyReasonCode::Banned,
		EMqttifyReasonCode::BadAuthenticationMethod,
		EMqttifyReasonCode::PacketIdentifierInUse,
		EMqttifyReasonCode::PacketIdentifierNotFound,
	};

	TArray<FMqttifyProperty> ValidProperties = {
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::SessionExpiryInterval>(static_cast<uint32>(0)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::ReasonString>(FString(TEXT("test"))),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(
			TTuple<FString, FString>{FString(TEXT("test")), FString(TEXT("test"))}),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::ServerReference>(FString(TEXT("test")))
	};

	TArray<FMqttifyProperty> InvalidProperties = {
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::PayloadFormatIndicator>(static_cast<uint8>(true)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::PayloadFormatIndicator>(static_cast<uint8>(false)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::MessageExpiryInterval>(static_cast<uint32>(30)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::ContentType>(FString(TEXT("test")))
	};

END_DEFINE_SPEC(MqttifyDisconnectPacket)

void MqttifyDisconnectPacket::Define()
{
	Describe(
		"TMqttifyDisconnectPacket<EMqttifyProtocolVersion::Mqtt_5>",
		[this]
		{
			for (const EMqttifyReasonCode ReasonCode : ValidReasonCodes)
			{
				It(
					FString::Printf(TEXT("Should serialize with: reason code %s"), EnumToTCharString(ReasonCode)),
					[this, ReasonCode]
					{
						const TArray<uint8> Expected = SerializeDisconnectMqtt5(ReasonCode);
						TMqttifyDisconnectPacket<EMqttifyProtocolVersion::Mqtt_5> DisconnectPacket{ReasonCode};

						TestPacketsEqual(TEXT("Packet should match expected"), DisconnectPacket, Expected, this);
					});

				It(
					FString::Printf(TEXT("Should deserialize with: reason code %s"), EnumToTCharString(ReasonCode)),
					[this, ReasonCode]
					{
						const TArray<uint8> Expected = SerializeDisconnectMqtt5(ReasonCode);
						FArrayReader Reader;
						Reader.Append(Expected);
						const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
						TMqttifyDisconnectPacket<EMqttifyProtocolVersion::Mqtt_5> DisconnectPacket{Reader, Header};

						TestPacketsEqual(TEXT("Packet should match expected"), DisconnectPacket, Expected, this);
					});
			}

			for (const EMqttifyReasonCode ReasonCode : InvalidReasonCodes)
			{
				It(
					FString::Printf(
						TEXT("Should log error when serializing with invalid reason code %s"),
						EnumToTCharString(ReasonCode)),
					[this, ReasonCode]
					{
						AddExpectedError(
							MqttifyReasonCode::InvalidReasonCode,
							EAutomationExpectedMessageFlags::Contains);
						TMqttifyDisconnectPacket<EMqttifyProtocolVersion::Mqtt_5> DisconnectPacket{ReasonCode};
					});

				It(
					FString::Printf(
						TEXT("Should log error when deserializing with invalid reason code %s"),
						EnumToTCharString(ReasonCode)),
					[this, ReasonCode]
					{
						AddExpectedError(
							MqttifyReasonCode::InvalidReasonCode,
							EAutomationExpectedMessageFlags::Contains);
						const TArray<uint8> Expected = SerializeDisconnectMqtt5(ReasonCode);
						FArrayReader Reader;
						Reader.Append(Expected);
						const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
						TMqttifyDisconnectPacket<EMqttifyProtocolVersion::Mqtt_5> DisconnectPacket{Reader, Header};
					});
			}

			for (const FMqttifyProperty& Property : ValidProperties)
			{
				It(
					FString::Printf(
						TEXT("Should serialize with: Property %s %s"),
						EnumToTCharString(Property.GetIdentifier()),
						*Property.ToString()),
					[this, Property]
					{
						const TArray<uint8> Expected = SerializeDisconnectMqtt5(
							EMqttifyReasonCode::DisconnectWithWillMessage,
							FMqttifyProperties{{Property}});

						TMqttifyDisconnectPacket<EMqttifyProtocolVersion::Mqtt_5> DisconnectPacket{
							EMqttifyReasonCode::DisconnectWithWillMessage,
							FMqttifyProperties{{Property}}
						};
					});

				It(
					FString::Printf(
						TEXT("Should deserialize with: Property %s %s"),
						EnumToTCharString(Property.GetIdentifier()),
						*Property.ToString()),
					[this, Property]
					{
						const TArray<uint8> Expected = SerializeDisconnectMqtt5(
							EMqttifyReasonCode::DisconnectWithWillMessage,
							FMqttifyProperties{{Property}});
						FArrayReader Reader;
						Reader.Append(Expected);
						const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
						TMqttifyDisconnectPacket<EMqttifyProtocolVersion::Mqtt_5> DisconnectPacket{Reader, Header};

						TestPacketsEqual(TEXT("Packet should match expected"), DisconnectPacket, Expected, this);
					});
			}
		});

	Describe(
		"TMqttifyDisconnectPacket<EMqttifyProtocolVersion::Mqtt_3>",
		[this]
		{
			It(
				FString::Printf(TEXT("Should serialize")),
				[this]
				{
					TMqttifyDisconnectPacket<EMqttifyProtocolVersion::Mqtt_3_1_1> DisconnectPacket{};
					TestPacketsEqual(TEXT("Packet should match expected"), DisconnectPacket, ValidMqtt3Packet, this);
				});
		});
}

#endif // WITH_DEV_AUTOMATION_TESTS
