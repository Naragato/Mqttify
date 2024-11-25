#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Packets/MqttifyConnAckPacket.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Serialization/MemoryReader.h"
#include "Tests/Packets/PacketComparision.h"

using namespace Mqttify;

BEGIN_DEFINE_SPEC(
	MqttifyConnAckPacket,
	"Mqttify.Automation.MqttifyConnAckPacket",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ServerContext |
	EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProgramContext | EAutomationTestFlags::
	ProductFilter)

	static TArray<uint8> SerializeConnackMqtt3(const bool bSessionPresent, const EMqttifyConnectReturnCode ReturnCode)
	{
		const uint8 Flags = (bSessionPresent ? 0x01 : 0x00);
		TArray<uint8> ConnackPacket = {
			0x20,
			// Packet type (0x20 = CONNACK)
			0x02,
			// Remaining Length (2 bytes)
			Flags,
			// Session present flag
			static_cast<uint8>(ReturnCode) // Return code
		};
		return ConnackPacket;
	}

	const TArray<uint8> InvalidRemainingLengthConnackMqtt3Packet = {
		0x20,
		// Packet type (CONNACK)
		0x09,
		// Incorrect Remaining Length (it should be 2 bytes)
		0x09,
		// Invalid session present flag (should be either 0 or 1)
		0xF0 // Invalid return code
	};

	const TArray<uint8> InvalidFlagsConnackMqtt3Packet = {
		0x20,
		// Packet type (CONNACK)
		0x02,
		// Remaining Length (2 bytes)
		0x09,
		// Invalid session present flag (should be either 0 or 1)
		0x01 // RefusedProtocolVersion
	};

	const TArray<uint8> InvalidReturnCodeConnackMqtt3Packet = {
		0x20,
		// Packet type (CONNACK)
		0x02,
		// Remaining Length (2 bytes)
		0x09,
		// Invalid session present flag (should be either 0 or 1)
		0xF0 // Invalid return code
	};

	static TArray<uint8> SerializeConnackMqtt5(const bool bSessionPresent, EMqttifyReasonCode ReasonCode)
	{
		const uint8 Flags = (bSessionPresent ? 0x01 : 0x00);

		TArray<uint8> ConnackPacket = {
			0x20,
			// Packet type
			3,
			// Size of Flags and Reason Code plus 1 byte for Properties Length of 0
			Flags,
			// Session present flag
			static_cast<uint8>(ReasonCode),
			// Reason code
			0 // Properties Length
		};
		return ConnackPacket;
	}

	static TArray<uint8> SerializeConnackMqtt5(
		const bool bSessionPresent,
		EMqttifyReasonCode ReasonCode,
		const FMqttifyProperties& Properties
		)
	{
		TArray<uint8> ConnackPacket = {
			0x20,
			// Packet type
		};

		Data::EmplaceVariableByteInteger(2 + Properties.GetLength(true), ConnackPacket); // Remaining Length
		const uint8 Flags = (bSessionPresent ? 0x01 : 0x00);
		ConnackPacket.Emplace(Flags);
		ConnackPacket.Emplace(static_cast<uint8>(ReasonCode));
		Data::EmplaceVariableByteInteger(Properties.GetLength(false), ConnackPacket);

		for (const FMqttifyProperty& Property : Properties.GetProperties())
		{
			ConnackPacket.Add(static_cast<uint8>(Property.GetIdentifier()));

			if (Property.GetIdentifier() == EMqttifyPropertyIdentifier::SubscriptionIdentifier)
			{
				uint32 Value;
				Property.TryGetValue(Value);

				Data::EmplaceVariableByteInteger(Value, ConnackPacket);
				continue;
			}

			uint8 Value;
			if (Property.TryGetValue(Value))
			{
				ConnackPacket.Add(Value);
				continue;
			}

			uint16 Value16;
			if (Property.TryGetValue(Value16))
			{
				ConnackPacket.Add(Value16 >> 8);
				ConnackPacket.Add(Value16 & 0xFF);
				continue;
			}

			uint32 Value32;
			if (Property.TryGetValue(Value32))
			{
				ConnackPacket.Add(Value32 >> 24);
				ConnackPacket.Add((Value32 >> 16) & 0xFF);
				ConnackPacket.Add((Value32 >> 8) & 0xFF);
				ConnackPacket.Add(Value32 & 0xFF);
				continue;
			}

			FString ValueString;
			if (Property.TryGetValue(ValueString))
			{
				const uint16 StringLength = ValueString.Len();
				ConnackPacket.Add(StringLength >> 8);
				ConnackPacket.Add(StringLength & 0xFF);
				const auto String = StringCast<UTF8CHAR>(*ValueString);
				ConnackPacket.Append(reinterpret_cast<const uint8*>(String.Get()), StringLength);
			}

			TTuple<FString, FString> ValueTuple;
			if (Property.TryGetValue(ValueTuple))
			{
				const uint16 StringLength = ValueTuple.Key.Len();
				ConnackPacket.Add(StringLength >> 8);
				ConnackPacket.Add(StringLength & 0xFF);
				const auto String = StringCast<UTF8CHAR>(*ValueTuple.Key);
				ConnackPacket.Append(reinterpret_cast<const uint8*>(String.Get()), StringLength);

				const uint16 StringLength2 = ValueTuple.Value.Len();
				ConnackPacket.Add(StringLength2 >> 8);
				ConnackPacket.Add(StringLength2 & 0xFF);
				const auto String2 = StringCast<UTF8CHAR>(*ValueTuple.Value);
				ConnackPacket.Append(reinterpret_cast<const uint8*>(String2.Get()), StringLength2);
			}

			TArray<uint8> ValueArray;
			if (Property.TryGetValue(ValueArray))
			{
				const uint16 ArrayLen = static_cast<uint16>(ValueArray.Num());
				ConnackPacket.Add(ArrayLen >> 8);
				ConnackPacket.Add(ArrayLen & 0xFF);
				ConnackPacket.Append(ValueArray);
			}
		}

		return ConnackPacket;
	}

	TSet<EMqttifyReasonCode> ValidReasonCodes = {
		EMqttifyReasonCode::Success,
		EMqttifyReasonCode::UnspecifiedError,
		EMqttifyReasonCode::MalformedPacket,
		EMqttifyReasonCode::ProtocolError,
		EMqttifyReasonCode::ImplementationSpecificError,
		EMqttifyReasonCode::UnsupportedProtocolVersion,
		EMqttifyReasonCode::ClientIdentifierNotValid,
		EMqttifyReasonCode::BadUsernameOrPassword,
		EMqttifyReasonCode::NotAuthorized,
		EMqttifyReasonCode::ServerUnavailable,
		EMqttifyReasonCode::ServerBusy,
		EMqttifyReasonCode::Banned,
		EMqttifyReasonCode::BadAuthenticationMethod,
		EMqttifyReasonCode::TopicNameInvalid,
		EMqttifyReasonCode::PacketTooLarge,
		EMqttifyReasonCode::QuotaExceeded,
		EMqttifyReasonCode::PayloadFormatInvalid,
		EMqttifyReasonCode::RetainNotSupported,
		EMqttifyReasonCode::QoSNotSupported,
		EMqttifyReasonCode::UseAnotherServer,
		EMqttifyReasonCode::ServerMoved,
		EMqttifyReasonCode::ConnectionRateExceeded
	};

	TSet<EMqttifyReasonCode> InvalidReasonCodes = {
		EMqttifyReasonCode::GrantedQualityOfService1,
		EMqttifyReasonCode::GrantedQualityOfService2,
		EMqttifyReasonCode::ContinueAuthentication,
		EMqttifyReasonCode::ReAuthenticate,
		EMqttifyReasonCode::DisconnectWithWillMessage,
		EMqttifyReasonCode::NoMatchingSubscribers,
		EMqttifyReasonCode::NoSubscriptionExisted,
		EMqttifyReasonCode::SessionTakenOver,
		EMqttifyReasonCode::TopicFilterInvalid,
		EMqttifyReasonCode::SubscriptionIdentifiersNotSupported,
		EMqttifyReasonCode::WildcardSubscriptionsNotSupported,
		EMqttifyReasonCode::SharedSubscriptionsNotSupported,
		EMqttifyReasonCode::TopicAliasInvalid,
		EMqttifyReasonCode::PacketIdentifierInUse,
		EMqttifyReasonCode::PacketIdentifierNotFound,
		EMqttifyReasonCode::ReceiveMaximumExceeded,
		EMqttifyReasonCode::MaximumConnectTime
	};

	TArray<FMqttifyProperty> ValidProperties = {
		FMqttifyProperty::Create<
			EMqttifyPropertyIdentifier::UserProperty>(MakeTuple(FString(TEXT("test")), FString(TEXT("test")))),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::SessionExpiryInterval>(static_cast<uint32>(30)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::ReceiveMaximum>(static_cast<uint16>(30)),
		FMqttifyProperty::Create<
			EMqttifyPropertyIdentifier::MaximumQoS>(static_cast<uint8>(EMqttifyQualityOfService::AtLeastOnce)),
		FMqttifyProperty::Create<
			EMqttifyPropertyIdentifier::MaximumQoS>(static_cast<uint8>(EMqttifyQualityOfService::AtMostOnce)),
		FMqttifyProperty::Create<
			EMqttifyPropertyIdentifier::MaximumQoS>(static_cast<uint8>(EMqttifyQualityOfService::ExactlyOnce)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::RetainAvailable>(static_cast<uint8>(true)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::RetainAvailable>(static_cast<uint8>(false)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::MaximumPacketSize>(static_cast<uint32>(30)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::AssignedClientIdentifier>(FString(TEXT("test"))),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::TopicAliasMaximum>(static_cast<uint16>(30)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::ReasonString>(FString(TEXT("test"))),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::WildcardSubscriptionAvailable>(static_cast<uint8>(true)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::WildcardSubscriptionAvailable>(static_cast<uint8>(false)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::SubscriptionIdentifierAvailable>(static_cast<uint8>(true)),
		FMqttifyProperty::Create<
			EMqttifyPropertyIdentifier::SubscriptionIdentifierAvailable>(static_cast<uint8>(false)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::SharedSubscriptionAvailable>(static_cast<uint8>(true)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::SharedSubscriptionAvailable>(static_cast<uint8>(false)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::ServerKeepAlive>(static_cast<uint16>(30)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::ResponseInformation>(FString(TEXT("test"))),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::ServerReference>(FString(TEXT("test"))),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::AuthenticationMethod>(FString(TEXT("test"))),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::AuthenticationData>(TArray<uint8>{0, 1, 2, 3, 4}),
	};

	TArray<FMqttifyProperty> InvalidProperties = {
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::PayloadFormatIndicator>(static_cast<uint8>(true)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::PayloadFormatIndicator>(static_cast<uint8>(false)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::MessageExpiryInterval>(static_cast<uint32>(30)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::ContentType>(FString(TEXT("test"))),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::ResponseTopic>(FString(TEXT("test"))),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::CorrelationData>(TArray<uint8>{0, 1, 2, 3, 4}),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::SubscriptionIdentifier>(static_cast<uint32>(30)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::SessionExpiryInterval>(static_cast<uint32>(0)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::AssignedClientIdentifier>(FString(TEXT("test"))),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::ServerKeepAlive>(static_cast<uint16>(0)),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::AuthenticationMethod>(FString(TEXT("test"))),
		FMqttifyProperty::Create<EMqttifyPropertyIdentifier::AuthenticationData>(TArray<uint8>{})
	};

END_DEFINE_SPEC(MqttifyConnAckPacket)

void MqttifyConnAckPacket::Define()
{
	Describe(
		"TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_5>",
		[this]
		{
			for (const bool bIsSessionPresent : {false, true})
			{
				for (const EMqttifyReasonCode ReasonCode : ValidReasonCodes)
				{
					It(
						FString::Printf(
							TEXT("Should serialize with: session present %s, reason code %s"),
							bIsSessionPresent ? TEXT("true") : TEXT("false"),
							EnumToTCharString(ReasonCode)),
						[this, ReasonCode, bIsSessionPresent]
						{
							const TArray<uint8> Expected = SerializeConnackMqtt5(bIsSessionPresent, ReasonCode);
							TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_5> ConnAckPacket{
								bIsSessionPresent,
								ReasonCode
							};

							TestPacketsEqual(TEXT("Packet should match expected"), ConnAckPacket, Expected, this);
						});

					It(
						FString::Printf(
							TEXT("Should deserialize with: session present %s, reason code %s"),
							bIsSessionPresent ? TEXT("true") : TEXT("false"),
							EnumToTCharString(ReasonCode)),
						[this, ReasonCode, bIsSessionPresent]
						{
							const TArray<uint8> Expected = SerializeConnackMqtt5(bIsSessionPresent, ReasonCode);
							FArrayReader Reader;
							Reader.Append(Expected);
							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_5> ConnAckPacket{Reader, Header};

							TestPacketsEqual(TEXT("Packet should match expected"), ConnAckPacket, Expected, this);
						});
				}
			}

			for (const bool bIsSessionPresent : {false, true})
			{
				for (const EMqttifyReasonCode ReasonCode : InvalidReasonCodes)
				{
					It(
						FString::Printf(
							TEXT(
								"Should log error when serializing with session present %s, and invalid reason code %s"),
							bIsSessionPresent ? TEXT("true") : TEXT("false"),
							EnumToTCharString(ReasonCode)),
						[this, ReasonCode, bIsSessionPresent]
						{
							AddExpectedError(
								MqttifyReasonCode::InvalidReasonCode,
								EAutomationExpectedMessageFlags::Contains);
							TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_5> ConnAckPacket{
								bIsSessionPresent,
								ReasonCode
							};
						});

					It(
						FString::Printf(
							TEXT(
								"Should log error when deserializing with session present %s, and invalid reason code %s"),
							bIsSessionPresent ? TEXT("true") : TEXT("false"),
							EnumToTCharString(ReasonCode)),
						[this, ReasonCode, bIsSessionPresent]
						{
							AddExpectedError(
								MqttifyReasonCode::InvalidReasonCode,
								EAutomationExpectedMessageFlags::Contains);
							const TArray<uint8> Expected = SerializeConnackMqtt5(bIsSessionPresent, ReasonCode);
							FArrayReader Reader;
							Reader.Append(Expected);
							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_5> ConnAckPacket{Reader, Header};
						});
				}
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
						const TArray<uint8> Expected = SerializeConnackMqtt5(
							true,
							EMqttifyReasonCode::Banned,
							FMqttifyProperties{{Property}});

						TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_5> ConnAckPacket{
							true,
							EMqttifyReasonCode::Banned,
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
						const TArray<uint8> Expected = SerializeConnackMqtt5(
							true,
							EMqttifyReasonCode::Banned,
							FMqttifyProperties{{Property}});
						FArrayReader Reader;
						Reader.Append(Expected);
						const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
						TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_5> ConnAckPacket{Reader, Header};

						TestPacketsEqual(TEXT("Packet should match expected"), ConnAckPacket, Expected, this);
					});
			}
		});

	Describe(
		"TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_3>",
		[this]
		{
			for (const bool bIsSessionPresent : {false, true})
			{
				for (uint8 Index = 1; Index < 5; ++Index)
				{
					const EMqttifyConnectReturnCode ReturnCode = static_cast<EMqttifyConnectReturnCode>(Index);
					It(
						FString::Printf(
							TEXT("Should serialize with: Session present %s, Return code %s"),
							bIsSessionPresent ? TEXT("true") : TEXT("false"),
							EnumToTCharString(ReturnCode)),
						[this, ReturnCode, bIsSessionPresent]
						{
							const TArray<uint8> Expected = SerializeConnackMqtt3(bIsSessionPresent, ReturnCode);
							TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1> ConnAckPacket{
								bIsSessionPresent,
								ReturnCode
							};

							TestPacketsEqual(TEXT("Packet should match expected"), ConnAckPacket, Expected, this);
						});
				}
			}
		});
}
#endif // WITH_DEV_AUTOMATION_TESTS
