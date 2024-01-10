#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Packets/MqttifyPubRecPacket.h"
#include "Tests/Packets/PacketComparision.h"

namespace Mqttify
{
	BEGIN_DEFINE_SPEC(
		MqttifyPubRecPacket,
		"Mqttify.Automation.MqttifyPubRecPacket",
		EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

		static TArray<uint8> GetMqtt5PubRecWithProperties(const EMqttifyReasonCode InReasonCode)
		{
			return {
				0x50, // Packet type (0x50 = PubRec)
				0x11, // Remaining Length (variable due to properties)

				// Variable header
				0x00, // Message ID MSB
				0x01, // Message ID LSB

				// Reason Code
				static_cast<uint8>(InReasonCode),

				// Properties Length
				0x0D, // Properties Length (13 bytes)

				// Property: User Property (0x26), name and value 'test'
				0x26,
				0x00, 0x04, 't', 'e', 's', 't',
				0x00, 0x04, 't', 'e', 's', 't'
			};
		}

		const TArray<uint8> Mqtt3PubRec = {
			// Fixed header
			0x50, // Packet type (0x50)
			0x02, // Remaining Length (2 bytes for message ID)

			// Variable header
			0x00, // Message ID MSB
			0x01 // Message ID LSB
		};

		const TArray<uint8> Mqtt5PubRecNoProperties = {
			// Fixed header
			0x50, // Packet type (0x50)
			0x02, // Remaining Length (2 bytes for message ID)

			// Variable header
			0x00, // Message ID MSB
			0x01, // Message ID LSB

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
			EMqttifyReasonCode::PayloadFormatInvalid
		};

	END_DEFINE_SPEC(MqttifyPubRecPacket)

	void MqttifyPubRecPacket::Define()
	{
		Describe("TMqttifyPubRecPacket<EMqttifyProtocolVersion::Mqtt_5>",
				[this]
				{
					It(
						"Should serialize without properties",
						[this]
						{
							auto PubRecPacket = TMqttifyPubRecPacket<EMqttifyProtocolVersion::Mqtt_5>{
								1, EMqttifyReasonCode::Success
							};

							TestPacketsEqual(
								TEXT("Packet should match expected"),
								PubRecPacket,
								Mqtt5PubRecNoProperties,
								this);
						});

					It("Should serialize success with properties",
						[this]
						{
							const TTuple<FString, FString> PropertyData = MakeTuple(TEXT("test"), TEXT("test"));
							const FMqttifyProperties Properties{
								{
									FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(
										PropertyData)
								}
							};
							auto PubRecPacket = TMqttifyPubRecPacket<EMqttifyProtocolVersion::Mqtt_5>{
								1,
								EMqttifyReasonCode::Success,
								Properties
							};

							TestPacketsEqual(
								TEXT("Packet should match expected"),
								PubRecPacket,
								Mqtt5PubRecNoProperties,
								// When serializing success, properties are ignored in our implementation to keep the packet size small
								this);
						});

					for (const EMqttifyReasonCode ReasonCode : ReasonCodes)
					{
						It(FString::Printf(TEXT("Should serialize with %s with properties"),
											EnumToTCharString<EMqttifyReasonCode>(ReasonCode)),
							[this, ReasonCode]
							{
								const TTuple<FString, FString> PropertyData = MakeTuple(TEXT("test"), TEXT("test"));
								const FMqttifyProperties Properties{
									{
										FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(
											PropertyData)
									}
								};
								auto PubRecPacket = TMqttifyPubRecPacket<EMqttifyProtocolVersion::Mqtt_5>{
									1,
									ReasonCode,
									Properties
								};

								TestPacketsEqual(
									TEXT("Packet should match expected"),
									PubRecPacket,
									GetMqtt5PubRecWithProperties(ReasonCode),
									this);
							});

						It(FString::Printf(TEXT("Should deserialize with %s with properties"),
											EnumToTCharString<EMqttifyReasonCode>(ReasonCode)),
							[this, ReasonCode]
							{
								FArrayReader Reader;
								Reader.Append(GetMqtt5PubRecWithProperties(ReasonCode));

								const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
								const TMqttifyPubRecPacket<EMqttifyProtocolVersion::Mqtt_5>
									PubRecPacket(Reader, Header);
								const TTuple<FString, FString> PropertyData = MakeTuple(TEXT("test"), TEXT("test"));
								const FMqttifyProperties Properties{
									{
										FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(
											PropertyData)
									}
								};
								TestEqual(TEXT("Packet Identifier should be equal"),
										PubRecPacket.GetPacketIdentifier(),
										1);

								TestEqual(TEXT("Reason code should be equal"),
										PubRecPacket.GetReasonCode(),
										ReasonCode);

								TestEqual(TEXT("Properties should be equal"),
										PubRecPacket.GetProperties(),
										Properties);
							});
					}
				});

		Describe("TMqttifyPubRecPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>",
				[this]
				{
					It("Should serialize success",
						[this]
						{
							auto PubRecPacket = TMqttifyPubRecPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>{1};

							TestPacketsEqual(
								TEXT("Packet should match expected"),
								PubRecPacket,
								Mqtt3PubRec,
								this);
						});

					It("Should Deserialize success",
						[this]
						{
							FArrayReader Reader;
							Reader.Append(Mqtt3PubRec);

							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							const TMqttifyPubRecPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>
								PubRecPacket(Reader, Header);

							TestEqual(TEXT("Packet Identifier should be equal"),
									PubRecPacket.GetPacketIdentifier(),
									1);
						});
				});
	}
}

#endif // WITH_AUTOMATION_TESTS
