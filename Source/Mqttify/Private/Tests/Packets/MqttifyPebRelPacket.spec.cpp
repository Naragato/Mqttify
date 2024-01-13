#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Packets/MqttifyPubRelPacket.h"
#include "Tests/Packets/PacketComparision.h"

namespace Mqttify
{

	BEGIN_DEFINE_SPEC(
		MqttifyPubRelPacket,
		"Mqttify.Automation.MqttifyPubRelPacket",
		EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

		const TArray<uint8> Mqtt3PubRel = {
			// Fixed header
			0x62, // Packet type (0x60) and Flags (0x02 for PubRel)
			0x02, // Remaining Length (2 bytes for message ID)

			// Variable header
			0x00, // Message ID MSB
			0x01  // Message ID LSB
		};

		const TArray<uint8> Mqtt5PubRelNoProperties = {
			// Fixed header
			0x62, // Packet type (0x60) and Flags (0x02 for PubRel)
			0x02, // Remaining Length (2 bytes for message ID)

			// Variable header
			0x00, // Message ID MSB
			0x01, // Message ID LSB

			// No properties
		};

		const TArray<uint8> Mqtt5PubRelSuccessWithProperties = {
			// Fixed header
			0x62, // Packet type (0x60) and Flags (0x02 for PubRel)
			0x11, // Remaining Length (variable due to properties)

			// Variable header
			0x00, // Message ID MSB
			0x01, // Message ID LSB

			// Reason Code: Success (0x00)
			0x00,

			// Properties Length
			0x0D, // Properties Length (13 bytes)

			// Property: User Property (0x26), name and value 'test'
			0x26,
			0x00, 0x04, 't', 'e', 's', 't',
			0x00, 0x04, 't', 'e', 's', 't'
		};

		const TArray<uint8> Mqtt5PubRelNotFoundWithProperties = {
			// Fixed header
			0x62, // Packet type (0x60) and Flags (0x02 for PubRel)
			0x11, // Remaining Length (variable due to properties)

			// Variable header
			0x00, // Message ID MSB
			0x01, // Message ID LSB

			// Reason Code: Packet Identifier not found (0x92)
			0x92,

			// Properties Length
			0x0D, // Properties Length (13 bytes)

			// Property: User Property (0x26), name and value 'test'
			0x26,
			0x00, 0x04, 't', 'e', 's', 't',
			0x00, 0x04, 't', 'e', 's', 't'
		};


	END_DEFINE_SPEC(MqttifyPubRelPacket)

	void MqttifyPubRelPacket::Define()
	{
		Describe("TMqttifyPebRelPacket<EMqttifyProtocolVersion::Mqtt_5>",
				[this] {
					It(
						"Should serialize without properties",
						[this] {

							auto PubRelPacket = TMqttifyPubRelPacket<EMqttifyProtocolVersion::Mqtt_5>{ 1 };

							TestPacketsEqual(
								TEXT("Packet should match expected"),
								PubRelPacket,
								Mqtt5PubRelNoProperties,
								this);
						});

					It("Should serialize success with properties",
						[this] {

							const TTuple<FString, FString> PropertyData = MakeTuple(TEXT("test"), TEXT("test"));
							const FMqttifyProperties Properties{
								{
									FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(
										PropertyData)
								}
							};
							auto PubRelPacket = TMqttifyPubRelPacket<EMqttifyProtocolVersion::Mqtt_5>{
								1,
								EMqttifyReasonCode::Success,
								Properties
							};

							TestPacketsEqual(
								TEXT("Packet should match expected"),
								PubRelPacket,
								Mqtt5PubRelNoProperties,
								// When serializing success, properties are ignored in our implementation to keep the packet size small
								this);
						});

					It("Should serialize not found with properties",
						[this] {

							const TTuple<FString, FString> PropertyData = MakeTuple(TEXT("test"), TEXT("test"));
							const FMqttifyProperties Properties{
								{
									FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(
										PropertyData)
								}
							};

							auto PubRelPacket = TMqttifyPubRelPacket<EMqttifyProtocolVersion::Mqtt_5>{
								1,
								EMqttifyReasonCode::PacketIdentifierNotFound,
								Properties
							};

							TestPacketsEqual(
								TEXT("Packet should match expected"),
								PubRelPacket,
								Mqtt5PubRelNotFoundWithProperties,
								this);
						});

					It("Should deserialize success with properties",
						[this] {

							FArrayReader Reader;
							Reader.Append(Mqtt5PubRelSuccessWithProperties);

							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							const TMqttifyPubRelPacket<EMqttifyProtocolVersion::Mqtt_5> PubRelPacket(Reader, Header);
							const TTuple<FString, FString> PropertyData = MakeTuple(TEXT("test"), TEXT("test"));
							const FMqttifyProperties Properties{
								{
									FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(
										PropertyData)
								}
							};
							TestEqual(TEXT("Should Contain Properties"),
									PubRelPacket.GetProperties().GetLength(),
									Properties.GetLength());

							TestEqual(TEXT("Properties should be equal"),
									PubRelPacket.GetProperties(),
									Properties);

							TestEqual(TEXT("Reason code should be equal"),
									PubRelPacket.GetReasonCode(),
									EMqttifyReasonCode::Success);
						});

					It("Should deserialize not found with properties",
						[this] {

							FArrayReader Reader;
							Reader.Append(Mqtt5PubRelNotFoundWithProperties);

							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							const TMqttifyPubRelPacket<EMqttifyProtocolVersion::Mqtt_5> PubRelPacket(Reader, Header);
							const TTuple<FString, FString> PropertyData = MakeTuple(TEXT("test"), TEXT("test"));
							const FMqttifyProperties Properties{
								{
									FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(
										PropertyData)
								}
							};
							TestEqual(TEXT("Should Contain Properties"),
									PubRelPacket.GetProperties().GetLength(),
									Properties.GetLength());

							TestEqual(TEXT("Properties should be equal"),
									PubRelPacket.GetProperties(),
									Properties);

							TestEqual(TEXT("Reason code should be equal"),
									PubRelPacket.GetReasonCode(),
									EMqttifyReasonCode::PacketIdentifierNotFound);
						});
				});

		Describe("TMqttifyPebRelPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>",
				[this] {
					It("Should serialize success",
						[this] {
							auto PubRelPacket = TMqttifyPubRelPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>{ 1 };

							TestPacketsEqual(
								TEXT("Packet should match expected"),
								PubRelPacket,
								Mqtt3PubRel,
								this);
						});

					It("Should Deserialize success",
						[this] {
							FArrayReader Reader;
							Reader.Append(Mqtt3PubRel);

							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							const TMqttifyPubRelPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>
								PubRelPacket(Reader, Header);

							TestEqual(TEXT("Packet Identifier should be equal"),
									PubRelPacket.GetPacketId(),
									1);
						});
				});
	}
}

#endif // WITH_AUTOMATION_TESTS
