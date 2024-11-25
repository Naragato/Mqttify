#include "Tests/MqttifyTestUtilities.h"
#if WITH_DEV_AUTOMATION_TESTS

#include "MqttifySocketRunnable.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Misc/AutomationTest.h"
#include "Packets/MqttifyConnectPacket.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Serialization/ArrayReader.h"
#include "Socket/Interface/MqttifySocketBase.h"
#include "Tests/Packets/PacketComparision.h"

using namespace Mqttify;
BEGIN_DEFINE_SPEC(
	MqttifyMqttifySocketSpec,
	"Mqttify.Automation.MqttifySocket",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
	EAutomationTestFlags::ServerContext | EAutomationTestFlags::CommandletContext)
	TSharedPtr<FMqttifySocketRunnable> SocketRunner;
	FSocket* ListeningSocket;
	static constexpr TCHAR SocketError[] = TEXT("SocketError");

END_DEFINE_SPEC(MqttifyMqttifySocketSpec)

void MqttifyMqttifySocketSpec::Define()
{
	// Define the setup function for each test case
	BeforeEach(
		[this]
		{
			const uint16 Port = FindAvailablePort(1024, 32000);
			SocketRunner = MakeShared<FMqttifySocketRunnable>(FString::Printf(TEXT("mqtt://localhost:%d"), Port));

			// Create a listening socket to simulate a server
			ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
			const TSharedPtr<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();
			Addr->SetPort(Port);
			const TSharedRef<FInternetAddr> LocalAddress = Addr.ToSharedRef();
			ListeningSocket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("ListeningSocket"), false);
			ListeningSocket->SetNonBlocking(true);

			while (!ListeningSocket->Bind(*LocalAddress))
			{
				FPlatformProcess::YieldThread();
			}

			// Start listening for incoming connections
			const bool bListenSuccessful = ListeningSocket->Listen(5);
			if (!bListenSuccessful)
			{
				// Handle the case where starting listening fails
				LOG_MQTTIFY(Error, TEXT("Starting listening failed"));
			}
		});

	// Define the teardown function for each test case
	AfterEach(
		[this]
		{
			// Clean up the listening socket
			ListeningSocket->Shutdown(ESocketShutdownMode::ReadWrite);
			ListeningSocket->Close();
			SocketRunner->Stop();
			SocketRunner.Reset();
			delete ListeningSocket;
		});

	// Describe the behavior of the BSD style socket
	Describe(
		"BSD Style Socket",
		[this]
		{
			// Test the connecting functionality
			LatentIt(
				"should connect to the server successfully",
				[this](const FDoneDelegate& Done)
				{
					SocketRunner->GetSocket()->GetOnConnectDelegate().AddLambda(
						[this, Done](const bool bWasSuccessful)
						{
							TestTrue(TEXT("Socket should be connected to the server"), bWasSuccessful);
							if (Done.IsBound())
							{
								Done.Execute();
							}
						});
					SocketRunner->GetSocket()->Connect();
				});

			// Test the sending functionality
			LatentIt(
				"should send data to the server successfully",
				[this](const FDoneDelegate& Done)
				{
					SocketRunner->GetSocket()->GetOnConnectDelegate().AddLambda(
						[this, Done](const bool bWasSuccessful)
						{
							if (!bWasSuccessful)
							{
								AddError(TEXT("Socket should be connected to the server"));
								Done.Execute();
								return;
							}

							bool bHasPendingConnection = false;
							ListeningSocket->WaitForPendingConnection(bHasPendingConnection, 5.0f);
							TestTrue(TEXT("Listening socket should have a pending connection"), bHasPendingConnection);

							if (!bHasPendingConnection)
							{
								LOG_MQTTIFY(Error, TEXT("Listening socket should have a pending connection"));
								Done.Execute();
								return;
							}

							FSocket* ClientSocket = ListeningSocket->Accept(TEXT("AcceptedSocket"));
							ClientSocket->SetNonBlocking(false);
							TestNotNull(TEXT("Client socket should not be null"), ClientSocket);

							if (!bHasPendingConnection)
							{
								LOG_MQTTIFY(Error, TEXT("Listening socket should have a pending connection"));
								Done.Execute();
								return;
							}
							TestTrue(TEXT("Socket should be connected to the server"), bWasSuccessful);

							// Send data using the socket
							constexpr uint8 Data[] = {0x01, 0x02, 0x03};
							constexpr uint32 Size = sizeof(Data);

							SocketRunner->GetSocket()->Send(Data, Size);

							// **Wait until data is available for reading**
							uint8 ReceivedData[Size]{};
							int32 BytesRead = 0;
							bool bDataReceived = false;
							const FTimespan WaitTime = FTimespan::FromSeconds(60.0f);
							const FDateTime EndTime = FDateTime::Now() + WaitTime;

							while (FDateTime::Now() < EndTime)
							{
								const bool bDataAvailable = ClientSocket->Wait(
									ESocketWaitConditions::WaitForRead,
									FTimespan::FromMilliseconds(60));
								if (bDataAvailable)
								{
									const bool bRecvSuccessful = ClientSocket->Recv(
										ReceivedData,
										Size,
										BytesRead,
										ESocketReceiveFlags::WaitAll);
									if (bRecvSuccessful && BytesRead > 0)
									{
										bDataReceived = true;
										break;
									}
								}
								FPlatformProcess::Sleep(0.01f); // Yield to other threads
							}

							TestTrue(TEXT("Data should be received"), bDataReceived);
							if (bDataReceived)
							{
								TestEqual(TEXT("Number of bytes received should match"), BytesRead, (int32)Size);
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
							else
							{
								LOG_MQTTIFY(Error, TEXT("Data was not received within the timeout period"));
							}

							ClientSocket->Close();
							Done.Execute();
						});
					// Connect the socket first
					SocketRunner->GetSocket()->Connect();
				});

			// Test the receiving functionality
			LatentIt(
				"should receive data from the server successfully",
				[this](const FDoneDelegate& Done)
				{
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
					SocketRunner->GetSocket()->GetOnDataReceivedDelegate().AddLambda(
						[this, Mqtt3BasicWithUsernamePassword, Done](const TSharedPtr<FArrayReader>& Reader)
						{
							const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(*Reader);
							FMqttifyConnectPacket3 Packet(*Reader, Header);
							TestPacketsEqual(
								TEXT("Data from socket should be equal to sent data"),
								Packet,
								Mqtt3BasicWithUsernamePassword,
								this);
							if (Done.IsBound())
							{
								Done.Execute();
							}
						});
					SocketRunner->GetSocket()->GetOnConnectDelegate().AddLambda(
						[this, Done, Mqtt3BasicWithUsernamePassword](const bool bWasSuccessful)
						{
							TestTrue(TEXT("Socket should be connected to the server"), bWasSuccessful);

							bool bHasPendingConnection = false;
							ListeningSocket->WaitForPendingConnection(bHasPendingConnection, 20.0f);
							TestTrue(TEXT("Listening socket should have a pending connection"), bHasPendingConnection);

							if (!bHasPendingConnection)
							{
								Done.Execute();
								return;
							}

							FSocket* ClientSocket = ListeningSocket->Accept(TEXT("AcceptedSocket"));
							ClientSocket->SetNonBlocking(false);

							ClientSocket->Wait(ESocketWaitConditions::WaitForWrite, FTimespan::FromSeconds(5.0f));
							int32 BytesSent;
							const bool bSendSucceeded = ClientSocket->Send(
								Mqtt3BasicWithUsernamePassword.GetData(),
								Mqtt3BasicWithUsernamePassword.Num(),
								BytesSent);
							ClientSocket->Close();
							TestTrue(TEXT("Send succeeded"), bSendSucceeded);
							TestEqual(
								TEXT("Bytes sent should be equal to sent data size"),
								BytesSent,
								Mqtt3BasicWithUsernamePassword.Num());
							if (Done.IsBound())
							{
								Done.Execute();
							}
						});
					// Connect the socket first
					SocketRunner->GetSocket()->Connect();
				});
		});

	// Test the disconnecting functionality
	It(
		"should disconnect from the server successfully",
		[this]
		{
			// Connect the socket first
			SocketRunner->GetSocket()->Connect();
			// Disconnect the socket
			SocketRunner->GetSocket()->Disconnect();
			TestFalse(TEXT("Socket should not be connected to the server"), SocketRunner->GetSocket()->IsConnected());
		});
}

#endif	// WITH_DEV_AUTOMATION_TESTS
