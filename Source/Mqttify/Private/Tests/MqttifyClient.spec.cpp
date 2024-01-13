#if WITH_AUTOMATION_TESTS
#include "MqttifyModule.h"
#include "Mqtt/MqttifyClient.h"

namespace Mqttify
{
	BEGIN_DEFINE_SPEC(FMqttifyClientTests,
					"Mqttify.Automation.MqttifyClientTests",
					EAutomationTestFlags::EngineFilter | EAutomationTestFlags::ApplicationContextMask)

		static constexpr TCHAR kTopic[] = TEXT("/Test");
		TSharedPtr<IMqttifyClient> MqttClientA = nullptr;
		TSharedPtr<IMqttifyClient> MqttClientB = nullptr;

		FDelegateHandle ClientAOnConnectHandle;
		FDelegateHandle ClientBOnConnectHandle;
		FOnSubscribe::FDelegate ClientBOnSubscribeDelegate;
		FOnMessage::FDelegate ClientBOnMessageDelegate;

		void OnClientConnect(const bool) const
		{
			if (nullptr == MqttClientA)
			{
				LOG_MQTTIFY(Error, TEXT("MqttClientA is null."));
				return;
			}

			if (nullptr == MqttClientB)
			{
				LOG_MQTTIFY(Error, TEXT("MqttClientB is null."));
				return;
			}

			if (!MqttClientA->IsConnected())
			{
				LOG_MQTTIFY(Warning, TEXT("MqttClientA is not connected."));
				return;
			}

			if (!MqttClientB->IsConnected())
			{
				LOG_MQTTIFY(Warning, TEXT("MqttClientB is not connected."));
				return;
			}

			MqttClientB->SubscribeAsync(FString(kTopic));
		}

	END_DEFINE_SPEC(FMqttifyClientTests)

	void FMqttifyClientTests::Define()
	{
		LatentBeforeEach([this](const FDoneDelegate& Done)
			{
				if (!FModuleManager::Get().IsModuleLoaded(TEXT("Mqttify")))
				{
					//NOTE: This module gets left in after the test completes otherwise the content browser would crash when it tries to access the created BehaviorTree.
					FModuleManager::Get().LoadModule(TEXT("Mqttify"));
				}

				if (FMqttifyModule* MqttifyModule = static_cast<FMqttifyModule*>(FMqttifyModule::Get()))
				{
					MqttClientA = MqttifyModule->GetOrCreateClient(
						FString(TEXT("mqtt://clientA:password@localhost:1883")));
					MqttClientB = MqttifyModule->GetOrCreateClient(
						FString(TEXT("mqtt://clientB:password@localhost:1883")));
				}

				if (nullptr != MqttClientA)
				{
					ClientAOnConnectHandle = MqttClientA->OnConnect().AddRaw(
						this,
						&FMqttifyClientTests::OnClientConnect);

					MqttClientA->ConnectAsync();
				}

				if (nullptr != MqttClientB)
				{
					ClientBOnConnectHandle = MqttClientB->OnConnect().AddRaw(
						this,
						&FMqttifyClientTests::OnClientConnect);

					ClientBOnSubscribeDelegate = FOnSubscribe::FDelegate::CreateLambda(
						[this, Done](const TSharedPtr<TArray<FMqttifySubscribeResult>>& InResult)
						{
							if (!InResult->IsEmpty() && InResult->Top().GetFilter().GetFilter() == kTopic)
							{
								Done.Execute();
							}
						});

					MqttClientB->OnSubscribe().Add(ClientBOnSubscribeDelegate);
					MqttClientB->ConnectAsync();
				}
			}
		);

		LatentAfterEach([this](const FDoneDelegate& Done)
		{
			auto DisconnectCallback = [Done, this](const TFuture<TMqttifyResult<void>>&)
			{
				if (nullptr != MqttClientA
					&& !MqttClientA->IsConnected()
					&& nullptr != MqttClientB
					&& !MqttClientB->IsConnected())
				{
					Done.Execute();
				}
			};

			if (nullptr != MqttClientA)
			{
				MqttClientA->DisconnectAsync().Then(DisconnectCallback);
			}
			if (nullptr != MqttClientB)
			{
				MqttClientB->DisconnectAsync().Then(DisconnectCallback);
			}
		});

		Describe(TEXT("When MQTT Client connected"),
				[this]()
				{
					LatentIt(TEXT("Client B should unsubscribe"),
							[this](const FDoneDelegate& TestDone)
							{
								MqttClientB->UnsubscribeAsync({FString(kTopic)}).Then(
									[this, TestDone](
									const TFuture<TMqttifyResult<TArray<FMqttifyUnsubscribeResult>>>& InFuture)
									{
										const TSharedPtr<TArray<FMqttifyUnsubscribeResult>> Result = InFuture.Get().
																											GetResult();
										if (nullptr != Result
											&& !Result->IsEmpty()
											&& Result->Top().WasSuccessful()
											&& Result->Top().GetFilter().GetFilter() == kTopic)
										{
											TestDone.Execute();
										}
									});
							});

					LatentIt(TEXT("Client A should successfully send a message to Client B"),
							[this](const FDoneDelegate& TestDone)
							{
								const TArray<uint8> Payload = TArray<uint8>{0, 1, 2, 3, 4};
								ClientBOnMessageDelegate
									= FOnMessage::FDelegate::CreateLambda(
										[this, TestDone, Payload](const FMqttifyMessage& InMessage)
										{
											TestEqual("Topic should match", InMessage.GetTopic(), kTopic);
											TestEqual("Payload should match", InMessage.GetPayload(), Payload);
											TestDone.Execute();
										});

								MqttClientB->OnMessage().Add(ClientBOnMessageDelegate);
								MqttClientA->PublishAsync(FMqttifyMessage{
									FString{kTopic}, TArray<uint8>{Payload}, false, EMqttifyQualityOfService::AtMostOnce
								});
							});
				});
	}
}

#endif // WITH_AUTOMATION_TESTS
