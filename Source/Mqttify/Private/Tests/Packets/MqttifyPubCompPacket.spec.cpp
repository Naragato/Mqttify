#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Packets/MqttifyPubCompPacket.h"
#include "Tests/Packets/PacketComparision.h"

using namespace Mqttify;
BEGIN_DEFINE_SPEC(
	MqttifyPubCompPacket,
	"Mqttify.Automation.MqttifyPubCompPacket",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

	static TArray<uint8> GetMqtt5PubCompWithProperties(const EMqttifyReasonCode InReasonCode)
	{
		TArray<uint8> PubCompPacket = {
			0x70,
			// Packet type (0x70)
			0x11,
			// Remaining Length (variable due to properties)

			// Variable header
			0x00,
			// Message ID MSB
			0x01,
			// Message ID LSB

			// Reason Code
			static_cast<uint8>(InReasonCode),

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
			't'
		};
		return PubCompPacket;
	}

	const TArray<uint8> Mqtt3PubComp = {
		// Fixed header
		0x70,
		// Packet type (0x70)
		0x02,
		// Remaining Length (2 bytes for message ID)

		// Variable header
		0x00,
		// Message ID MSB
		0x01 // Message ID LSB
	};

	const TArray<uint8> Mqtt5PubCompNoProperties = {
		// Fixed header
		0x70,
		// Packet type (0x70)
		0x02,
		// Remaining Length (2 bytes for message ID)

		// Variable header
		0x00,
		// Message ID MSB
		0x01,
		// Message ID LSB

		// No properties
	};

	const TArray<EMqttifyReasonCode> ReasonCodes = {EMqttifyReasonCode::PacketIdentifierNotFound,};

END_DEFINE_SPEC(MqttifyPubCompPacket)

void MqttifyPubCompPacket::Define()
{
	Describe(
		"TMqttifyPubCompPacket<EMqttifyProtocolVersion::Mqtt_5>",
		[this]
		{
			It(
				"Should serialize without properties",
				[this]
				{
					auto PubCompPacket = TMqttifyPubCompPacket<EMqttifyProtocolVersion::Mqtt_5>{1, EMqttifyReasonCode::Success};

					TestPacketsEqual(TEXT("Packet should match expected"), PubCompPacket, Mqtt5PubCompNoProperties, this);
				});

			It(
				"Should serialize success with properties",
				[this]
				{
					const TTuple<FString, FString> PropertyData = MakeTuple(TEXT("test"), TEXT("test"));
					const FMqttifyProperties Properties{{FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(PropertyData)}};
					auto PubCompPacket = TMqttifyPubCompPacket<EMqttifyProtocolVersion::Mqtt_5>{1, EMqttifyReasonCode::Success, Properties};

					TestPacketsEqual(
						TEXT("Packet should match expected"),
						PubCompPacket,
						Mqtt5PubCompNoProperties,
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
						auto PubCompPacket = TMqttifyPubCompPacket<EMqttifyProtocolVersion::Mqtt_5>{1, ReasonCode, Properties};

						TestPacketsEqual(TEXT("Packet should match expected"), PubCompPacket, GetMqtt5PubCompWithProperties(ReasonCode), this);
					});

				It(
					FString::Printf(TEXT("Should deserialize with %s with properties"), EnumToTCharString<EMqttifyReasonCode>(ReasonCode)),
					[this, ReasonCode]
					{
						FArrayReader Reader;
						Reader.Append(GetMqtt5PubCompWithProperties(ReasonCode));

						const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
						const TMqttifyPubCompPacket<EMqttifyProtocolVersion::Mqtt_5> PubCompPacket(Reader, Header);
						const TTuple<FString, FString> PropertyData = MakeTuple(TEXT("test"), TEXT("test"));
						const FMqttifyProperties Properties{{FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(PropertyData)}};
						TestEqual(TEXT("Packet Identifier should be equal"), PubCompPacket.GetPacketId(), 1);

						TestEqual(TEXT("Reason code should be equal"), PubCompPacket.GetReasonCode(), ReasonCode);

						TestEqual(TEXT("Properties should be equal"), PubCompPacket.GetProperties(), Properties);
					});
			}
		});

	Describe(
		"TMqttifyPubCompPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>",
		[this]
		{
			It(
				"Should serialize success",
				[this]
				{
					auto PubCompPacket = TMqttifyPubCompPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>{1};

					TestPacketsEqual(TEXT("Packet should match expected"), PubCompPacket, Mqtt3PubComp, this);
				});

			It(
				"Should Deserialize success",
				[this]
				{
					FArrayReader Reader;
					Reader.Append(Mqtt3PubComp);

					const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
					const TMqttifyPubCompPacket<EMqttifyProtocolVersion::Mqtt_3_1_1> PubCompPacket(Reader, Header);

					TestEqual(TEXT("Packet Identifier should be equal"), PubCompPacket.GetPacketId(), 1);
				});
		});
}

#endif // WITH_DEV_AUTOMATION_TESTS
