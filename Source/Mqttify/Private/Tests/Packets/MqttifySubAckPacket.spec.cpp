#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Packets/MqttifySubAckPacket.h"
#include "Tests/Packets/PacketComparision.h"

using namespace Mqttify;
BEGIN_DEFINE_SPEC(
	MqttifySubAckPacket,
	"Mqttify.Automation.MqttifySubAckPacket",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

	static TArray<uint8> GetMqtt5SubAckWithProperties(const EMqttifyReasonCode InReasonCode)
	{
		TArray<uint8> SubAckPacket = {
			0x90,
			// Packet type (0x90 = SUBACK)
			0x11,
			// Remaining Length (variable due to properties)

			// Variable header
			0x00,
			// Message ID MSB
			0x01,
			// Message ID LSB

			// Properties Length
			0x0D,
			// Properties Length (13 bytes)

			// Property: User Property (0x26), name and value 'test'
			0x26,
			0x00,
			0x04,
			't',
			'e',
			's',
			't',
			0x00,
			0x04,
			't',
			'e',
			's',
			't',

			// Reason Code
			static_cast<uint8>(InReasonCode),
		};
		return SubAckPacket;
	}

	const TArray<uint8> Mqtt3SubAck = {
		// Fixed header
		0x90,
		// Packet type (0x90 = SUBACK)
		0x03,
		// Remaining Length (2 bytes for message ID, 1 byte for return code)

		// Variable header
		0x00,
		// Message ID MSB
		0x01,
		// Message ID LSB

		// Return Code
		0x00
	};

	const TArray<uint8> Mqtt5SubAckNoProperties = {
		// Fixed header
		0x90,
		// Packet type (0x90 = SUBACK)
		0x04,
		// Remaining Length (2 bytes for message ID)

		// Variable header
		0x00,
		// Message ID MSB
		0x01,
		// Message ID LSB

		// No properties so Properties Length is 0
		0x00,

		// Reason Code
		0x00
	};

	const TArray<EMqttifyReasonCode> ReasonCodes = {
		EMqttifyReasonCode::GrantedQualityOfService0,
		EMqttifyReasonCode::GrantedQualityOfService1,
		EMqttifyReasonCode::GrantedQualityOfService2,
		EMqttifyReasonCode::UnspecifiedError,
		EMqttifyReasonCode::ImplementationSpecificError,
		EMqttifyReasonCode::NotAuthorized,
		EMqttifyReasonCode::TopicFilterInvalid,
		EMqttifyReasonCode::PacketIdentifierInUse,
		EMqttifyReasonCode::QuotaExceeded,
		EMqttifyReasonCode::SharedSubscriptionsNotSupported,
		EMqttifyReasonCode::SubscriptionIdentifiersNotSupported,
		EMqttifyReasonCode::WildcardSubscriptionsNotSupported
	};

END_DEFINE_SPEC(MqttifySubAckPacket)

void MqttifySubAckPacket::Define()
{
	Describe(
		"TMqttifySubAckPacket<EMqttifyProtocolVersion::Mqtt_5>",
		[this]
		{
			It(
				"Should serialize without properties",
				[this]
				{
					auto SubAckPacket = TMqttifySubAckPacket<EMqttifyProtocolVersion::Mqtt_5>{1, {EMqttifyReasonCode::Success}};

					TestPacketsEqual(TEXT("Packet should match expected"), SubAckPacket, Mqtt5SubAckNoProperties, this);
				});

			It(
				"Should serialize success with properties",
				[this]
				{
					const TTuple<FString, FString> PropertyData = MakeTuple(TEXT("test"), TEXT("test"));
					const FMqttifyProperties Properties{{FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(PropertyData)}};
					auto SubAckPacket = TMqttifySubAckPacket<EMqttifyProtocolVersion::Mqtt_5>{1, {EMqttifyReasonCode::Success}, Properties};

					TestPacketsEqual(
						TEXT("Packet should match expected"),
						SubAckPacket,
						GetMqtt5SubAckWithProperties(EMqttifyReasonCode::Success),
						// When serializing success, properties are ignored in our implementation to keep the packet size small
						this);
				});

			for (const EMqttifyReasonCode ReasonCode : ReasonCodes)
			{
				It(
					FString::Printf(TEXT("Should serialize with %s with properties"), EnumToTCharString<EMqttifyReasonCode>(ReasonCode)),
					[this, ReasonCode]
					{
						const TTuple<FString, FString> PropertyData = MakeTuple(TEXT("test"), TEXT("test"));
						const FMqttifyProperties Properties{{FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(PropertyData)}};
						auto SubAckPacket = TMqttifySubAckPacket<EMqttifyProtocolVersion::Mqtt_5>{1, {ReasonCode}, Properties};

						TestPacketsEqual(TEXT("Packet should match expected"), SubAckPacket, GetMqtt5SubAckWithProperties(ReasonCode), this);
					});

				It(
					FString::Printf(TEXT("Should deserialize with %s with properties"), EnumToTCharString<EMqttifyReasonCode>(ReasonCode)),
					[this, ReasonCode]
					{
						FArrayReader Reader;
						Reader.Append(GetMqtt5SubAckWithProperties(ReasonCode));

						const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
						const TMqttifySubAckPacket<EMqttifyProtocolVersion::Mqtt_5> SubAckPacket(Reader, Header);
						const TTuple<FString, FString> PropertyData = MakeTuple(TEXT("test"), TEXT("test"));
						const FMqttifyProperties Properties{{FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(PropertyData)}};
						TestEqual(TEXT("Packet Identifier should be equal"), SubAckPacket.GetPacketId(), 1);

						TestEqual(TEXT("Reason codes should be equal"), SubAckPacket.GetReasonCodes(), {ReasonCode});

						TestEqual(TEXT("Properties should be equal"), SubAckPacket.GetProperties(), Properties);
					});
			}
		});

	Describe(
		"TMqttifySubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>",
		[this]
		{
			It(
				"Should serialize success",
				[this]
				{
					auto SubAckPacket = TMqttifySubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>{
						1,
						{EMqttifySubscribeReturnCode::SuccessQualityOfService0}
					};

					TestPacketsEqual(TEXT("Packet should match expected"), SubAckPacket, Mqtt3SubAck, this);
				});

			It(
				"Should Deserialize success",
				[this]
				{
					FArrayReader Reader;
					Reader.Append(Mqtt3SubAck);

					const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
					const TMqttifySubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1> SubAckPacket(Reader, Header);

					TestEqual(TEXT("Packet Identifier should be equal"), SubAckPacket.GetPacketId(), 1);
				});
		});
}

#endif // WITH_DEV_AUTOMATION_TESTS
