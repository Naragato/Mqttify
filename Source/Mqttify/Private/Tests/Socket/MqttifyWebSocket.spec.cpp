#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "LibWebSocketRunnable.h"
#include "MqttifySocketRunnable.h"
#include "Misc/AutomationTest.h"
#include "Packets/MqttifyConnectPacket.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Serialization/ArrayReader.h"
#include "Socket/Interface/MqttifySocketBase.h"
#include "Tests/Packets/PacketComparision.h"

using namespace Mqttify;

const TArray<uint8> Mqtt3BasicWithUsernamePassword = {
	// Fixed header
	0x10,
	// Packet type (0x10 = CONNECT)
	0x26,
	// Remaining length (38 bytes)

	// Variable header
	0x00,
	0x04,
	'M',
	'Q',
	'T',
	'T',
	// Protocol name (6 bytes)
	0x04,
	// Protocol version (1 byte)
	0xC2,
	// Connect flags (1 byte)
	0x00,
	0x3C,
	// Keep alive timeout (60 s) (2 bytes)

	// Payload
	0x00,
	0x06,
	'c',
	'l',
	'i',
	'e',
	'n',
	't',
	// Client ID (8 bytes)
	0x00,
	0x08,
	'u',
	's',
	'e',
	'r',
	'n',
	'a',
	'm',
	'e',
	// Username (10 bytes)
	0x00,
	0x08,
	'p',
	'a',
	's',
	's',
	'w',
	'o',
	'r',
	'd' // Password (10 bytes)
};


BEGIN_DEFINE_SPEC(
	MqttifyMqttifyWebSocketSpec,
	"Mqttify.Automation.MqttifyWebSocket",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
	EAutomationTestFlags::ServerContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::
	ProgramContext)
	TSharedPtr<FMqttifySocketRunnable> SocketRunner;
	TUniquePtr<FLibWebSocketRunnable> LibWebSocketRunner;
	static constexpr TCHAR SocketError[] = TEXT("SocketError");

END_DEFINE_SPEC(MqttifyMqttifyWebSocketSpec)

void MqttifyMqttifyWebSocketSpec::Define()
{
	LatentBeforeEach(
		EAsyncExecution::ThreadPool,
		[this](const FDoneDelegate& BeforeDone)
		{
			SocketRunner = MakeShared<FMqttifySocketRunnable>(TEXT("ws://127.0.0.1:9001"));
			LibWebSocketRunner = MakeUnique<FLibWebSocketRunnable>();
			LibWebSocketRunner->Open();

			while (!LibWebSocketRunner->IsConnected())
			{
				// Sleep
				LOG_MQTTIFY(Error, TEXT("Starting listening failed"));
				FPlatformProcess::SleepNoStats(5);
			}

			LOG_MQTTIFY(Display, TEXT("Connecting Socket runner"));
			SocketRunner->GetSocket()->GetOnConnectDelegate().AddLambda(
				[this, BeforeDone](const bool bWasSuccessful)
				{
					TestTrue(TEXT("Connection successful"), bWasSuccessful);
					BeforeDone.Execute();
				});
			SocketRunner->GetSocket()->Connect();
		});

	AfterEach(
		[this]()
		{
			LibWebSocketRunner->Stop();
			SocketRunner->Stop();
		});

	// Describe the behavior of the BSD style socket
	Describe(
		"Websocket ",
		[this]()
		{
			// Test the sending functionality
			LatentIt(
				"should send data to the server successfully",
				FTimespan::FromSeconds(30.0f),
				[this](const FDoneDelegate& Done)
				{
					// Send data using the socket
					constexpr uint8 Data[] = {0x01, 0x02, 0x03};
					constexpr uint32 Size = sizeof(Data);

					LibWebSocketRunner->Callback = [this, Data, Done, Size](
						struct lws* Wsi,
						const enum lws_callback_reasons Reason,
						void* User,
						void* In,
						const size_t Len
						)
						{
							if (Reason == LWS_CALLBACK_RECEIVE)
							{
								TArray<uint8> ReceivedData;
								ReceivedData.Append(static_cast<uint8*>(In), Len);
								TestEqual(TEXT("Received data should be equal to sent data"), Len, Size);
								TestEqual(
									TEXT("Received data should be equal to sent data [0]"),
									ReceivedData[0],
									Data[0]);
								TestEqual(
									TEXT("Received data should be equal to sent data [1]"),
									ReceivedData[1],
									Data[1]);
								TestEqual(
									TEXT("Received data should be equal to sent data [2]"),
									ReceivedData[2],
									Data[2]);
							}
							LOG_MQTTIFY(Display, TEXT("Test done"));
							Done.Execute();
							return 0;
						};

					LOG_MQTTIFY(Display, TEXT("Send data"));
					SocketRunner->GetSocket()->Send(Data, Size);
					TestTrue(TEXT("Socket should not be null"), SocketRunner->GetSocket().IsValid());
				});

			// Test the receiving functionality
			LatentIt(
				"should receive data from the server successfully",
				FTimespan::FromSeconds(30.0f),
				[this](const FDoneDelegate& Done)
				{
					SocketRunner->GetSocket()->GetOnDataReceivedDelegate().AddLambda(
						[this, Done](const TSharedPtr<FArrayReader, ESPMode::ThreadSafe>& DataReader)
						{
							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(*DataReader.Get());
							FMqttifyConnectPacket3 Packet(*DataReader.Get(), Header);
							TestPacketsEqual(
								TEXT("Data from socket should be equal to sent data"),
								Packet,
								Mqtt3BasicWithUsernamePassword,
								this);
							Done.Execute();
						});
					LibWebSocketRunner->Send(
						Mqtt3BasicWithUsernamePassword.GetData(),
						Mqtt3BasicWithUsernamePassword.Num());
				});
		});
}
#endif	// WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
