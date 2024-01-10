#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Mqtt/MqttifyQualityOfService.h"
#include "Packets/MqttifyConnectPacket.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Serialization/MemoryReader.h"
#include "Tests/Packets/PacketComparision.h"

namespace Mqttify
{
	BEGIN_DEFINE_SPEC(MqttifyConnectPacketSpec,
					"Mqttify.Automation.MqttifyConnectPacket",
					EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

		const TArray<uint8> Mqtt3BasicWithUsernamePassword = {
			// Fixed header
			0x10, // Packet type (0x10 = CONNECT)
			0x26, // Remaining length (38 bytes)

			// Variable header
			0x00, 0x04, 'M', 'Q', 'T', 'T', // Protocol name (6 bytes)
			0x04,                           // Protocol version (1 byte)
			0xC2,                           // Connect flags (1 byte)
			0x00, 0x3C,                     // Keep alive timeout (60 s) (2 bytes)

			// Payload
			0x00, 0x06, 'c', 'l', 'i', 'e', 'n', 't',           // Client ID (8 bytes)
			0x00, 0x08, 'u', 's', 'e', 'r', 'n', 'a', 'm', 'e', // Username (10 bytes)
			0x00, 0x08, 'p', 'a', 's', 's', 'w', 'o', 'r', 'd'  // Password (10 bytes)
		};

		const TArray<uint8> Mqtt5BasicWithUsernamePassword = {
			// Fixed header
			0x10, // Packet type (0x10 = CONNECT)
			0x27, // Remaining length (39 bytes)

			// Variable header
			0x00, 0x04, 'M', 'Q', 'T', 'T', // Protocol name (6 bytes)
			0x05,                           // Protocol version (1 byte)
			0xC2,                           // Connect flags (1 byte)
			0x00, 0x3C,                     // Keep alive timeout (60 s) (2 bytes)
			0x00,                           // Properties length (1 byte)

			// Payload
			0x00, 0x06, 'c', 'l', 'i', 'e', 'n', 't',           // Client ID (8 bytes)
			0x00, 0x08, 'u', 's', 'e', 'r', 'n', 'a', 'm', 'e', // Username (10 bytes)
			0x00, 0x08, 'p', 'a', 's', 's', 'w', 'o', 'r', 'd'  // Password (10 bytes)
		};

		const TArray<uint8> Mqtt3WithWill = {
			// Fixed header
			0x10, // Packet type (0x10 = CONNECT)
			0x21, // Remaining length (33 bytes)

			// Variable header
			0x00, 0x04, 'M', 'Q', 'T', 'T', // Protocol name (6 bytes)
			0x04,                           // Protocol version (1 byte)
			0x2E,                           // Connect flags (1 byte)
			0x00, 0x3C,                     // Keep alive timeout (60 s) (2 bytes)

			// Payload
			0x00, 0x06, 'c', 'l', 'i', 'e', 'n', 't',     // Client ID (8 bytes)
			0x00, 0x04, 'w', 'i', 'l', 'l',               // Will topic (6 bytes)
			0x00, 0x07, 'm', 'e', 's', 's', 'a', 'g', 'e' // Will message (9 bytes)
		};

		const TArray<uint8> Mqtt5WithWill = {
			// Fixed header
			0x10, // Packet type (0x10 = CONNECT)
			0x23, // Remaining length (35 bytes)

			// Variable header
			0x00, 0x04, 'M', 'Q', 'T', 'T', // Protocol name (6 bytes)
			0x05,                           // Protocol version (1 byte)
			0x2E,                           // Connect flags (1 byte)
			0x00, 0x3C,                     // Keep alive timeout (60 s) (2 bytes)
			0x00,                           // Properties length (1 byte)

			// Payload
			0x00, 0x06, 'c', 'l', 'i', 'e', 'n', 't',     // Client ID (8 bytes)
			0x00,                                         // Will properties length (1 byte)
			0x00, 0x04, 'w', 'i', 'l', 'l',               // Will topic (6 bytes)
			0x00, 0x07, 'm', 'e', 's', 's', 'a', 'g', 'e' // Will message (9 bytes)
		};

		const TArray<uint8> Mqtt5WithoutWill = {
			// Fixed header
			0x10, // Packet type (0x10 = CONNECT)
			0x13, // Remaining length (19 bytes)

			// Variable header
			0x00, 0x04, 'M', 'Q', 'T', 'T', // Protocol name (6 bytes)
			0x05,                           // Protocol version (1 byte)
			0x02,                           // Connect flags (1 byte, Clean session enabled)
			0x00, 0x3C,                     // Keep alive timeout (60 s) (2 bytes)
			0x00,                           // Properties length (0 bytes)

			// Payload
			0x00, 0x06, 'c', 'l', 'i', 'e', 'n', 't' // Client ID (8 bytes)
		};

		const TArray<uint8> Mqtt3WithoutWill = {
			// Fixed header
			0x10, // Packet type (0x10 = CONNECT)
			0x12, // Remaining length (18 bytes)

			// Variable header
			0x00, 0x04, 'M', 'Q', 'T', 'T', // Protocol name (6 bytes)
			0x04,                           // Protocol version (1 byte)
			0x02,                           // Connect flags (1 byte, Clean session enabled)
			0x00, 0x3C,                     // Keep alive timeout (60 s) (2 bytes)

			// Payload
			0x00, 0x06, 'c', 'l', 'i', 'e', 'n', 't' // Client ID (8 bytes)
		};

		const TArray<uint8> Mqtt5WithCleanSession = {
			// Fixed header
			0x10, // Packet type (0x10 = CONNECT)
			0x13, // Remaining length (19 bytes)

			// Variable header
			0x00, 0x04, 'M', 'Q', 'T', 'T', // Protocol name (6 bytes)
			0x05,                           // Protocol version (1 byte)
			0x02,                           // Connect flags (1 byte, Clean session enabled)
			0x00, 0x3C,                     // Keep alive timeout (60 s) (2 bytes)
			0x00,                           // Properties length (1 bytes)

			// Payload
			0x00, 0x06, 'c', 'l', 'i', 'e', 'n', 't' // Client ID (8 bytes)
		};

		const TArray<uint8> Mqtt3WithCleanSession = {
			// Fixed header
			0x10, // Packet type (0x10 = CONNECT)
			0x12, // Remaining length (18 bytes)

			// Variable header
			0x00, 0x04, 'M', 'Q', 'T', 'T', // Protocol name (6 bytes)
			0x04,                           // Protocol version (1 byte)
			0x02,                           // Connect flags (1 byte, Clean session enabled)
			0x00, 0x3C,                     // Keep alive timeout (60 s) (2 bytes)

			// Payload
			0x00, 0x06, 'c', 'l', 'i', 'e', 'n', 't' // Client ID (8 bytes)
		};

		const TArray<uint8> Mqtt3WithoutCleanSession = {
			// Fixed header
			0x10, // Packet type (0x10 = CONNECT)
			0x12, // Remaining length (18 bytes)

			// Variable header
			0x00, 0x04, 'M', 'Q', 'T', 'T', // Protocol name (6 bytes)
			0x04,                           // Protocol version (1 byte)
			0x00,                           // Connect flags (1 byte, all flags disabled)
			0x00, 0x3C,                     // Keep alive timeout (60 s) (2 bytes)

			// Payload
			0x00, 0x06, 'c', 'l', 'i', 'e', 'n', 't' // Client ID (8 bytes)
		};

		const TArray<uint8> Mqtt5WithoutCleanSession = {
			// Fixed header
			0x10, // Packet type (0x10 = CONNECT)
			0x13, // Remaining length (19 bytes)

			// Variable header
			0x00, 0x04, 'M', 'Q', 'T', 'T', // Protocol name (6 bytes)
			0x05,                           // Protocol version (1 byte)
			0x00,                           // Connect flags (1 byte, all flags disabled)
			0x00, 0x3C,                     // Keep alive timeout (60 s) (2 bytes)
			0x00,                           // Properties length (1 bytes)

			// Payload
			0x00, 0x06, 'c', 'l', 'i', 'e', 'n', 't' // Client ID (8 bytes)
		};

		const TArray<uint8> Mqtt3WithKeepAlive = {
			// Fixed header
			0x10, // Packet type (0x10 = CONNECT)
			0x12, // Remaining length (18 bytes)

			// Variable header
			0x00, 0x04, 'M', 'Q', 'T', 'T', // Protocol name (6 bytes)
			0x04,                           // Protocol version (1 byte)
			0x02,                           // Connect flags (1 byte, Clean session enabled)
			0x00, 0x0A,                     // Keep alive timeout (10 s) (2 bytes)

			// Payload
			0x00, 0x06, 'c', 'l', 'i', 'e', 'n', 't' // Client ID (8 bytes)
		};

		const TArray<uint8> Mqtt5WithKeepAlive = {
			// Fixed header
			0x10, // Packet type (0x10 = CONNECT)
			0x13, // Remaining length (19 bytes)

			// Variable header
			0x00, 0x04, 'M', 'Q', 'T', 'T', // Protocol name (6 bytes)
			0x05,                           // Protocol version (1 byte)
			0x02,                           // Connect flags (1 byte, Clean session enabled)
			0x00, 0x0A,                     // Keep alive timeout (10 s) (2 bytes)
			0x00,                           // Properties length (1 bytes)

			// Payload
			0x00, 0x06, 'c', 'l', 'i', 'e', 'n', 't' // Client ID (8 bytes)
		};

		const TArray<uint8> WithWillProperties = {
			// Fixed header
			0x10, // Packet type (0x10 = CONNECT)
			0x2F, // Remaining length (47 bytes)

			// Variable header
			0x00, 0x04, 'M', 'Q', 'T', 'T', // Protocol name (6 bytes)
			0x05,                           // Protocol version (1 byte)
			0x2C,                           // Connect flags (1 byte)
			// Flags:
			// - Will flag (1)
			// - Will QoS (1)
			// - Will retain (1)
			// - Clean session (0)
			0x00, 0x3C, // Keep alive timeout (60 s) (2 bytes)
			0x00,       // Properties length (1 bytes)


			// Payload
			0x00, 0x06, 'c', 'l', 'i', 'e', 'n', 't',     // Client ID (8 bytes)
			0x0C,                                         // Will properties length (12 bytes)
			0x18, 0x00, 0x00, 0x00, 0x0A,                 // Will delay interval (10 s) (5 bytes)
			0x02, 0x00, 0x00, 0x00, 0x64,                 // Message expiry interval (100 s) (5 bytes)
			0x01, 0x01,                                   // Payload format indicator (2 bytes)
			0x00, 0x04, 'w', 'i', 'l', 'l',               // Will topic (6 bytes)
			0x00, 0x07, 'm', 'e', 's', 's', 'a', 'g', 'e' // Will message (9 bytes)
		};

		const TArray<uint8> WithProperties = {
			// Fixed header
			0x10, // Packet type (0x10 = CONNECT)
			0x1b, // Remaining length (27 bytes)

			// Variable header
			0x00, 0x04, 'M', 'Q', 'T', 'T', // Protocol name
			0x05,                           // Protocol version
			0x02,                           // Connect flags (Clean session enabled, all other flags disabled)
			0x00, 0x3C,                     // Keep alive timeout (60 s)
			0x08,                           // Properties length (8 bytes)
			0x11, 0x00, 0x00, 0x00, 0x3C,   // Session Expiry Interval (60 s)
			0x21, 0x00, 0x00,               // Receive Maximum (0)

			// Payload
			0x00, 0x06, 'c', 'l', 'i', 'e', 'n', 't', // Client ID
		};


	END_DEFINE_SPEC(MqttifyConnectPacketSpec)

	void MqttifyConnectPacketSpec::Define()
	{
		Describe("TMqttifyConnectPacket<EMqttifyProtocolVersion::Mqtt_5>",
				[this] {
					It("Should serialize with properties",
						[this] {
							const FMqttifyProperties Properties{
								{
									FMqttifyProperty::Create<
										EMqttifyPropertyIdentifier::SessionExpiryInterval>(static_cast<uint32>(60)),
									FMqttifyProperty::Create<EMqttifyPropertyIdentifier::ReceiveMaximum>(
										static_cast<uint16>(0))
								}
							};

							FMqttifyConnectPacket5 Packet("client",
														60,
														"",
														"",
														true,
														false,
														"",
														"",
														EMqttifyQualityOfService::AtMostOnce,
														FMqttifyProperties{},
														Properties);

							TestPacketsEqual(TEXT("Packet should match expected"), Packet, WithProperties, this);
						});

					It("Should serialize with will properties",
						[this] {
							const FMqttifyProperties Properties{
								{
									FMqttifyProperty::Create<
										EMqttifyPropertyIdentifier::WillDelayInterval>(static_cast<uint32>(10)),
									FMqttifyProperty::Create<
										EMqttifyPropertyIdentifier::MessageExpiryInterval>(static_cast<uint32>(100)),
									FMqttifyProperty::Create<
										EMqttifyPropertyIdentifier::PayloadFormatIndicator>(
										static_cast<uint8>(1))
								}
							};

							FMqttifyConnectPacket5 Packet("client",
														60,
														"",
														"",
														false,
														true,
														"will",
														"message",
														EMqttifyQualityOfService::AtLeastOnce,
														Properties,
														FMqttifyProperties{});

							TestPacketsEqual(TEXT("Packet should match expected"), Packet, WithWillProperties, this);
						});
					It("Should properly deserialize basic packet with username and password",
						[this] {
							FArrayReader Reader;
							Reader.Append(Mqtt5BasicWithUsernamePassword);

							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							TestTrue(TEXT("Packet should be a connect packet"),
									Header.GetPacketType() == EMqttifyPacketType::Connect);
							FMqttifyConnectPacket5 Packet(Reader, Header);

							TestEqual(TEXT("Username should be 'username'"), Packet.GetUserName(), "username");

							TestEqual(TEXT("Password should be 'password'"), Packet.GetPassword(), "password");

							TestEqual(TEXT(
								"Remaining length should equal the packet length minus the fixed header length"),
									Packet.GetLength(),
									Mqtt5BasicWithUsernamePassword.Num() - 2);
						});

					It("Should properly handle packets with will",
						[this] {
							FArrayReader Reader;
							Reader.Append(Mqtt5WithWill);

							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							TestTrue(TEXT("Packet should be a connect packet"),
									Header.GetPacketType() == EMqttifyPacketType::Connect);
							FMqttifyConnectPacket5 Packet(Reader, Header);

							TestEqual(TEXT("Will topic should be 'will'"), Packet.GetWillTopic(), "will");
							TestEqual(TEXT("Will message should be 'message'"), Packet.GetWillMessage(), "message");
							TestEqual(TEXT("Will QoS should be AtLeastOnce"),
									Packet.GetWillQoS(),
									EMqttifyQualityOfService::AtLeastOnce);
							TestEqual(TEXT("Client Id should be 'client'"), Packet.GetClientId(), TEXT("client"));
							TestEqual(TEXT(
								"Remaining length should equal the packet length minus the fixed header length"),
									Packet.GetLength(),
									Mqtt5WithWill.Num() - 2);
						});

					It("Should properly handle packets without will",
						[this] {
							FArrayReader Reader;
							Reader.Append(Mqtt5WithoutWill);

							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							TestTrue(TEXT("Packet should be a connect packet"),
									Header.GetPacketType() == EMqttifyPacketType::Connect);
							FMqttifyConnectPacket5 Packet(Reader, Header);

							TestEqual(TEXT("Client Id should be 'client'"), Packet.GetClientId(), TEXT("client"));
							TestEqual(TEXT("Will topic should be empty"), Packet.GetWillTopic(), TEXT(""));
							TestEqual(TEXT("Will message should be empty"), Packet.GetWillMessage(), TEXT(""));
							TestEqual(TEXT("Will QoS should be AtMostOnce"),
									Packet.GetWillQoS(),
									EMqttifyQualityOfService::AtMostOnce);
							TestEqual(TEXT(
								"Remaining length should equal the packet length minus the fixed header length"),
									Packet.GetLength(),
									Mqtt5WithoutWill.Num() - 2);
						});

					It("Should properly handle clean session flag",
						[this] {
							FArrayReader Reader;
							Reader.Append(Mqtt5WithCleanSession);

							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							TestTrue(TEXT("Packet should be a connect packet"),
									Header.GetPacketType() == EMqttifyPacketType::Connect);
							FMqttifyConnectPacket5 Packet(Reader, Header);

							TestTrue(TEXT("Clean session flag should be set"), Packet.GetCleanSession());
							TestEqual(TEXT(
								"Remaining length should equal the packet length minus the fixed header length"),
									Packet.GetLength(),
									Mqtt5WithCleanSession.Num() - 2);
						});

					It("Should properly handle when clean session flag is not set",
						[this] {
							FArrayReader Reader;
							Reader.Append(Mqtt5WithoutCleanSession);

							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							TestTrue(TEXT("Packet should be a connect packet"),
									Header.GetPacketType() == EMqttifyPacketType::Connect);
							FMqttifyConnectPacket5 Packet(Reader, Header);

							TestFalse(TEXT("Clean session flag should not be set"), Packet.GetCleanSession());
							TestEqual(TEXT(
								"Remaining length should equal the packet length minus the fixed header length"),
									Packet.GetLength(),
									Mqtt5WithoutCleanSession.Num() - 2);
						});

					It("Should properly handle varying keep-alive values",
						[this] {
							FArrayReader Reader;
							Reader.Append(Mqtt5WithKeepAlive);

							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							TestTrue(TEXT("Packet should be a connect packet"),
									Header.GetPacketType() == EMqttifyPacketType::Connect);
							FMqttifyConnectPacket5 Packet(Reader, Header);

							TestEqual(TEXT("Keep alive should be 10"), Packet.GetKeepAliveSeconds(), 10);
							TestEqual(TEXT(
								"Remaining length should equal the packet length minus the fixed header length"),
									Packet.GetLength(),
									Mqtt5WithKeepAlive.Num() - 2);
						});

					It("Should properly handle will properties",
						[this] {
							FArrayReader Reader;
							Reader.Append(WithWillProperties);

							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							TestTrue(TEXT("Packet should be a connect packet"),
									Header.GetPacketType() == EMqttifyPacketType::Connect);
							FMqttifyConnectPacket5 Packet(Reader, Header);

							TestEqual(TEXT("Will topic should be 'will'"), Packet.GetWillTopic(), "will");
							TestEqual(TEXT("Will message should be 'message'"), Packet.GetWillMessage(), "message");
							TestEqual(TEXT("Will QoS should be AtLeastOnce"),
									Packet.GetWillQoS(),
									EMqttifyQualityOfService::AtLeastOnce);
							TestEqual(TEXT("Client Id should be 'client'"), Packet.GetClientId(), TEXT("client"));

							TestTrue(TEXT("Will properties should contain at least one property"),
									Packet.GetWillProperties().GetProperties().Num() > 1);

							TArray Properties = Packet.GetWillProperties().GetProperties();
							TestTrue(TEXT("Should contain format indicator property"),
									Properties.ContainsByPredicate([](const FMqttifyProperty& Property) {
										uint32 DelayInterval = 0;
										return Property.GetIdentifier() ==
											EMqttifyPropertyIdentifier::WillDelayInterval &&
											Property.TryGetValue(DelayInterval) && DelayInterval == 10;
									}));

							TestEqual(TEXT(
								"Remaining length should equal the packet length minus the fixed header length"),
									Packet.GetLength(),
									WithWillProperties.Num() - 2);

						});

					It("Should properly handle properties",
						[this] {
							FArrayReader Reader;
							Reader.Append(WithProperties);

							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							TestTrue(TEXT("Packet should be a connect packet"),
									Header.GetPacketType() == EMqttifyPacketType::Connect);
							FMqttifyConnectPacket5 Packet(Reader, Header);

							TestTrue(TEXT("Properties should contain at least one property"),
									Packet.GetProperties().GetProperties().Num() > 1);

							TArray Properties = Packet.GetProperties().GetProperties();
							TestTrue(TEXT("Should contain format indicator property"),
									Properties.ContainsByPredicate([](const FMqttifyProperty& Property) {
										return Property.GetIdentifier() ==
											EMqttifyPropertyIdentifier::SessionExpiryInterval;
									}));

							TestEqual(TEXT(
								"Remaining length should equal the packet length minus the fixed header length"),
									Packet.GetLength(),
									WithProperties.Num() - 2);
						});
				});

		Describe("TMqttifyConnectPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>",
				[this] {

					It("Should properly serialize basic packet with username and password",
						[this] {
							FMqttifyConnectPacket3 Packet{ "client", 60, "username", "password", true };
							TestPacketsEqual(
								TEXT("FMqttifyConnectPacket3 with username and password"),
								Packet,
								Mqtt3BasicWithUsernamePassword,
								this);
						});

					It("Should properly deserialize basic packet with username and password",
						[this] {
							FArrayReader Reader;
							Reader.Append(Mqtt3BasicWithUsernamePassword);

							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							TestTrue(TEXT("Packet should be a connect packet"),
									Header.GetPacketType() == EMqttifyPacketType::Connect);
							FMqttifyConnectPacket3 Packet(Reader, Header);

							TestEqual(TEXT("Username should be 'username'"), Packet.GetUserName(), "username");

							TestEqual(TEXT("Password should be 'password'"), Packet.GetPassword(), "password");

							TestEqual(TEXT(
								"Remaining length should equal the packet length minus the fixed header length"),
									Packet.GetLength(),
									Mqtt3BasicWithUsernamePassword.Num() - 2);
						});

					It("Should properly serialize packets with will",
						[this] {
							FMqttifyConnectPacket3 Packet{
								TEXT("client"),
								60, {},
								{}, true,
								true,
								TEXT("will"),
								TEXT("message"),
								EMqttifyQualityOfService::AtLeastOnce };
							TestPacketsEqual(
								TEXT("FMqttifyConnectPacket3 with will"),
								Packet,
								Mqtt3WithWill,
								this);
						});

					It("Should properly handle packets with will",
						[this] {
							FArrayReader Reader;
							Reader.Append(Mqtt3WithWill);

							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							TestTrue(TEXT("Packet should be a connect packet"),
									Header.GetPacketType() == EMqttifyPacketType::Connect);
							FMqttifyConnectPacket3 Packet(Reader, Header);

							TestEqual(TEXT("Will topic should be 'will'"), Packet.GetWillTopic(), "will");
							TestEqual(TEXT("Will message should be 'message'"), Packet.GetWillMessage(), "message");
							TestEqual(TEXT("Will QoS should be AtLeastOnce"),
									Packet.GetWillQoS(),
									EMqttifyQualityOfService::AtLeastOnce);
							TestEqual(TEXT("Client Id should be 'client'"), Packet.GetClientId(), TEXT("client"));
							TestEqual(TEXT(
								"Remaining length should equal the packet length minus the fixed header length"),
									Packet.GetLength(),
									Mqtt3WithWill.Num() - 2);
						});

					It("Should properly serialize packets without will",
						[this] {
							FMqttifyConnectPacket3 Packet{
								TEXT("client"),
								60, {},
								{}, true,
								false };
							TestPacketsEqual(
								TEXT("FMqttifyConnectPacket3 without will"),
								Packet,
								Mqtt3WithoutWill,
								this);
						});

					It("Should properly handle packets without will",
						[this] {
							FArrayReader Reader;
							Reader.Append(Mqtt3WithoutWill);

							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							TestTrue(TEXT("Packet should be a connect packet"),
									Header.GetPacketType() == EMqttifyPacketType::Connect);
							FMqttifyConnectPacket3 Packet(Reader, Header);

							TestEqual(TEXT("Client Id should be 'client'"), Packet.GetClientId(), TEXT("client"));
							TestEqual(TEXT("Will topic should be empty"), Packet.GetWillTopic(), TEXT(""));
							TestEqual(TEXT("Will message should be empty"), Packet.GetWillMessage(), TEXT(""));
							TestEqual(TEXT("Will QoS should be AtMostOnce"),
									Packet.GetWillQoS(),
									EMqttifyQualityOfService::AtMostOnce);
							TestEqual(TEXT(
								"Remaining length should equal the packet length minus the fixed header length"),
									Packet.GetLength(),
									Mqtt3WithoutWill.Num() - 2);
						});

					It("Should properly serialize clean session flag",
						[this] {
							FMqttifyConnectPacket3 Packet{
								TEXT("client"),
								60, {},
								{}, true,
								false };
							TestPacketsEqual(
								TEXT("FMqttifyConnectPacket3 with clean session"),
								Packet,
								Mqtt3WithCleanSession,
								this);
						});

					It("Should properly handle clean session flag",
						[this] {
							FArrayReader Reader;
							Reader.Append(Mqtt3WithCleanSession);

							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							TestTrue(TEXT("Packet should be a connect packet"),
									Header.GetPacketType() == EMqttifyPacketType::Connect);
							FMqttifyConnectPacket3 Packet(Reader, Header);

							TestTrue(TEXT("Clean session flag should be set"), Packet.GetCleanSession());
							TestEqual(TEXT(
								"Remaining length should equal the packet length minus the fixed header length"),
									Packet.GetLength(),
									Mqtt3WithCleanSession.Num() - 2);
						});

					It("Should properly serialize when clean session flag is not set",
						[this] {
							FMqttifyConnectPacket3 Packet{
								TEXT("client"),
								60, {},
								{}, false,
								false };
							TestPacketsEqual(
								TEXT("FMqttifyConnectPacket3 without clean session"),
								Packet,
								Mqtt3WithoutCleanSession,
								this);
						});

					It("Should properly handle when clean session flag is not set",
						[this] {
							FArrayReader Reader;
							Reader.Append(Mqtt3WithoutCleanSession);

							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							TestTrue(TEXT("Packet should be a connect packet"),
									Header.GetPacketType() == EMqttifyPacketType::Connect);
							FMqttifyConnectPacket3 Packet(Reader, Header);

							TestFalse(TEXT("Clean session flag should not be set"), Packet.GetCleanSession());
							TestEqual(TEXT(
								"Remaining length should equal the packet length minus the fixed header length"),
									Packet.GetLength(),
									Mqtt3WithoutCleanSession.Num() - 2);
						});

					It("Should properly serialize varying keep-alive values",
						[this] {
							FMqttifyConnectPacket3 Packet{
								TEXT("client"),
								10, {},
								{}, true,
								false };
							TestPacketsEqual(
								TEXT("FMqttifyConnectPacket3 10 second keepalive"),
								Packet,
								Mqtt3WithKeepAlive,
								this);
						});

					It("Should properly handle varying keep-alive values",
						[this] {
							FArrayReader Reader;
							Reader.Append(Mqtt3WithKeepAlive);

							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
							TestTrue(TEXT("Packet should be a connect packet"),
									Header.GetPacketType() == EMqttifyPacketType::Connect);
							FMqttifyConnectPacket3 Packet(Reader, Header);

							TestEqual(TEXT("Keep alive should be 10"), Packet.GetKeepAliveSeconds(), 10);
							TestEqual(TEXT(
								"Remaining length should equal the packet length minus the fixed header length"),
									Packet.GetLength(),
									Mqtt3WithKeepAlive.Num() - 2);
						});
				});
	}
} // namespace Mqttify
#endif // WITH_DEV_AUTOMATION_TESTS
