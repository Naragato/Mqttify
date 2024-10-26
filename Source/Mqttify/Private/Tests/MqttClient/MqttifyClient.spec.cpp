#include "Mqtt/MqttifyConnectionSettingsBuilder.h"
#include "Tests/MqttifyTestUtilities.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "MqttifyModule.h"
#include "Misc/AutomationTest.h"
#include "Mqtt/MqttifyClient.h"

using namespace Mqttify;

BEGIN_DEFINE_SPEC(
	FMqttifyClientTests,
	"Mqttify.Automation.MqttifyClientTests",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

	static constexpr TCHAR kTopic[] = TEXT("/Test");
	bool bSubscribeSent = false;
	FString DockerContainerName = TEXT("vernemq_test_container");
	TSharedPtr<IMqttifyClient> MqttClientA = nullptr;
	TSharedPtr<IMqttifyClient> MqttClientB = nullptr;

	FDelegateHandle ClientAOnConnectHandle;
	FDelegateHandle ClientBOnConnectHandle;
	FOnSubscribe::FDelegate ClientBOnSubscribeDelegate;

	void SetupClients(const FMqttifyTestDockerSpec& Spec, const FDoneDelegate& BeforeDone);
	void OnClientConnect();
	void UnsubscribeClientB(const FDoneDelegate& TestDone);
	void PublishMessage(EMqttifyQualityOfService QualityOfService);
	void DisconnectClients(const FDoneDelegate& AfterDone) const;
	static void DisconnectClient(
		const TSharedPtr<IMqttifyClient>& MqttClient,
		const TFunction<void()>& OnDisconnectComplete
		);

END_DEFINE_SPEC(FMqttifyClientTests)

void FMqttifyClientTests::Define()
{
	FMqttifyTestDockerSpec Spec{8080, FindAvailablePort(5000, 10000), EMqttifyConnectionProtocol::Ws};

	Describe(
		"MQTT Client test",
		[this, Spec]()
		{
			LatentBeforeEach(
				[this, Spec](const FDoneDelegate& BeforeDone)
				{
					SetupClients(Spec, BeforeDone);
				});

			LatentIt(
				"Client B should unsubscribe",
				FTimespan::FromSeconds(120.0f),
				[this](const FDoneDelegate& TestDone)
				{
					UnsubscribeClientB(TestDone);
				});

			TArray QualityOfServices = {
				EMqttifyQualityOfService::AtLeastOnce,
				EMqttifyQualityOfService::ExactlyOnce,
				EMqttifyQualityOfService::AtMostOnce
			};

			for (const EMqttifyQualityOfService& QoS : QualityOfServices)
			{
				LatentIt(
					FString::Printf(TEXT("Client A should publish %s"), EnumToTCharString(QoS)),
					FTimespan::FromSeconds(120.0f),
					[this, QoS](const FDoneDelegate& TestDone)
					{
						MqttClientB->OnMessage().AddLambda(
							[this, TestDone](const FMqttifyMessage& InMessage)
							{
								LOG_MQTTIFY(VeryVerbose, TEXT("Received message on topic: %s"), *InMessage.GetTopic());
								LOG_MQTTIFY_PACKET_DATA(
									VeryVerbose,
									InMessage.GetPayload().GetData(),
									static_cast<uint32>(InMessage.GetPayload().Num()));
								TestDone.Execute();
							});
						PublishMessage(QoS);
					});
			}

			LatentAfterEach(
				[this](const FDoneDelegate& AfterDone)
				{
					bSubscribeSent = false;
					DisconnectClients(AfterDone);
				});
		});
}

void FMqttifyClientTests::SetupClients(const FMqttifyTestDockerSpec& Spec, const FDoneDelegate& BeforeDone)
{
	auto& MqttifyModule = IMqttifyModule::Get();

	// Build ConnectionSettingsA
	FMqttifyConnectionSettingsBuilder ConnectionSettingsBuilderA(
		FString::Printf(
			TEXT("%s://clientA:password@localhost:%d%s"),
			GetProtocolString(Spec.Protocol),
			Spec.PublicPort,
			GetPath(Spec.Protocol)));
	ConnectionSettingsBuilderA.SetClientId(TEXT("Test - ClientA"));
	const TSharedRef<FMqttifyConnectionSettings> ConnectionSettingsA = ConnectionSettingsBuilderA.Build().ToSharedRef();

	// Build ConnectionSettingsB
	FMqttifyConnectionSettingsBuilder ConnectionSettingsBuilderB(
		FString::Printf(
			TEXT("%s://clientB:password@localhost:%d%s"),
			GetProtocolString(Spec.Protocol),
			Spec.PublicPort,
			GetPath(Spec.Protocol)));
	ConnectionSettingsBuilderB.SetClientId(TEXT("Test - ClientB"));
	const TSharedRef<FMqttifyConnectionSettings> ConnectionSettingsB = ConnectionSettingsBuilderB.Build().ToSharedRef();

	// Create clients
	MqttClientA = MqttifyModule.GetOrCreateClient(ConnectionSettingsA);
	MqttClientB = MqttifyModule.GetOrCreateClient(ConnectionSettingsB);

	if (!MqttClientA || !MqttClientB)
	{
		AddError(TEXT("Failed to create MQTT clients."));
		BeforeDone.Execute();
		return;
	}

	// Start broker
	if (!StartBroker(DockerContainerName, Spec))
	{
		AddError(TEXT("Failed to start MQTT broker."));
		BeforeDone.Execute();
		return;
	}

	// Set up OnConnect handlers
	MqttClientA->OnConnect().AddLambda(
		[this](bool bIsConnected)
		{
			if (bIsConnected)
			{
				OnClientConnect();
			}
			else
			{
				AddError(TEXT("MqttClientA failed to connect."));
			}
		});

	MqttClientB->OnConnect().AddLambda(
		[this](bool bIsConnected)
		{
			if (bIsConnected)
			{
				OnClientConnect();
			}
			else
			{
				AddError(TEXT("MqttClientB failed to connect."));
			}
		});

	// Set up OnSubscribe handler
	ClientBOnSubscribeDelegate = FOnSubscribe::FDelegate::CreateLambda(
		[this, BeforeDone](const TSharedPtr<TArray<FMqttifySubscribeResult>>& InResult)
		{
			if (!InResult || InResult->IsEmpty() || InResult->Top().GetFilter().GetFilter() != kTopic)
			{
				AddError(TEXT("ClientB subscription failed."));
				BeforeDone.Execute();
				return;
			}

			LOG_MQTTIFY(VeryVerbose, TEXT("ClientB subscribed successfully."));
			BeforeDone.Execute();
		});
	MqttClientB->OnSubscribe().Add(ClientBOnSubscribeDelegate);

	// Connect clients
	MqttClientA->ConnectAsync(false);
	MqttClientB->ConnectAsync(false);
}

void FMqttifyClientTests::OnClientConnect()
{
	LOG_MQTTIFY(VeryVerbose, TEXT("On Client Connected"));
	if (MqttClientA && MqttClientB && MqttClientA->IsConnected() && MqttClientB->IsConnected() && !bSubscribeSent)
	{
		bSubscribeSent = true;
		MqttClientB->SubscribeAsync(FString(kTopic));
	}
}

void FMqttifyClientTests::UnsubscribeClientB(const FDoneDelegate& TestDone)
{
	MqttClientB->UnsubscribeAsync({FString(kTopic)}).Next(
		[this, TestDone](const TMqttifyResult<TArray<FMqttifyUnsubscribeResult>>& InResult)
		{
			const auto Result = InResult.GetResult();
			if (!Result.IsValid() || Result->IsEmpty())
			{
				AddError(TEXT("Unsubscribe failed: Result is null or empty."));
				TestDone.Execute();
				return;
			}

			const auto& UnsubscribeResult = Result->Top();
			TestTrue(TEXT("Unsubscribe should be successful"), UnsubscribeResult.WasSuccessful());
			TestEqual(TEXT("Topic should match"), UnsubscribeResult.GetFilter().GetFilter(), kTopic);
			TestDone.Execute();
		});
}

void FMqttifyClientTests::PublishMessage(EMqttifyQualityOfService QualityOfService)
{
	const TArray<uint8> Payload = {0, 1, 2, 3, 4};
	FMqttifyMessage Message{FString(kTopic), Payload, false, QualityOfService};

	MqttClientA->PublishAsync(MoveTemp(Message)).Next(
		[this](const TMqttifyResult<void>& InResult)
		{
			if (!InResult.HasSucceeded())
			{
				AddError(TEXT("Publish failed."));
			}
			else
			{
				LOG_MQTTIFY(VeryVerbose, TEXT("Publish succeeded."));
			}
		});
}

void FMqttifyClientTests::DisconnectClients(const FDoneDelegate& AfterDone) const
{
	DisconnectClient(
		MqttClientA,
		[this, AfterDone]()
		{
			LOG_MQTTIFY(VeryVerbose, TEXT("MqttClientA is disconnected, disconnecting B."));
			DisconnectClient(
				MqttClientB,
				[AfterDone]()
				{
					LOG_MQTTIFY(VeryVerbose, TEXT("MqttClientB is disconnected, completing test."));
					AfterDone.Execute();
				});
		});
}

void FMqttifyClientTests::DisconnectClient(
	const TSharedPtr<IMqttifyClient>& MqttClient,
	const TFunction<void()>& OnDisconnectComplete
	)
{
	if (MqttClient)
	{
		MqttClient->DisconnectAsync().Next(
			[OnDisconnectComplete](const TMqttifyResult<void>&)
			{
				OnDisconnectComplete();
			});
	}
	else
	{
		OnDisconnectComplete();
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS
