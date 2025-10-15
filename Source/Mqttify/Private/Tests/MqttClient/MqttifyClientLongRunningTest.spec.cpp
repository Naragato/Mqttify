#if WITH_DEV_AUTOMATION_TESTS

#include "JsonObjectConverter.h"
#include "MqttifyModule.h"
#include "MqttifyTestMessage.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Timespan.h"
#include "Mqtt/MqttifyClient.h"
#include "Mqtt/MqttifyConnectionSettingsBuilder.h"
#include "Tests/MqttifyTestUtilities.h"

using namespace Mqttify;

BEGIN_DEFINE_SPEC(
	FMqttifyClientLongRunningTest,
	"Mqttify.Automation.MqttifyClientLongRunningTest",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
	EAutomationTestFlags::ServerContext | EAutomationTestFlags::CommandletContext)
	static constexpr TCHAR kTopic[] = TEXT("/LongRunningTest");
	FString DockerContainerName = TEXT("vernemq_test_container");
	TArray<TSharedRef<IMqttifyClient>> MqttClients;

	float TestDurationSeconds = 120.0f;
	int32 MaxPendingMessages = 120;

	FDateTime StartTime;
	TSet<FMqttifyTestMessage> PublishBuffer;

	bool bTestDoneExecuted = false;

	void SetupClients(
		const FMqttifyTestDockerSpec& InSpec,
		const FDoneDelegate& InBeforeDone,
		const EMqttifyQualityOfService InQoS
		);
	void PublishMessages(const EMqttifyQualityOfService InQoS);
	void DisconnectClients(const FDoneDelegate& InAfterDone) const;
	void CheckTestCompletion(const FDoneDelegate& TestDone);
	ELogVerbosity::Type OriginalLogVerbosity = LogMqttify.GetVerbosity();
END_DEFINE_SPEC(FMqttifyClientLongRunningTest)

void FMqttifyClientLongRunningTest::Define()
{
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

	// Define protocol specs: MQTT (1883), MQTTS (1884), and existing WS (8080)
	const TArray ProtocolSpecs = {
		// FMqttifyTestDockerSpec{1883, FindAvailablePort(5000, 10000), EMqttifyConnectionProtocol::Mqtt},
		// FMqttifyTestDockerSpec{1884, FindAvailablePort(5000, 10000), EMqttifyConnectionProtocol::Mqtts}, TODO: Need to find a docker image which terminates with TLS
		FMqttifyTestDockerSpec{8080, FindAvailablePort(5000, 10000), EMqttifyConnectionProtocol::Ws}
	};

	for (const FMqttifyTestDockerSpec& Spec : ProtocolSpecs)
	{
		for (const EMqttifyQualityOfService& QoS : QualityOfServices)
		{
			Describe(
				FString::Printf(TEXT("Long-running test for %s QoS %s"),
				                GetProtocolString(Spec.Protocol),
				                EnumToTCharString(QoS)),
				[this, Spec, QoS]() {
					LatentBeforeEach(
						FTimespan::FromSeconds(120),
						[this, Spec, QoS](const FDoneDelegate& BeforeDone) {
							LogMqttify.SetVerbosity(ELogVerbosity::VeryVerbose);
							if (!StartBroker(DockerContainerName, Spec))
							{
								AddError(TEXT("Failed to start MQTT broker"));
								LOG_MQTTIFY(Error, TEXT("Failed to start MQTT broker"));
								BeforeDone.Execute();
								return;
							}
							SetupClients(Spec, BeforeDone, QoS);
						});

					LatentIt(
						TEXT("UnsubscribeAsync.Next is called for const TSet<FString>& InTopicFilters"),
						FTimespan::FromSeconds(120.0f),
						[this](const FDoneDelegate& TestDone) {
							const TSharedPtr<IMqttifyClient> Client = MqttClients[0];
							const TSet TopicFilters = {FString{kTopic}};
							Client->UnsubscribeAsync(TopicFilters).Next(
								[this, TestDone](const TMqttifyResult<TArray<FMqttifyUnsubscribeResult>>& InResult) {
									TestTrue(TEXT("UnsubscribeAsync.Next should be called"), InResult.HasSucceeded());
									TestDone.Execute();
								});
						});

					LatentIt(
						TEXT("Wildcard subscriptions should work with real broker"),
						FTimespan::FromSeconds(60.0f + TestDurationSeconds),
						[this, QoS](const FDoneDelegate& TestDone) {
							if (MqttClients.Num() != 2)
							{
								AddError(TEXT("Expected 2 client pointers"));
								TestDone.Execute();
								return;
							}

							const TSharedRef<IMqttifyClient> Publisher = MqttClients[0];
							const TSharedRef<IMqttifyClient> Subscriber = MqttClients[1];

							const FString PlusFilter = TEXT("tests/wild/+/temp");
							const FString HashFilter = TEXT("tests/wild/#");
							const FString ExactTopic = TEXT("tests/wild/uk/temp");
							const FString OnlyHashTopic = TEXT("tests/wild/uk/humidity");
							const FString NonMatchTopic = TEXT("tests/other/uk/temp");
							const FString DoneTopic = TEXT("tests/wild/__done");

							auto bCompleted = MakeShared<TAtomic<bool>>(false);

							AddExpectedMessage(TEXT("WILD_PLUS_OK"),
							                   EAutomationExpectedMessageFlags::Contains,
							                   0,
							                   false);
							AddExpectedMessage(TEXT("WILD_HASH_OK"),
							                   EAutomationExpectedMessageFlags::Contains,
							                   0,
							                   false);
							AddExpectedMessage(TEXT("WILD_EXACT_OK"),
							                   EAutomationExpectedMessageFlags::Contains,
							                   0,
							                   false);
							AddExpectedMessage(TEXT("WILD_DONE"), EAutomationExpectedMessageFlags::Contains, 1, false);

							TArray<FMqttifyTopicFilter> Filters;
							Filters.Emplace(FMqttifyTopicFilter{PlusFilter, QoS});
							Filters.Emplace(FMqttifyTopicFilter{HashFilter, QoS});
							Filters.Emplace(FMqttifyTopicFilter{ExactTopic, QoS});

							Subscriber->SubscribeAsync(Filters).Next(
								[this, TestDone, Publisher, Subscriber, PlusFilter, HashFilter, ExactTopic,
									OnlyHashTopic, NonMatchTopic, DoneTopic, bCompleted](
								const TMqttifyResult<TArray<FMqttifySubscribeResult>>& InResult) {
									if (!InResult.HasSucceeded())
									{
										AddError(TEXT("Wildcard SubscribeAsync failed"));
										TestDone.Execute();
										return InResult;
									}

									for (const FMqttifySubscribeResult& Res : *InResult.GetResult())
									{
										const FString F = Res.GetFilter().GetFilter();
										if (const TSharedPtr<FOnMessage> D = Res.GetOnMessage())
										{
											if (F == PlusFilter)
											{
												D->AddLambda([this, ExactTopic](const FMqttifyMessage& Msg) {
													if (Msg.GetTopic() == ExactTopic)
													{
														LOG_MQTTIFY(Display, TEXT("WILD_PLUS_OK"));
													}
													else
													{
														AddError(FString::Printf(
															TEXT("Plus filter received unexpected topic: %s"),
															*Msg.GetTopic()));
													}
												});
											}
											else if (F == HashFilter)
											{
												D->AddLambda(
													[this, ExactTopic, OnlyHashTopic, DoneTopic, TestDone, bCompleted, Subscriber, PlusFilter, HashFilter](
													const FMqttifyMessage& Msg) {
														if (Msg.GetTopic() == ExactTopic || Msg.GetTopic() ==
															OnlyHashTopic)
														{
															LOG_MQTTIFY(Display, TEXT("WILD_HASH_OK"));
														}
														else if (Msg.GetTopic() == DoneTopic)
														{
															LOG_MQTTIFY(Display, TEXT("WILD_DONE"));
															bCompleted->Store(true);
															const TSet<FString> FiltersToRemove{
																PlusFilter, HashFilter, ExactTopic};
															Subscriber->UnsubscribeAsync(FiltersToRemove).Next(
																[this, TestDone](
																const TMqttifyResult<TArray<FMqttifyUnsubscribeResult>>&
																R) {
																	if (!R.HasSucceeded())
																	{
																		AddError(TEXT(
																			"Unsubscribe failed in wildcard test cleanup"));
																	}
																	TestDone.Execute();
																	return R;
																});
															return;
														}
														else
														{
															AddError(FString::Printf(
																TEXT("Hash filter received unexpected topic: %s"),
																*Msg.GetTopic()));
														}
													});
											}
											else if (F == ExactTopic)
											{
												D->AddLambda([this, ExactTopic](const FMqttifyMessage& Msg) {
													if (Msg.GetTopic() == ExactTopic)
													{
														LOG_MQTTIFY(Display, TEXT("WILD_EXACT_OK"));
													}
													else
													{
														AddError(FString::Printf(
															TEXT("Exact filter received unexpected topic: %s"),
															*Msg.GetTopic()));
													}
												});
											}
										}
									}

									Publisher->PublishAsync(FMqttifyMessage{
										ExactTopic, TArray<uint8>{'E', 'X', 'A', 'C', 'T'}, false,
										EMqttifyQualityOfService::AtMostOnce});
									Publisher->PublishAsync(FMqttifyMessage{
										OnlyHashTopic, TArray<uint8>{'H', 'A', 'S', 'H'}, false,
										EMqttifyQualityOfService::AtMostOnce});
									Publisher->PublishAsync(FMqttifyMessage{
										NonMatchTopic, TArray<uint8>{'N', 'O', 'N'}, false,
										EMqttifyQualityOfService::AtMostOnce});
									Publisher->PublishAsync(FMqttifyMessage{
										DoneTopic, TArray<uint8>{'D', 'O', 'N', 'E'}, false,
										EMqttifyQualityOfService::AtLeastOnce});

									const double Start = FPlatformTime::Seconds();
									FTSTicker::GetCoreTicker().AddTicker(
										FTickerDelegate::CreateLambda([this, TestDone, Start, bCompleted](float) {
											if (bCompleted->Load())
											{
												return false;
											}
											if (FPlatformTime::Seconds() - Start > 60.0)
											{
												AddError(TEXT(
													"Timed out waiting for 'done' topic in wildcard broker test"));
												bCompleted->Store(true);
												TestDone.Execute();
												return false;
											}
											return true;
										}),
										0.25f);

									return InResult;
								});
						});

					LatentIt(
						FString::Printf(TEXT("Continuously send messages to a subscriber")),
						FTimespan::FromSeconds(TestDurationSeconds + 300.0f),
						[this, QoS](const FDoneDelegate& TestDone) {
							StartTime = FDateTime::UtcNow();
							bTestDoneExecuted = false;

							LOG_MQTTIFY(VeryVerbose,
							            TEXT("Starting long-running test for QoS %s"),
							            EnumToTCharString(QoS));
							const TSharedPtr<IMqttifyClient> Subscriber = MqttClients[1];
							Subscriber->OnMessage().AddLambda(
								[this, QoS, TestDone](const FMqttifyMessage& InMessage) {
									const FString OriginalMessage = BytesToString(
										InMessage.GetPayload().GetData(),
										InMessage.GetPayload().Num());

									LOG_MQTTIFY_PACKET_DATA(
										VeryVerbose,
										InMessage.GetPayload().GetData(),
										InMessage.GetPayload().Num(),
										TEXT("Received message payload %s"),
										*OriginalMessage);

									FMqttifyTestMessage TestMessage;
									FJsonObjectConverter::JsonObjectStringToUStruct(OriginalMessage, &TestMessage);
									const int32 Removed = PublishBuffer.Remove(TestMessage);
									const auto Elapsed = FDateTime::UtcNow() - TestMessage.Time;
									LOG_MQTTIFY(
										VeryVerbose,
										TEXT(
											"Message ID %s was removed from the publish buffer. Elapsed time: %s. Removed: %d"
										),
										*TestMessage.MessageId.ToString(),
										*Elapsed.ToString(),
										Removed);
									if (Removed == 0 && QoS != EMqttifyQualityOfService::AtLeastOnce)
									{
										LOG_MQTTIFY(Error,
										            TEXT("Received message which was not in the publish buffer"));
										AddError(TEXT("Received message which was not in the publish buffer"));
										bTestDoneExecuted = true;
										CheckTestCompletion(TestDone);
										return;
									}

									CheckTestCompletion(TestDone);
								});

							FTSTicker::GetCoreTicker().AddTicker(
								FTickerDelegate::CreateLambda(
									[this, QoS, TestDone](float Delta) {
										LOG_MQTTIFY(VeryVerbose, TEXT("Ticker tick"));
										PublishMessages(QoS);
										CheckTestCompletion(TestDone);
										return !bTestDoneExecuted;
									}),
								1.f);
						});

					LatentAfterEach(
						FTimespan::FromSeconds(120),
						[this](const FDoneDelegate& AfterDone) {
							FTSTicker::GetCoreTicker().AddTicker(
								FTickerDelegate::CreateLambda(
									[this, AfterDone](float Delta) {
										if (PublishBuffer.Num() == 0)
										{
											DisconnectClients(AfterDone);
											return false;
										}

										LOG_MQTTIFY(
											VeryVerbose,
											TEXT("Waiting for all messages to be received. %d messages remaining"),
											PublishBuffer.Num());
										return true;
									}),
								5.0f);
						});
				});
		}
	}
}

void FMqttifyClientLongRunningTest::SetupClients(
	const FMqttifyTestDockerSpec& InSpec,
	const FDoneDelegate& InBeforeDone,
	const EMqttifyQualityOfService InQoS
	)
{
	LOG_MQTTIFY(VeryVerbose, TEXT("Setting up clients"));
	auto& MqttifyModule = IMqttifyModule::Get();
	MqttClients.Empty();
	for (int32 i = 0; i < 2; ++i)
	{
		FString Id = FGuid::NewGuid().ToString();
		FString ClientName = FString::Printf(TEXT("client%s"), *Id);
		FString Username = FString::Printf(TEXT("client%s"), *Id);
		FString Password = TEXT("password");

		FMqttifyConnectionSettingsBuilder ConnectionSettingsBuilder(
			FString::Printf(
				TEXT("%s://%s:%s@127.0.0.1:%d%s"),
				GetProtocolString(InSpec.Protocol),
				*Username,
				*Password,
				InSpec.PublicPort,
				GetPath(InSpec.Protocol)));
		ConnectionSettingsBuilder.SetClientId(FString::Printf(TEXT("Test - Client%s"), *Id));
		const TSharedRef<FMqttifyConnectionSettings> ConnectionSettings = ConnectionSettingsBuilder.Build().
			ToSharedRef();

		TSharedPtr<IMqttifyClient> MqttClient = MqttifyModule.GetOrCreateClient(ConnectionSettings);

		if (!MqttClient.IsValid())
		{
			AddError(FString::Printf(TEXT("Failed to create MQTT client %d"), i));
			InBeforeDone.Execute();
			return;
		}

		MqttClients.Add(MqttClient.ToSharedRef());
		MqttClient->OnConnect().AddLambda(
			[this](const bool bIsConnected) {
				if (!bIsConnected)
				{
					AddError(TEXT("Client failed to connect"));
					bTestDoneExecuted = true;
				}
			});
	}

	const TWeakPtr<IMqttifyClient> SubscriberWeak = MqttClients[1];
	SubscriberWeak.Pin()->OnConnect().AddLambda(
		[this, InBeforeDone, InQoS, SubscriberWeak](const bool bIsConnected) {
			FMqttifyTopicFilter TopicFilter{FString{kTopic}, InQoS};
			if (const TSharedPtr<IMqttifyClient> SubscriberStrong = SubscriberWeak.Pin())
			{
				SubscriberStrong->SubscribeAsync(MoveTemp(TopicFilter)).Next(
					[this, InBeforeDone](const TMqttifyResult<FMqttifySubscribeResult>& InResult) {
						if (!InResult.HasSucceeded())
						{
							AddError(TEXT("Failed to subscribe"));
							bTestDoneExecuted = true;
						}
						InBeforeDone.Execute();
					});
			}
		});

	for (const auto& MqttClient : MqttClients)
	{
		MqttClient->ConnectAsync(false);
	}
}

void FMqttifyClientLongRunningTest::PublishMessages(const EMqttifyQualityOfService InQoS)
{
	LOG_MQTTIFY(VeryVerbose, TEXT("Publishing messages"));
	if (MqttClients.Num() != 2)
	{
		AddError(TEXT("Expected 2 client pointers"));
		bTestDoneExecuted = true;
		return;
	}

	const TSharedRef<IMqttifyClient> Publisher = MqttClients[0];

	LOG_MQTTIFY(VeryVerbose, TEXT("PublishBuffer.Num() = %d"), PublishBuffer.Num());
	while (!bTestDoneExecuted && PublishBuffer.Num() < MaxPendingMessages && (FDateTime::UtcNow() - StartTime).
		GetTotalSeconds() < TestDurationSeconds)
	{
		FMqttifyTestMessage TestMessage;
		TestMessage.MessageId = FGuid::NewGuid();
		FString JSONSerializedData;
		FJsonObjectConverter::UStructToJsonObjectString(TestMessage, JSONSerializedData);
		TArray<uint8> PayloadBytes;
		PayloadBytes.SetNumUninitialized(JSONSerializedData.Len());

		StringToBytes(JSONSerializedData, PayloadBytes.GetData(), JSONSerializedData.Len());

		LOG_MQTTIFY_PACKET_DATA(
			VeryVerbose,
			PayloadBytes.GetData(),
			PayloadBytes.Num(),
			TEXT("Publishing message with payload %s"),
			*JSONSerializedData);
		FMqttifyMessage Message{FString{kTopic}, MoveTemp(PayloadBytes), false, InQoS};

		PublishBuffer.Add(TestMessage);
		Publisher->PublishAsync(MoveTemp(Message)).Next(
			[this, TestMessage](const TMqttifyResult<void>& InResult) {
				if (!InResult.HasSucceeded())
				{
					LOG_MQTTIFY(
						Error,
						TEXT("PublishAsync failed for message ID %s"),
						*TestMessage.MessageId.ToString());
					AddError(
						FString::Printf(
							TEXT("PublishAsync failed for message ID %s"),
							*TestMessage.MessageId.ToString()));
					bTestDoneExecuted = true;
				}
			});
	}
}

void FMqttifyClientLongRunningTest::DisconnectClients(const FDoneDelegate& InAfterDone) const
{
	LOG_MQTTIFY(VeryVerbose, TEXT("Disconnecting clients"));

	for (const TSharedRef<IMqttifyClient>& MqttClient : MqttClients)
	{
		MqttClient->DisconnectAsync().Next(
			[this, InAfterDone](const TMqttifyResult<void>&) {
				if (MqttClients.ContainsByPredicate(
					[](const TSharedPtr<IMqttifyClient>& Client) { return Client->IsConnected(); }))
				{
					return;
				}
				LogMqttify.SetVerbosity(OriginalLogVerbosity);
				InAfterDone.Execute();
			});
	}
}

void FMqttifyClientLongRunningTest::CheckTestCompletion(const FDoneDelegate& TestDone)
{
	LOG_MQTTIFY(VeryVerbose, TEXT("Checking test completion"));
	if (bTestDoneExecuted)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("Test already done"));
		TestDone.Execute();
		return;
	}

	if ((FDateTime::UtcNow() - StartTime).GetTotalSeconds() >= TestDurationSeconds && PublishBuffer.Num() == 0)
	{
		bTestDoneExecuted = true;
		LOG_MQTTIFY(VeryVerbose, TEXT("Test completed successfully"));
		TestDone.Execute();
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS