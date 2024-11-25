#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Packets/MqttifyUnsubAckPacket.h"
#include "Tests/Packets/PacketComparision.h"

using namespace Mqttify;
BEGIN_DEFINE_SPEC(
	MqttifyUnsubAckPacket,
	"Mqttify.Automation.MqttifyUnsubAckPacket",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ServerContext |
	EAutomationTestFlags::CommandletContext  | EAutomationTestFlags::
	ProductFilter)

	static TArray<uint8> GetMqtt5UnsubAckWithProperties(const EMqttifyReasonCode InReasonCode)
	{
		TArray<uint8> UnsubAckPacket = {
			0xB0,
			// Packet type (0xB0 = UNSUBACK)
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
		return UnsubAckPacket;
	}

	const TArray<uint8> Mqtt3UnsubAck = {
		// Fixed header
		0xB0,
		// Packet type (0xB0 = UNSUBACK)
		0x02,
		// Remaining Length (2 bytes for message ID, 1 byte for return code)

		// Variable header
		0x00,
		// Message ID MSB
		0x01,
		// Message ID LSB

	};

	const TArray<uint8> Mqtt5UnsubAckNoProperties = {
		// Fixed header
		0xB0,
		// Packet type (0xB0 = UNSUBACK)
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
		EMqttifyReasonCode::Success,
		EMqttifyReasonCode::NoSubscriptionExisted,
		EMqttifyReasonCode::UnspecifiedError,
		EMqttifyReasonCode::ImplementationSpecificError,
		EMqttifyReasonCode::NotAuthorized,
		EMqttifyReasonCode::TopicFilterInvalid,
		EMqttifyReasonCode::PacketIdentifierInUse
	};

END_DEFINE_SPEC(MqttifyUnsubAckPacket)

void MqttifyUnsubAckPacket::Define()
{
	Describe(
		"TMqttifyUnsubAckPacket<EMqttifyProtocolVersion::Mqtt_5>",
		[this]
		{
			It(
				"Should serialize without properties",
				[this]
				{
					auto UnsubAckPacket = TMqttifyUnsubAckPacket<EMqttifyProtocolVersion::Mqtt_5>{
						1,
						{EMqttifyReasonCode::Success}
					};

					TestPacketsEqual(
						TEXT("Packet should match expected"),
						UnsubAckPacket,
						Mqtt5UnsubAckNoProperties,
						this);
				});

			It(
				"Should serialize success with properties",
				[this]
				{
					const TTuple<FString, FString> PropertyData = MakeTuple(TEXT("test"), TEXT("test"));
					const FMqttifyProperties Properties{
						{FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(PropertyData)}
					};
					auto UnsubAckPacket = TMqttifyUnsubAckPacket<EMqttifyProtocolVersion::Mqtt_5>{
						1,
						{EMqttifyReasonCode::Success},
						Properties
					};

					TestPacketsEqual(
						TEXT("Packet should match expected"),
						UnsubAckPacket,
						GetMqtt5UnsubAckWithProperties(EMqttifyReasonCode::Success),
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
						auto UnsubAckPacket = TMqttifyUnsubAckPacket<EMqttifyProtocolVersion::Mqtt_5>{
							1,
							{ReasonCode},
							Properties
						};

						TestPacketsEqual(
							TEXT("Packet should match expected"),
							UnsubAckPacket,
							GetMqtt5UnsubAckWithProperties(ReasonCode),
							this);
					});

				It(
					FString::Printf(
						TEXT("Should deserialize with %s with properties"),
						EnumToTCharString<EMqttifyReasonCode>(ReasonCode)),
					[this, ReasonCode]
					{
						FArrayReader Reader;
						Reader.Append(GetMqtt5UnsubAckWithProperties(ReasonCode));

						const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
						const TMqttifyUnsubAckPacket<EMqttifyProtocolVersion::Mqtt_5> UnsubAckPacket(Reader, Header);
						const TTuple<FString, FString> PropertyData = MakeTuple(TEXT("test"), TEXT("test"));
						const FMqttifyProperties Properties{
							{FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(PropertyData)}
						};
						TestEqual(TEXT("Packet Identifier should be equal"), UnsubAckPacket.GetPacketId(), 1);

						TestEqual(TEXT("Reason codes should be equal"), UnsubAckPacket.GetReasonCodes(), {ReasonCode});

						TestEqual(TEXT("Properties should be equal"), UnsubAckPacket.GetProperties(), Properties);
					});
			}
		});

	Describe(
		"TMqttifyUnsubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>",
		[this]
		{
			It(
				"Should serialize success",
				[this]
				{
					auto UnsubAckPacket = TMqttifyUnsubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>{1};

					TestPacketsEqual(TEXT("Packet should match expected"), UnsubAckPacket, Mqtt3UnsubAck, this);
				});

			It(
				"Should Deserialize success",
				[this]
				{
					FArrayReader Reader;
					Reader.Append(Mqtt3UnsubAck);

					const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
					const TMqttifyUnsubAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1> UnsubAckPacket(Reader, Header);

					TestEqual(TEXT("Packet Identifier should be equal"), UnsubAckPacket.GetPacketId(), 1);
				});
		});
}

#endif // WITH_DEV_AUTOMATION_TESTS
