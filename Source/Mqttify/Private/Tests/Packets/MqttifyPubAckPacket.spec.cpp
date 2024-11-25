#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Packets/MqttifyPubAckPacket.h"
#include "Tests/Packets/PacketComparision.h"

using namespace Mqttify;
BEGIN_DEFINE_SPEC(
	MqttifyPubAckPacket,
	"Mqttify.Automation.MqttifyPubAckPacket",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ServerContext |
	EAutomationTestFlags::CommandletContext  | EAutomationTestFlags::
	ProductFilter)

	static TArray<uint8> GetMqtt5PubAckWithProperties(const EMqttifyReasonCode InReasonCode)
	{
		TArray<uint8> PubAckPacket = {
			0x40,
			// Packet type (0x40 = PUBACK)
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
		return PubAckPacket;
	}

	const TArray<uint8> Mqtt3PubAck = {
		// Fixed header
		0x40,
		// Packet type (0x40 = PUBACK)
		0x02,
		// Remaining Length (2 bytes for message ID)

		// Variable header
		0x00,
		// Message ID MSB
		0x01 // Message ID LSB
	};

	const TArray<uint8> Mqtt5PubAckNoProperties = {
		// Fixed header
		0x40,
		// Packet type (0x40 = PUBACK)
		0x02,
		// Remaining Length (2 bytes for message ID)

		// Variable header
		0x00,
		// Message ID MSB
		0x01,
		// Message ID LSB

		// No properties
	};

	const TArray<EMqttifyReasonCode> ReasonCodes = {
		EMqttifyReasonCode::NoMatchingSubscribers,
		EMqttifyReasonCode::UnspecifiedError,
		EMqttifyReasonCode::ImplementationSpecificError,
		EMqttifyReasonCode::NotAuthorized,
		EMqttifyReasonCode::TopicNameInvalid,
		EMqttifyReasonCode::PacketIdentifierInUse,
		EMqttifyReasonCode::QuotaExceeded,
		EMqttifyReasonCode::PayloadFormatInvalid,
	};

END_DEFINE_SPEC(MqttifyPubAckPacket)

void MqttifyPubAckPacket::Define()
{
	Describe(
		"TMqttifyPubAckPacket<EMqttifyProtocolVersion::Mqtt_5>",
		[this]
		{
			It(
				"Should serialize without properties",
				[this]
				{
					auto PubAckPacket = TMqttifyPubAckPacket<EMqttifyProtocolVersion::Mqtt_5>{1};

					TestPacketsEqual(TEXT("Packet should match expected"), PubAckPacket, Mqtt5PubAckNoProperties, this);
				});

			It(
				"Should serialize success with properties",
				[this]
				{
					const TTuple<FString, FString> PropertyData = MakeTuple(TEXT("test"), TEXT("test"));
					const FMqttifyProperties Properties{
						{FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(PropertyData)}
					};
					auto PubAckPacket = TMqttifyPubAckPacket<EMqttifyProtocolVersion::Mqtt_5>{
						1,
						EMqttifyReasonCode::Success,
						Properties
					};

					TestPacketsEqual(
						TEXT("Packet should match expected"),
						PubAckPacket,
						Mqtt5PubAckNoProperties,
						// When serializing success, properties are ignored in our implementation to keep the packet size small
						this);
				});

			for (const EMqttifyReasonCode ReasonCode : ReasonCodes)
			{
				It(
					FString::Printf(
						TEXT("Should serialize with %s with properties"),
						EnumToTCharString<EMqttifyReasonCode>(ReasonCode)),
					[this, ReasonCode]
					{
						const TTuple<FString, FString> PropertyData = MakeTuple(TEXT("test"), TEXT("test"));
						const FMqttifyProperties Properties{
							{FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(PropertyData)}
						};
						auto PubAckPacket = TMqttifyPubAckPacket<EMqttifyProtocolVersion::Mqtt_5>{
							1,
							ReasonCode,
							Properties
						};

						TestPacketsEqual(
							TEXT("Packet should match expected"),
							PubAckPacket,
							GetMqtt5PubAckWithProperties(ReasonCode),
							this);
					});

				It(
					FString::Printf(
						TEXT("Should deserialize with %s with properties"),
						EnumToTCharString<EMqttifyReasonCode>(ReasonCode)),
					[this, ReasonCode]
					{
						FArrayReader Reader;
						Reader.Append(GetMqtt5PubAckWithProperties(ReasonCode));

						const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
						const TMqttifyPubAckPacket<EMqttifyProtocolVersion::Mqtt_5> PubAckPacket(Reader, Header);
						const TTuple<FString, FString> PropertyData = MakeTuple(TEXT("test"), TEXT("test"));
						const FMqttifyProperties Properties{
							{FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(PropertyData)}
						};
						TestEqual(TEXT("Packet Identifier should be equal"), PubAckPacket.GetPacketId(), 1);

						TestEqual(TEXT("Reason code should be equal"), PubAckPacket.GetReasonCode(), ReasonCode);

						TestEqual(TEXT("Properties should be equal"), PubAckPacket.GetProperties(), Properties);
					});
			}
		});

	Describe(
		"TMqttifyPubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>",
		[this]
		{
			It(
				"Should serialize success",
				[this]
				{
					auto PubAckPacket = TMqttifyPubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>{1};

					TestPacketsEqual(TEXT("Packet should match expected"), PubAckPacket, Mqtt3PubAck, this);
				});

			It(
				"Should Deserialize success",
				[this]
				{
					FArrayReader Reader;
					Reader.Append(Mqtt3PubAck);

					const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
					const TMqttifyPubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1> PubAckPacket(Reader, Header);

					TestEqual(TEXT("Packet Identifier should be equal"), PubAckPacket.GetPacketId(), 1);
				});
		});
}

#endif // WITH_DEV_AUTOMATION_TESTS
