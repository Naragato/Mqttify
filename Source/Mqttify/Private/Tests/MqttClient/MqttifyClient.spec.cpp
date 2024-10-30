#if WITH_DEV_AUTOMATION_TESTS

#include "MqttifyModule.h"
#include "Misc/AutomationTest.h"
#include "Mqtt/MqttifyClient.h"
#include "Mqtt/MqttifyConnectionSettingsBuilder.h"
#include "Tests/MqttifyTestUtilities.h"
#include "Tests/Packets/PacketComparision.h"

using namespace Mqttify;

BEGIN_DEFINE_SPEC(
	FMqttifyClientTests,
	"Mqttify.Automation.MqttifyClientTests",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

	static constexpr TCHAR kTopic[] = TEXT("/Test");
	FString DockerContainerName = TEXT("vernemq_test_container");
	TArray<TSharedPtr<IMqttifyClient>> MqttClients;

	void SetupClients(const FMqttifyTestDockerSpec& InSpec, const FDoneDelegate& InBeforeDone, int32 InNumClients);
	void DisconnectClients(const FDoneDelegate& InAfterDone) const;

END_DEFINE_SPEC(FMqttifyClientTests)

void FMqttifyClientTests::Define()
{
	FMqttifyTestDockerSpec Spec{8080, FindAvailablePort(5000, 10000), EMqttifyConnectionProtocol::Ws};

	TArray QualityOfServices = {
		EMqttifyQualityOfService::AtMostOnce,
		EMqttifyQualityOfService::AtLeastOnce,
		EMqttifyQualityOfService::ExactlyOnce
	};

	Describe(
		"MQTT single client test",
		[this, Spec, QualityOfServices]()
		{
			LatentBeforeEach(
				[this, Spec](const FDoneDelegate& BeforeDone)
				{
					if (!StartBroker(DockerContainerName, Spec))
					{
						AddError(TEXT("Failed to start MQTT broker"));
						BeforeDone.Execute();
						return;
					}
					SetupClients(Spec, BeforeDone, 1);
				});

			for (const EMqttifyQualityOfService& QoS : QualityOfServices)
			{
				LatentIt(
					FString::Printf(TEXT("PublishAsync.Next is called for QoS %s"), EnumToTCharString(QoS)),
					FTimespan::FromSeconds(120.0f),
					[this, QoS](const FDoneDelegate& TestDone)
					{
						const TSharedPtr<IMqttifyClient> Client = MqttClients.Top();

						FMqttifyMessage Message{FString{kTopic}, TArray<uint8>{1, 2, 3}, false, QoS};
						Client->PublishAsync(MoveTemp(Message)).Next(
							[this, TestDone](const TMqttifyResult<void>& InResult)
							{
								Async(
									EAsyncExecution::TaskGraphMainThread,
									[InResult, TestDone, this]
									{
										TestTrue(TEXT("PublishAsync.Next should be called"), InResult.HasSucceeded());
										TestDone.Execute();
									});
							});
					});
			}

			LatentIt(
				TEXT("SubscribeAsync.Next is called for FString&& InTopicFilter"),
				FTimespan::FromSeconds(120.0f),
				[this](const FDoneDelegate& TestDone)
				{
					const TSharedPtr<IMqttifyClient> Client = MqttClients.Top();
					Client->SubscribeAsync(FString{kTopic}).Next(
						[this, TestDone](const TMqttifyResult<FMqttifySubscribeResult>& InResult)
						{
							TestTrue(TEXT("SubscribeAsync.Next should be called"), InResult.HasSucceeded());
							TestDone.Execute();
						});
				});

			LatentIt(
				TEXT("SubscribeAsync.Next is called for FMqttifyTopicFilter&& InTopicFilter"),
				FTimespan::FromSeconds(120.0f),
				[this](const FDoneDelegate& TestDone)
				{
					const TSharedPtr<IMqttifyClient> Client = MqttClients.Top();
					FMqttifyTopicFilter TopicFilter{FString{kTopic}};
					Client->SubscribeAsync(MoveTemp(TopicFilter)).Next(
						[this, TestDone](const TMqttifyResult<FMqttifySubscribeResult>& InResult)
						{
							TestTrue(TEXT("SubscribeAsync.Next should be called"), InResult.HasSucceeded());
							TestDone.Execute();
						});
				});

			LatentIt(
				TEXT("SubscribeAsync.Next is called for const TArray<FMqttifyTopicFilter>& InTopicFilters"),
				FTimespan::FromSeconds(120.0f),
				[this](const FDoneDelegate& TestDone)
				{
					const TSharedPtr<IMqttifyClient> Client = MqttClients.Top();
					const TArray TopicFilters = {FMqttifyTopicFilter{FString{kTopic}}};
					Client->SubscribeAsync(TopicFilters).Next(
						[this, TestDone](const TMqttifyResult<TArray<FMqttifySubscribeResult>>& InResult)
						{
							TestTrue(TEXT("SubscribeAsync.Next should be called"), InResult.HasSucceeded());
							TestDone.Execute();
						});
				});

			LatentIt(
				TEXT("UnsubscribeAsync.Next is called for const TSet<FString>& InTopicFilters"),
				FTimespan::FromSeconds(120.0f),
				[this](const FDoneDelegate& TestDone)
				{
					const TSharedPtr<IMqttifyClient> Client = MqttClients[0];
					const TSet TopicFilters = {FString{kTopic}};
					Client->UnsubscribeAsync(TopicFilters).Next(
						[this, TestDone](const TMqttifyResult<TArray<FMqttifyUnsubscribeResult>>& InResult)
						{
							TestTrue(TEXT("UnsubscribeAsync.Next should be called"), InResult.HasSucceeded());
							TestDone.Execute();
						});
				});

			LatentAfterEach(
				[this](const FDoneDelegate& AfterDone)
				{
					DisconnectClients(AfterDone);
				});
		});

	Describe(
		TEXT("MQTT multi client test"),
		[this, Spec, QualityOfServices]
		{
			LatentBeforeEach(
				[this, Spec](const FDoneDelegate& BeforeDone)
				{
					if (!StartBroker(DockerContainerName, Spec))
					{
						AddError(TEXT("Failed to start MQTT broker"));
						BeforeDone.Execute();
						return;
					}
					SetupClients(Spec, BeforeDone, 2);
				});

			for (const EMqttifyQualityOfService& PublisherQoS : QualityOfServices)
			{
				for (const EMqttifyQualityOfService& SubscriberQoS : QualityOfServices)
				{
					LatentIt(
						FString::Printf(
							TEXT("Publisher's %s messages are received by a Subscriber %s client"),
							EnumToTCharString(PublisherQoS),
							EnumToTCharString(SubscriberQoS)),
						FTimespan::FromSeconds(120.0f),
						[this, Spec, SubscriberQoS, PublisherQoS](const FDoneDelegate& TestDone)
						{
							if (MqttClients.Num() != 2)
							{
								AddError(TEXT("Expected 2 client pointers"));
								TestDone.Execute();
								return;
							}
							TSharedPtr<IMqttifyClient> Publisher = MqttClients[0];
							TSharedPtr<IMqttifyClient> Subscriber = MqttClients[1];
							FMqttifyTopicFilter TopicFilter{FString{kTopic}, SubscriberQoS};
							Subscriber->SubscribeAsync(MoveTemp(TopicFilter)).Next(
								[this, Publisher, Subscriber, TestDone, PublisherQoS](
								const TMqttifyResult<FMqttifySubscribeResult>& InResult
								)
								{
									if (!InResult.HasSucceeded())
									{
										AddError(TEXT("SubscribeAsync failed"));
										TestDone.Execute();
										return;
									}

									TArray<int> X = {1, 2, 3};
									Subscriber->OnMessage().AddLambda(
										[this, TestDone](const FMqttifyMessage& InMessage)
										{
											LOG_MQTTIFY(VeryVerbose, TEXT("Received message"));
											LOG_MQTTIFY_PACKET_DATA(
												VeryVerbose,
												InMessage.GetPayload().GetData(),
												InMessage.GetPayload().Num());
											TestEqual(
												TEXT("Received message topic matches"),
												InMessage.GetTopic(),
												FString{kTopic});

											TestArrayEqual(
												TEXT("Received message payload matches"),
												InMessage.GetPayload(),
												TArray<uint8>{1, 2, 3},
												this);
											TestDone.Execute();
										});

									FMqttifyMessage Message{
										FString{kTopic},
										TArray<uint8>{1, 2, 3},
										false,
										PublisherQoS
									};
									Publisher->PublishAsync(MoveTemp(Message));
								});
						});
				}
			}

			LatentAfterEach(
				[this](const FDoneDelegate& AfterDone)
				{
					DisconnectClients(AfterDone);
				});
		});
}

void FMqttifyClientTests::SetupClients(
	const FMqttifyTestDockerSpec& InSpec,
	const FDoneDelegate& InBeforeDone,
	const int32 InNumClients
	)
{
	auto& MqttifyModule = IMqttifyModule::Get();
	MqttClients.Empty();
	for (int32 i = 0; i < InNumClients; ++i)
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

void FMqttifyClientTests::DisconnectClients(const FDoneDelegate& InAfterDone) const
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

#endif // WITH_DEV_AUTOMATION_TESTS
