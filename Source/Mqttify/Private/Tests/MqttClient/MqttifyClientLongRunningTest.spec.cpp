﻿#if WITH_DEV_AUTOMATION_TESTS

#include "JsonObjectConverter.h"
#include "MqttifyModule.h"
#include "MqttifyTestMessage.h"
#include "Misc/AutomationTest.h"
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
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
	static constexpr TCHAR kTopic[] = TEXT("/LongRunningTest");
	FString DockerContainerName = TEXT("vernemq_test_container");
	TArray<TSharedPtr<IMqttifyClient>> MqttClients;

	float TestDurationSeconds = 3600.0f; // default to 1 hour
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

	for (const EMqttifyQualityOfService& QoS : QualityOfServices)
	{
		Describe(
			FString::Printf(TEXT("Long-running test for QoS %s"), EnumToTCharString(QoS)),
			[this, Spec, QoS]()
			{
				LatentBeforeEach(
					FTimespan::FromSeconds(120),
					[this, Spec, QoS](const FDoneDelegate& BeforeDone)
					{
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
					FString::Printf(TEXT("Continuously send messages to a subscriber")),
					FTimespan::FromSeconds(TestDurationSeconds + 300.0f),
					[this, QoS](const FDoneDelegate& TestDone)
					{
						StartTime = FDateTime::UtcNow();
						bTestDoneExecuted = false;

						LOG_MQTTIFY(VeryVerbose, TEXT("Starting long-running test for QoS %s"), EnumToTCharString(QoS));
						const TSharedPtr<IMqttifyClient> Subscriber = MqttClients[1];
						Subscriber->OnMessage().AddLambda(
							[this, QoS, TestDone](const FMqttifyMessage& InMessage)
							{
								const FString OriginalMessage = BytesToString(
									InMessage.GetPayload().GetData(),
									InMessage.GetPayload().Num());

								LOG_MQTTIFY(VeryVerbose, TEXT("Received message %s"), *OriginalMessage);
								LOG_MQTTIFY_PACKET_DATA(
									VeryVerbose,
									InMessage.GetPayload().GetData(),
									InMessage.GetPayload().Num());

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
									LOG_MQTTIFY(Error, TEXT("Received message which was not in the publish buffer"));
									AddError(TEXT("Received message which was not in the publish buffer"));
									bTestDoneExecuted = true;
									CheckTestCompletion(TestDone);
									return;
								}

								CheckTestCompletion(TestDone);
							});

						FTSTicker::GetCoreTicker().AddTicker(
							FTickerDelegate::CreateLambda(
								[this, QoS, TestDone](float Delta)
								{
									LOG_MQTTIFY(VeryVerbose, TEXT("Ticker tick"));
									PublishMessages(QoS);
									CheckTestCompletion(TestDone);
									return !bTestDoneExecuted;
								}),
							1.f);
					});

				LatentAfterEach(
					FTimespan::FromSeconds(120),
					[this](const FDoneDelegate& AfterDone)
					{
						FTSTicker::GetCoreTicker().AddTicker(
							FTickerDelegate::CreateLambda(
								[this, AfterDone](float Delta)
								{
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
			AddError(FString::Printf(TEXT("Failed to create MQTT client %d"), i));
			InBeforeDone.Execute();
			return;
		}

		MqttClients.Add(MqttClient);
		MqttClient->OnConnect().AddLambda(
			[this](const bool bIsConnected)
			{
				if (!bIsConnected)
				{
					AddError(TEXT("Client failed to connect"));
					bTestDoneExecuted = true;
				}
			});
	}

	const TSharedPtr<IMqttifyClient> Subscriber = MqttClients[1];
	Subscriber->OnConnect().AddLambda(
		[this, InBeforeDone, InQoS, Subscriber](const bool bIsConnected)
		{
			FMqttifyTopicFilter TopicFilter{FString{kTopic}, InQoS};
			Subscriber->SubscribeAsync(MoveTemp(TopicFilter)).Next(
				[this, InBeforeDone](const TMqttifyResult<FMqttifySubscribeResult>& InResult)
				{
					if (!InResult.HasSucceeded())
					{
						AddError(TEXT("Failed to subscribe"));
						bTestDoneExecuted = true;
					}
					InBeforeDone.Execute();
				});
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

	const TSharedPtr<IMqttifyClient> Publisher = MqttClients[0];

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

		LOG_MQTTIFY(VeryVerbose, TEXT("Publishing message with payload %s"), *JSONSerializedData);
		LOG_MQTTIFY_PACKET_DATA(VeryVerbose, PayloadBytes.GetData(), PayloadBytes.Num());
		FMqttifyMessage Message{FString{kTopic}, MoveTemp(PayloadBytes), false, InQoS};

		PublishBuffer.Add(TestMessage);
		Publisher->PublishAsync(MoveTemp(Message)).Next(
			[this, TestMessage](const TMqttifyResult<void>& InResult)
			{
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
