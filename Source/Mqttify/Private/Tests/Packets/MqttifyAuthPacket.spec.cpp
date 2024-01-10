#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Mqtt/MqttifyErrorStrings.h"
#include "Packets/MqttifyAuthPacket.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Serialization/MemoryReader.h"
#include "Tests/Packets/PacketComparision.h"

namespace Mqttify
{
	BEGIN_DEFINE_SPEC(MqttifyAuthPacket,
					"Mqttify.Automation.MqttifyAuthPacket",
					EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
		const TArray<uint8> Mqtt5ValidAuth = {
			// Fixed header
			0xF0, // Packet type (0xF0 = AUTH)
			0x02, // Remaining Length (2 bytes)

			// Variable header
			0x18, // Auth Reason Code (Continue authentication) (1 byte)
			0x00, // Properties Length (0 byte)

		};

		const TArray<uint8> Mqtt5InvalidAuth = {
			// Fixed header
			0xF0, // Packet type (0xF0 = AUTH)
			0x15, // Remaining Length (incorrect, should be 2 bytes)

			// Variable header
			0x00, // Auth Reason Code (Success) (1 byte)
			0x00, // Properties Length (0 byte)
		};

		const TArray<uint8> Mqtt5ValidAuthWithProperties = {
			// Fixed header
			0xF0, // Packet type (0xF0 = AUTH)
			0x0F, // Remaining Length (15 bytes)

			// Variable header
			0x18, // Auth Reason Code (Continue authentication) (1 byte)
			0x0D, // Properties Length (13 bytes)

			// Property: User Property (0x26), name and value 'test'
			0x26,
			0x00, 0x04, 't', 'e', 's', 't',
			0x00, 0x04, 't', 'e', 's', 't'
		};

		const TArray<uint8> Mqtt5InvalidAuthWithProperties = {
			// Fixed header
			0xF0, // Packet type (0xF0 = AUTH)
			0x0F, // Remaining Length (15 bytes)

			// Variable header
			0x00, // Auth Reason Code (Success) (1 byte)
			0x0D, // Properties Length (13 bytes)

			// Invalid Property: Invalid Property identifier (0x00), name and value 'test'
			0x00,
			0x00, 0x04, 't', 'e', 's', 't',
			0x00, 0x04, 't', 'e', 's', 't'
		};


		const TArray<uint8> Mqtt5ValidSerializedAuthWithPropertiesResult = {
			// Fixed header
			0xF0, // Packet type (0xF0 = AUTH)
			0x00, // Remaining Length (0 bytes)
		};

	END_DEFINE_SPEC(MqttifyAuthPacket)

	void MqttifyAuthPacket::Define()
	{
		Describe("TMqttifyAuthPacket<EMqttifyProtocolVersion::Mqtt_5>",
				[this] {
					It("Should serialize without properties for success packets",
						[this] {
							const TTuple<FString, FString> PropertyData = MakeTuple(TEXT("test"), TEXT("test"));
							const FMqttifyProperties Properties{
								{
									FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(
										PropertyData)
								}
							};

							FMqttifyAuthPacket Packet(EMqttifyReasonCode::Success, Properties);

							TestPacketsEqual(
								TEXT("Packet should match expected"),
								Packet,
								Mqtt5ValidSerializedAuthWithPropertiesResult,
								this);
						});

					It("Should serialize with properties for non success packets",
						[this] {
							const TTuple<FString, FString> PropertyData = MakeTuple(TEXT("test"), TEXT("test"));
							const FMqttifyProperties Properties{
								{
									FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(
										PropertyData)
								}
							};

							FMqttifyAuthPacket Packet(EMqttifyReasonCode::ContinueAuthentication, Properties);

							TestPacketsEqual(
								TEXT("Packet should match expected"),
								Packet,
								Mqtt5ValidAuthWithProperties,
								this);
						});

					It("Should deserialize with properties success packets",
						[this] {
							FArrayReader Reader;
							Reader.Append(Mqtt5ValidAuthWithProperties);

							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							FMqttifyAuthPacket Packet(Reader, Header);

							TestPacketsEqual(
								TEXT("Packet should match expected"),
								Packet,
								Mqtt5ValidAuthWithProperties,
								this);
						});

					It("Should return invalid packet and log error when remaining length is incorrect",
						[this] {
							FArrayReader Reader;
							Reader.Append(Mqtt5InvalidAuth);
							AddExpectedError(TEXT("Invalid packet size"), EAutomationExpectedErrorFlags::Contains);
							AddExpectedError(TEXT("Invalid packet type"), EAutomationExpectedErrorFlags::Contains);
							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							FMqttifyAuthPacket Packet(Reader, Header);
						});

					It("Should return invalid packet and log error when properties are incorrect",
						[this] {
							FArrayReader Reader;
							Reader.Append(Mqtt5InvalidAuthWithProperties);
							AddExpectedError(FMqttifyProperty::FailedToReadPropertyError,
											EAutomationExpectedErrorFlags::Contains,
											2);
							AddExpectedError(ErrorStrings::ArchiveTooSmall,
											EAutomationExpectedErrorFlags::Contains);

							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							const FMqttifyAuthPacket Packet(Reader, Header);
							TestFalse(TEXT("Packet should be invalid"), Packet.IsValid());
						});
				});
	}
} // namespace Mqttify
#endif // WITH_DEV_AUTOMATION_TESTS
