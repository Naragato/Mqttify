#include "Mqtt/MqttifyConnectionSettingsBuilder.h"
#include "Tests/MqttifyTestUtilities.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "MqttifyModule.h"
#include "Misc/AutomationTest.h"
#include "Mqtt/MqttifyClient.h"
#include "Misc/Guid.h"
#include "Misc/DateTime.h"
#include "Misc/Timespan.h"
#include "Misc/Parse.h"
#include "Containers/Ticker.h"

using namespace Mqttify;

BEGIN_DEFINE_SPEC(
	FMqttifyClientLongRunningTest,
	"Mqttify.Automation.MqttifyClientLongRunningTest",
	EAutomationTestFlags::StressFilter | EAutomationTestFlags::ApplicationContextMask)
	static constexpr TCHAR kTopic[] = TEXT("/LongRunningTest");
	FString DockerContainerName = TEXT("vernemq_test_container");
	TArray<TSharedPtr<IMqttifyClient>> MqttClients;

	float TestDurationSeconds = 3600.0f; // default to 1 hour
	int32 MaxPendingMessages = 1000;

	FDateTime StartTime;
	TMap<FString, FDateTime> PublishBuffer;

	bool bTestDoneExecuted = false;

	void SetupClients(const FMqttifyTestDockerSpec& InSpec, const FDoneDelegate& InBeforeDone);
	void DisconnectClients(const FDoneDelegate& InAfterDone) const;
	void CheckTestCompletion(const FDoneDelegate& TestDone);

END_DEFINE_SPEC(FMqttifyClientLongRunningTest)

void FMqttifyClientLongRunningTest::Define()
{
	FMqttifyTestDockerSpec Spec{8080, FindAvailablePort(5000, 10000), EMqttifyConnectionProtocol::Ws};

	FString DurationArg;
	if (FParse::Value(FCommandLine::Get(), TEXT("MqttifyLongTestDuration="), DurationArg))
	{
		TestDurationSeconds = FCString::Atof(*DurationArg);
	}

	TArray QualityOfServices = {
		EMqttifyQualityOfService::AtMostOnce,
		EMqttifyQualityOfService::AtLeastOnce,
		EMqttifyQualityOfService::ExactlyOnce
	};

	Describe(
		"Long-Running Client Stability Test",
		[this, Spec, QualityOfServices]()
		{
			LatentBeforeEach(
				[this, Spec](const FDoneDelegate& BeforeDone)
				{
					if (!StartBroker(DockerContainerName, Spec))
					{
						AddError(TEXT("Failed to start MQTT broker."));
						BeforeDone.Execute();
						return;
					}
					SetupClients(Spec, BeforeDone);
				});

			for (const EMqttifyQualityOfService& QoS : QualityOfServices)
			{
				LatentIt(
					FString::Printf(TEXT("Long-running test for QoS %s"), EnumToTCharString(QoS)),
					FTimespan::FromSeconds(TestDurationSeconds + 300.0f),
					[this, QoS](const FDoneDelegate& TestDone)
					{
						if (MqttClients.Num() != 2)
						{
							AddError(TEXT("Expected 2 client pointers"));
							TestDone.Execute();
							return;
						}

						TSharedPtr<IMqttifyClient> Publisher = MqttClients[0];
						TSharedPtr<IMqttifyClient> Subscriber = MqttClients[1];

						StartTime = FDateTime::UtcNow();
						bTestDoneExecuted = false;

						FMqttifyTopicFilter TopicFilter{FString{kTopic}, QoS};
						Subscriber->SubscribeAsync(MoveTemp(TopicFilter)).Next(
							[this, Publisher, Subscriber, QoS, TestDone](
							const TMqttifyResult<FMqttifySubscribeResult>& InResult
							)
							{
								if (!InResult.HasSucceeded())
								{
									AddError(TEXT("SubscribeAsync failed."));
									if (!bTestDoneExecuted)
									{
										bTestDoneExecuted = true;
										TestDone.Execute();
									}
									return;
								}

								Subscriber->OnMessage().AddLambda(
									[this, Publisher, QoS, TestDone](const FMqttifyMessage& InMessage)
									{
										const FString MessagePayload = BytesToString(
											InMessage.GetPayload().GetData(),
											InMessage.GetPayload().Num());
										FString MessageID, SentTimeString;
										if (!MessagePayload.Split(TEXT("|"), &MessageID, &SentTimeString))
										{
											AddError(TEXT("Failed to parse message payload"));
											if (!bTestDoneExecuted)
											{
												bTestDoneExecuted = true;
												TestDone.Execute();
											}
											return;
										}

										FDateTime SentTime;
										if (!FDateTime::ParseIso8601(*SentTimeString, SentTime))
										{
											AddError(TEXT("Failed to parse timestamp"));
											if (!bTestDoneExecuted)
											{
												bTestDoneExecuted = true;
												TestDone.Execute();
											}
											return;
										}

										const FTimespan TimeTaken = FDateTime::UtcNow() - SentTime;
										LOG_MQTTIFY(
											VeryVerbose,
											TEXT("Received message ID %s, Time taken: %s"),
											*MessageID,
											*TimeTaken.ToString());

										if (PublishBuffer.Remove(MessageID) == 0)
										{
											AddError(
												FString::Printf(
													TEXT("Received message ID %s which was not in the publish buffer"),
													*MessageID));
											if (!bTestDoneExecuted)
											{
												bTestDoneExecuted = true;
												TestDone.Execute();
											}
											return;
										}

										if ((FDateTime::UtcNow() - StartTime).GetTotalSeconds() < TestDurationSeconds)
										{
											if (PublishBuffer.Num() < MaxPendingMessages)
											{
												FString NewMessageID = FGuid::NewGuid().ToString();
												const FString NewMessagePayload = FString::Printf(
													TEXT("%s|%s"),
													*NewMessageID,
													*FDateTime::UtcNow().ToIso8601());

												TArray<uint8> PayloadBytes;
												PayloadBytes.Reserve(NewMessagePayload.Len());
												StringToBytes(
													NewMessagePayload,
													PayloadBytes.GetData(),
													NewMessagePayload.Len());
												FMqttifyMessage Message{
													FString{kTopic},
													MoveTemp(PayloadBytes),
													false,
													QoS
												};

												PublishBuffer.Add(NewMessageID, FDateTime::UtcNow());

												Publisher->PublishAsync(MoveTemp(Message)).Next(
													[this, NewMessageID, TestDone](const TMqttifyResult<void>& InResult)
													{
														if (!InResult.HasSucceeded())
														{
															AddError(
																FString::Printf(
																	TEXT("PublishAsync failed for message ID %s"),
																	*NewMessageID));
															if (!bTestDoneExecuted)
															{
																bTestDoneExecuted = true;
																AsyncTask(
																	ENamedThreads::GameThread,
																	[this, TestDone]()
																	{
																		TestDone.Execute();
																	});
															}
														}
													});
											}
										}

										// Check if test duration has passed and PublishBuffer is empty
										CheckTestCompletion(TestDone);
									});

								for (int32 i = 0; i < MaxPendingMessages; ++i)
								{
									if ((FDateTime::UtcNow() - StartTime).GetTotalSeconds() >= TestDurationSeconds)
									{
										break;
									}

									FString MessageID = FGuid::NewGuid().ToString();
									FString MessagePayload = FString::Printf(
										TEXT("%s|%s"),
										*MessageID,
										*FDateTime::UtcNow().ToIso8601());

									TArray<uint8> PayloadBytes;
									PayloadBytes.Reserve(MessagePayload.Len());
									StringToBytes(MessagePayload, PayloadBytes.GetData(), MessagePayload.Len());

									FMqttifyMessage Message{FString{kTopic}, MoveTemp(PayloadBytes), false, QoS};

									PublishBuffer.Add(MessageID, FDateTime::UtcNow());

									Publisher->PublishAsync(MoveTemp(Message)).Next(
										[this, MessageID, TestDone](const TMqttifyResult<void>& InResult)
										{
											if (!InResult.HasSucceeded())
											{
												AddError(
													FString::Printf(
														TEXT("PublishAsync failed for message ID %s"),
														*MessageID));
												if (!bTestDoneExecuted)
												{
													bTestDoneExecuted = true;
													AsyncTask(
														ENamedThreads::GameThread,
														[this, TestDone]()
														{
															TestDone.Execute();
														});
												}
											}
										});
								}

								FTSTicker::GetCoreTicker().AddTicker(
									FTickerDelegate::CreateLambda(
										[this, TestDone](float DeltaTime)
										{
											CheckTestCompletion(TestDone);
											return !bTestDoneExecuted;
										}),
									1.0f);
							});
					});
			}

			LatentAfterEach(
				[this](const FDoneDelegate& AfterDone)
				{
					DisconnectClients(AfterDone);
				});
		});
}

void FMqttifyClientLongRunningTest::SetupClients(
	const FMqttifyTestDockerSpec& InSpec,
	const FDoneDelegate& InBeforeDone
	)
{
	auto& MqttifyModule = IMqttifyModule::Get();
	MqttClients.Empty();
	for (int32 i = 0; i < 2; ++i)
	{
		FString ClientName = FString::Printf(TEXT("client%d"), i);
		FString Username = FString::Printf(TEXT("client%d"), i);
		FString Password = TEXT("password");

		FMqttifyConnectionSettingsBuilder ConnectionSettingsBuilder(
			FString::Printf(
				TEXT("%s://%s:%s@localhost:%d%s"),
				GetProtocolString(InSpec.Protocol),
				*Username,
				*Password,
				InSpec.PublicPort,
				GetPath(InSpec.Protocol)));
		ConnectionSettingsBuilder.SetClientId(FString::Printf(TEXT("Test - Client%d"), i));
		const TSharedRef<FMqttifyConnectionSettings> ConnectionSettings = ConnectionSettingsBuilder.Build().
			ToSharedRef();

		TSharedPtr<IMqttifyClient> MqttClient = MqttifyModule.GetOrCreateClient(ConnectionSettings);

		if (!MqttClient)
		{
			AddError(FString::Printf(TEXT("Failed to create MQTT client %d."), i));
			InBeforeDone.Execute();
			return;
		}

		MqttClients.Add(MqttClient);
		MqttClient->OnConnect().AddLambda(
			[this, InBeforeDone](const bool bIsConnected)
			{
				if (!bIsConnected)
				{
					AddError(TEXT("Client failed to connect"));
				}

				if (MqttClients.ContainsByPredicate(
					[](const TSharedPtr<IMqttifyClient>& Client) { return !Client->IsConnected(); }))
				{
					return;
				}
				InBeforeDone.Execute();
			});
	}

	for (const auto& MqttClient : MqttClients)
	{
		MqttClient->ConnectAsync(false);
	}
}

void FMqttifyClientLongRunningTest::DisconnectClients(const FDoneDelegate& InAfterDone) const
{
	for (const TSharedPtr<IMqttifyClient>& MqttClient : MqttClients)
	{
		MqttClient->DisconnectAsync().Next(
			[this, InAfterDone](const TMqttifyResult<void>&)
			{
				if (MqttClients.ContainsByPredicate(
					[](const TSharedPtr<IMqttifyClient>& Client) { return Client->IsConnected(); }))
				{
					return;
				}
				InAfterDone.Execute();
			});
	}
}

void FMqttifyClientLongRunningTest::CheckTestCompletion(const FDoneDelegate& TestDone)
{
	if (!bTestDoneExecuted && (FDateTime::UtcNow() - StartTime).GetTotalSeconds() >= TestDurationSeconds &&
		PublishBuffer.Num() == 0)
	{
		bTestDoneExecuted = true;
		AsyncTask(
			ENamedThreads::GameThread,
			[TestDone]()
			{
				TestDone.Execute();
			});
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS
