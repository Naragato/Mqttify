#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Packets/MqttifyPingRespPacket.h"
#include "Serialization/MemoryReader.h"
#include "Tests/Packets/PacketComparision.h"

namespace Mqttify
{
	BEGIN_DEFINE_SPEC(MqttifyPingRespPacket,
					"Mqttify.Automation.MqttifyPingRespPacket",
					EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)


		const TArray<uint8> ValidPacket = {
			0xD0, // Packet type (PingResp)
			0x00
		};

		const TArray<uint8> InvalidPacket = {
			0xD0, // Packet type (PingResp)
			0x01
		};

	END_DEFINE_SPEC(MqttifyPingRespPacket)

	void MqttifyPingRespPacket::Define()
	{
		Describe("TMqttifyPingRespPacket",
				[this] {
					It("Should serialize with properties",
						[this] {
							FMqttifyPingRespPacket Packet;
							TestPacketsEqual(TEXT("Packet should match expected"), Packet, ValidPacket, this);
						});

					It("Should log error when deserializing invalid packet",
						[this] {
							AddExpectedError(FMqttifyFixedHeader::InvalidPacketSize,
											EAutomationExpectedMessageFlags::Contains);
							FArrayReader Reader;
							Reader.Append(InvalidPacket);
							const FMqttifyFixedHeader FixedHeader = FMqttifyFixedHeader::Create(Reader);
							FMqttifyPingRespPacket Packet{ FixedHeader };
						});
				});
	}
} // namespace Mqttify
#endif // WITH_DEV_AUTOMATION_TESTS
