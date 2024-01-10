#ifdef EnableSocketTests

#if WITH_AUTOMATION_TESTS
#include "LibWebSocketRunnable.h"
#include "MqttifySocketRunnable.h"
#include "PacketComparision.h"
#include "Packets/MqttifyConnectPacket.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Serialization/ArrayReader.h"
#include "Socket/Interface/IMqttifySocket.h"

#define UI UI_ST
THIRD_PARTY_INCLUDES_START
#pragma warning(push, 0)
#include <libwebsockets.h>
THIRD_PARTY_INCLUDES_END
#undef UI

namespace Mqttify
{
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


	BEGIN_DEFINE_SPEC(MqttifyMqttifyWebSocketSpec,
					"Mqttify.Automation.MqttifyWebSocket",
					EAutomationTestFlags::EngineFilter | EAutomationTestFlags::ApplicationContextMask)
		TSharedPtr<FMqttifySocketRunnable> SocketRunner;
		TSharedPtr<FLibWebSocketRunnable> LibWebSocketRunner;
		static constexpr TCHAR SocketError[] = TEXT("SocketError");

	END_DEFINE_SPEC(MqttifyMqttifyWebSocketSpec)

	void MqttifyMqttifyWebSocketSpec::Define()
	{
		BeforeEach([this]()
		{
			SocketRunner       = MakeShared<FMqttifySocketRunnable>(TEXT("ws://localhost:9001"));
			LibWebSocketRunner = FLibWebSocketRunnable::GetInstance();
			LibWebSocketRunner->Open();
			// Connect the socket first
			SocketRunner->GetSocket()->Connect();
		});

		AfterEach([this]()
		{
			LibWebSocketRunner->Stop();
			LibWebSocketRunner.Reset();
			SocketRunner->Stop();
			SocketRunner.Reset();
		});

		// Describe the behavior of the BSD style socket
		Describe("Websocket ",
				[this]()
				{
					// Test the sending functionality
					LatentIt("should send data to the server successfully",
							FTimespan::FromSeconds(30.0f),
							[this](const FDoneDelegate& Done)
							{
								TestTrue(TEXT("Socket should not be null"), SocketRunner->GetSocket().IsValid());
								// Connect the socket first
								SocketRunner->GetSocket()->Connect();
								// Send data using the socket
								constexpr uint8 Data[] = { 0x01, 0x02, 0x03 };
								constexpr uint32 Size  = sizeof(Data);
								SocketRunner->GetSocket()->Send(Data, Size);
								LibWebSocketRunner->Callback = [this, Data, Done](struct lws* Wsi,
																				const enum lws_callback_reasons Reason,
																				void* User,
																				void* In,
																				const size_t Len)
								{
									if (Reason == LWS_CALLBACK_RECEIVE)
									{
										TArray<uint8> ReceivedData;
										ReceivedData.Append(static_cast<uint8*>(In), Len);
										TestEqual(TEXT("Received data should be equal to sent data"), Len, Size);
										TestEqual(TEXT("Received data should be equal to sent data [0]"),
												ReceivedData[0],
												Data[0]);
										TestEqual(TEXT("Received data should be equal to sent data [1]"),
												ReceivedData[1],
												Data[1]);
										TestEqual(TEXT("Received data should be equal to sent data [2]"),
												ReceivedData[2],
												Data[2]);
									}
									Done.Execute();
									return 0;
								};
							});

					// Test the receiving functionality
					LatentIt("should receive data from the server successfully",
							FTimespan::FromSeconds(30.0f),
							[this](const FDoneDelegate& Done)
							{
 								LibWebSocketRunner->Send(Mqtt3BasicWithUsernamePassword.GetData(),
														Mqtt3BasicWithUsernamePassword.Num());
								SocketRunner->GetSocket()->OnDataReceived().AddLambda(
									[this, Done](uint8* Data, const SIZE_T Length)
									{
										FArrayReader Reader;
										Reader.Append(Data, Length);
										FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
										FMqttifyConnectPacket3 Packet(Reader, Header);
										TestPacketsEqual(
											TEXT("Data from socket should be equal to sent data"),
											Packet,
											Mqtt3BasicWithUsernamePassword,
											this);
										Done.Execute();
									});
							});
				});
	}
} // namespace Mqttify
#endif	// WITH_DEV_AUTOMATION_TESTS
#endif	// EnableSocketTests
