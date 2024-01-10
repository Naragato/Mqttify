#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Packets/MqttifyPingReqPacket.h"
#include "Serialization/MemoryReader.h"
#include "Tests/Packets/PacketComparision.h"

namespace Mqttify
{
	BEGIN_DEFINE_SPEC(MqttifyPingReqPacket,
					"Mqttify.Automation.MqttifyPingReqPacket",
					EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)


		const TArray<uint8> ValidPacket = {
			0xC0, // Packet type (PingReq)
			0x00
		};

		const TArray<uint8> InvalidPacket = {
			0xC0, // Packet type (PingReq)
			0x01
		};

	END_DEFINE_SPEC(MqttifyPingReqPacket)

	void MqttifyPingReqPacket::Define()
	{
		Describe("TMqttifyPingReqPacket",
				[this] {
					It("Should serialize with properties",
						[this] {
							FMqttifyPingReqPacket Packet;
							TestPacketsEqual(TEXT("Packet should match expected"), Packet, ValidPacket, this);
						});

					It("Should log error when deserializing invalid packet",
						[this] {
							AddExpectedError(FMqttifyFixedHeader::InvalidPacketSize,
											EAutomationExpectedMessageFlags::Contains);
							FArrayReader Reader;
							Reader.Append(InvalidPacket);
							const FMqttifyFixedHeader FixedHeader = FMqttifyFixedHeader::Create(Reader);
							FMqttifyPingReqPacket Packet{ FixedHeader };
						});
				});
	}
} // namespace Mqttify
#endif // WITH_DEV_AUTOMATION_TESTS
