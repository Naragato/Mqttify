#include "Mqtt/MqttifyReasonCode.h"
#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Packets/MqttifyPublishPacket.h"
#include "Tests/Packets/PacketComparision.h"

namespace Mqttify
{
	BEGIN_DEFINE_SPEC(
		MqttifyPublishPacket,
		"Mqttify.Automation.MqttifyPublishPacket",
		EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)


		static TArray<uint8> GetMqtt5PublishPacketWithProperties(const bool bInIsDup,
																const EMqttifyQualityOfService InQoS,
																const bool InShouldRetain)
		{
			uint8 Flags = 0x30; // Packet type (0x30 = Publish)
			if (bInIsDup && InQoS != EMqttifyQualityOfService::AtMostOnce)
			{
				Flags |= 1 << 3;
			}
			Flags |= static_cast<uint8>(InQoS) << 1;
			if (InShouldRetain)
			{
				Flags |= 0x01;
			}

			// Topic name 'test'
			TArray<uint8> TopicName = {0x00, 0x04, 't', 'e', 's', 't'};

			// Message ID (only if QoS > 0)
			TArray<uint8> MessageID;
			if (static_cast<uint8>(InQoS) > 0)
			{
				MessageID = {0x00, 0x01};
			}

			// Properties: User Property (0x26), name and value 'info' and 'data'
			TArray<uint8> Properties = {
				0x26,
				0x00, 0x04, 'i', 'n', 'f', 'o',
				0x00, 0x04, 'd', 'a', 't', 'a'
			};

			// Payload 'payload'
			TArray<uint8> Payload = {'p', 'a', 'y', 'l', 'o', 'a', 'd'};

			// Calculate Remaining Length ... we're assuming that this fits in one byte for simplicity
			int RemainingLength = TopicName.Num()
				+ MessageID.Num()
				+ Properties.Num() + 1
				+ Payload.Num();

			checkf(RemainingLength < 256, TEXT("Remaining Length must fit in one byte"));

			// Construct the full packet
			TArray<uint8> PublishPacket;
			PublishPacket.Add(Flags); // Fixed header: Flags
			PublishPacket.Add(static_cast<uint8>(RemainingLength)); // Fixed header: Remaining Length
			PublishPacket.Append(TopicName); // Variable header: Topic Name
			PublishPacket.Append(MessageID); // Variable header: Message ID (only if QoS > 0)
			PublishPacket.Add(Properties.Num());
			PublishPacket.Append(Properties); // Variable header: Properties
			PublishPacket.Append(Payload); // Payload

			return PublishPacket;
		}

		static TArray<uint8> GetMqtt311PublishPacket(bool bInIsDup, EMqttifyQualityOfService InQoS, bool InShouldRetain)
		{
			uint8 Flags = 0x30; // Packet type (0x30 = Publish)
			if (bInIsDup && InQoS != EMqttifyQualityOfService::AtMostOnce)
			{
				Flags |= 1 << 3;
			}
			Flags |= static_cast<uint8>(InQoS) << 1;
			if (InShouldRetain)
			{
				Flags |= 0x01;
			}

			// Topic name 'test'
			TArray<uint8> TopicName = {0x00, 0x04, 't', 'e', 's', 't'};

			// Message ID (only if QoS > 0)
			TArray<uint8> MessageID;
			if (static_cast<uint8>(InQoS) > 0)
			{
				MessageID = {0x00, 0x01};
			}

			// Payload 'payload'
			TArray<uint8> Payload = {'p', 'a', 'y', 'l', 'o', 'a', 'd'};

			// Calculate Remaining Length ... we're assuming that this fits in one byte for simplicity
			int RemainingLength = TopicName.Num() + MessageID.Num() + Payload.Num();
			checkf(RemainingLength < 256, TEXT("Remaining Length must fit in one byte"));

			// Construct the full packet
			TArray<uint8> PublishPacket;
			PublishPacket.Add(Flags); // Fixed header: Flags
			PublishPacket.Add(RemainingLength); // Fixed header: Remaining Length
			PublishPacket.Append(TopicName); // Variable header: Topic Name
			PublishPacket.Append(MessageID); // Variable header: Message ID (only if QoS > 0)
			PublishPacket.Append(Payload); // Payload

			return PublishPacket;
		}

		TArray<TTuple<bool, EMqttifyQualityOfService, bool>> PacketTypes = {
			{true, EMqttifyQualityOfService::AtLeastOnce, true},
			{true, EMqttifyQualityOfService::AtLeastOnce, false},
			{true, EMqttifyQualityOfService::ExactlyOnce, true},
			{true, EMqttifyQualityOfService::ExactlyOnce, false},
			{false, EMqttifyQualityOfService::AtMostOnce, true},
			{false, EMqttifyQualityOfService::AtMostOnce, false},
			{false, EMqttifyQualityOfService::AtLeastOnce, true},
			{false, EMqttifyQualityOfService::AtLeastOnce, false},
			{false, EMqttifyQualityOfService::ExactlyOnce, true},
			{false, EMqttifyQualityOfService::ExactlyOnce, false},
		};

		static const TCHAR* BoolToTCharString(const bool bInBool)
		{
			if (bInBool)
				return TEXT("true");
			else
				return TEXT("false");
		}

	END_DEFINE_SPEC(MqttifyPublishPacket)

	void MqttifyPublishPacket::Define()
	{
		Describe("MQTT 5 Publish Packet",
				[this]
				{
					for (const TTuple<bool, EMqttifyQualityOfService, bool>& PacketType : PacketTypes)
					{
						const bool bIsDup = PacketType.Get<0>();
						const EMqttifyQualityOfService QoS = PacketType.Get<1>();
						const bool bShouldRetain = PacketType.Get<2>();

						It(FString::Printf(TEXT("Should serialize Duplicate %s, QoS %s, Retain %s with properties"),
											BoolToTCharString(bIsDup),
											EnumToTCharString<EMqttifyQualityOfService>(QoS),
											BoolToTCharString(bShouldRetain)),
							[this, bIsDup, QoS, bShouldRetain]
							{
								const TTuple<FString, FString> PropertyData = MakeTuple(TEXT("info"), TEXT("data"));
								const FMqttifyProperties Properties{
									{
										FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(
											PropertyData)
									}
								};

								FMqttifyMessage Message = FMqttifyMessage(
									TEXT("test"),
									{'p', 'a', 'y', 'l', 'o', 'a', 'd'},
									bShouldRetain,
									QoS
								);

								auto PublishPacketRef = MakeShared<TMqttifyPublishPacket<
									EMqttifyProtocolVersion::Mqtt_5>>(MoveTemp(Message), 1, Properties);

								if (bIsDup)
								{
									PublishPacketRef = TMqttifyPublishPacket<
										EMqttifyProtocolVersion::Mqtt_5>::GetDuplicate(MoveTemp(PublishPacketRef));
								}

								const TArray<uint8> PublishPacketBytes = GetMqtt5PublishPacketWithProperties(
									bIsDup,
									QoS,
									bShouldRetain);
								TestPacketsEqual(
									TEXT("Packet should match expected"),
									*PublishPacketRef,
									PublishPacketBytes,
									this);
							});

						It(FString::Printf(
								TEXT("Should deserialize with QoS:%s, Duplicate: %s, Retain: %s with properties"),
								EnumToTCharString<EMqttifyQualityOfService>(QoS),
								BoolToTCharString(bIsDup),
								BoolToTCharString(bShouldRetain)),
							[this, QoS, bIsDup, bShouldRetain]
							{
								FArrayReader Reader;
								Reader.Append(GetMqtt5PublishPacketWithProperties(bIsDup, QoS, bShouldRetain));

								const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
								const FMqttifyPublishPacket5
									PublishPacket(Reader, Header);
								const TTuple<FString, FString> PropertyData = MakeTuple(TEXT("info"), TEXT("data"));
								const FMqttifyProperties Properties{
									{
										FMqttifyProperty::Create<EMqttifyPropertyIdentifier::UserProperty>(
											PropertyData)
									}
								};
								TestEqual(TEXT("Packet Identifier should be equal"),
										PublishPacket.GetPacketIdentifier(),
										QoS == EMqttifyQualityOfService::AtMostOnce ? 0 : 1);

								TestEqual(TEXT("Is Duplicate should be equal"),
										PublishPacket.GetIsDuplicate(),
										bIsDup);

								TestEqual(TEXT("QoS should be equal"),
										PublishPacket.GetQualityOfService(),
										QoS);

								TestEqual(TEXT("Retain should be equal"),
										PublishPacket.GetShouldRetain(),
										bShouldRetain);

								TestEqual(TEXT("Properties should be equal"),
										PublishPacket.GetProperties(),
										Properties);
							});
					}
				});

		Describe("MQTT 3.1.1 Publish Packet",
				[this]
				{
					for (const TTuple<bool, EMqttifyQualityOfService, bool>& PacketType : PacketTypes)
					{
						const bool bIsDup = PacketType.Get<0>();
						const EMqttifyQualityOfService QoS = PacketType.Get<1>();
						const bool bShouldRetain = PacketType.Get<2>();

						It(FString::Printf(TEXT("Should serialize Duplicate %s, QoS %s, Retain %s"),
											BoolToTCharString(bIsDup),
											EnumToTCharString<EMqttifyQualityOfService>(QoS),
											BoolToTCharString(bShouldRetain)),
							[this, bIsDup, QoS, bShouldRetain]
							{
								FMqttifyMessage Message = FMqttifyMessage(
									TEXT("test"),
									{'p', 'a', 'y', 'l', 'o', 'a', 'd'},
									bShouldRetain,
									QoS
								);

								auto PublishPacketRef = MakeShared<FMqttifyPublishPacket3>(MoveTemp(Message), 1);

								if (bIsDup)
								{
									PublishPacketRef = FMqttifyPublishPacket3::GetDuplicate(MoveTemp(PublishPacketRef));
								}

								const TArray<uint8> PublishPacketBytes = GetMqtt311PublishPacket(
									bIsDup,
									QoS,
									bShouldRetain);
								TestPacketsEqual(
									TEXT("Packet should match expected"),
									*PublishPacketRef,
									PublishPacketBytes,
									this);
							});

						It(FString::Printf(
								TEXT("Should deserialize with QoS:%s, Duplicate: %s, Retain: %s"),
								EnumToTCharString<EMqttifyQualityOfService>(QoS),
								BoolToTCharString(bIsDup),
								BoolToTCharString(bShouldRetain)),
							[this, QoS, bIsDup, bShouldRetain]
							{
								FArrayReader Reader;
								Reader.Append(GetMqtt311PublishPacket(bIsDup, QoS, bShouldRetain));

								const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
								const FMqttifyPublishPacket3
									PublishPacket(Reader, Header);

								TestEqual(TEXT("Packet Identifier should be equal"),
										PublishPacket.GetPacketIdentifier(),
										QoS == EMqttifyQualityOfService::AtMostOnce ? 0 : 1);

								TestEqual(TEXT("Is Duplicate should be equal"),
										PublishPacket.GetIsDuplicate(),
										bIsDup);

								TestEqual(TEXT("QoS should be equal"),
										PublishPacket.GetQualityOfService(),
										QoS);

								TestEqual(TEXT("Retain should be equal"),
										PublishPacket.GetShouldRetain(),
										bShouldRetain);
							});
					}
				});
	}
}

#endif // WITH_AUTOMATION_TESTS
