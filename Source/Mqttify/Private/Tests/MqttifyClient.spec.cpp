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
				LOG_MQTTIFY(Verbose, TEXT("MqttClientA is not connected."));
				return;
			}

			if (!MqttClientB->IsConnected())
			{
				LOG_MQTTIFY(Verbose, TEXT("MqttClientB is not connected."));
				return;
			}

			MqttClientB->SubscribeAsync(FString(kTopic));
		}

	END_DEFINE_SPEC(FMqttifyClientTests)

	void FMqttifyClientTests::Define()
	{
		Describe(TEXT("When MQTT Client connected"),
				[this]
				{
					LatentBeforeEach([this](const FDoneDelegate& BeforeDone)
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

							if (nullptr == MqttClientA)
							{
								LOG_MQTTIFY(Error, TEXT("MqttClientA is nullptr."));
								BeforeDone.Execute();
								return;
							}

							if (nullptr == MqttClientB)
							{
								LOG_MQTTIFY(Error, TEXT("MqttClientB is nullptr."));
								BeforeDone.Execute();
								return;
							}

							ClientAOnConnectHandle = MqttClientA->OnConnect().AddRaw(
								this,
								&FMqttifyClientTests::OnClientConnect);

							ClientBOnConnectHandle = MqttClientB->OnConnect().AddRaw(
								this,
								&FMqttifyClientTests::OnClientConnect);

							ClientBOnSubscribeDelegate = FOnSubscribe::FDelegate::CreateLambda(
								[this, BeforeDone](const TSharedPtr<TArray<FMqttifySubscribeResult>>& InResult)
								{
									if (nullptr == InResult)
									{
										LOG_MQTTIFY(Error, TEXT("InResult is nullptr."));
										BeforeDone.Execute();
										return;
									}

									if (InResult->IsEmpty())
									{
										LOG_MQTTIFY(Error, TEXT("InResult is empty."));
									}

									if (!InResult->IsEmpty() && InResult->Top().GetFilter().GetFilter() != kTopic)
									{
										LOG_MQTTIFY(Error, TEXT("InResult contains wrong topic."));
									}
									BeforeDone.Execute();
								});
							MqttClientB->OnSubscribe().Add(ClientBOnSubscribeDelegate);

							MqttClientA->ConnectAsync();
							MqttClientB->ConnectAsync();
						}
					);

					LatentIt("Client B should unsubscribe",
							[this](const FDoneDelegate& TestDone)
							{
								MqttClientB->UnsubscribeAsync({FString(kTopic)}).Then(
									[this, TestDone](
									const TFuture<TMqttifyResult<TArray<FMqttifyUnsubscribeResult>>>& InFuture)
									{
										const TSharedPtr<TArray<FMqttifyUnsubscribeResult>> Result = InFuture.Get().
																											GetResult();
										TestValid(TEXT("Result should not be null"), Result);
										TestFalse(TEXT("Result should be empty"), Result->IsEmpty());
										TestEqual(TEXT("Result should have 1 element"), Result->Num(), 1);
										TestTrue(TEXT("Result should be successful"), Result->Top().WasSuccessful());
										TestEqual(TEXT("Topic should match"),
												Result->Top().GetFilter().GetFilter(),
												kTopic);
										TestDone.Execute();
									});
							});

					LatentIt(TEXT("Client A should successfully send a message to Client B  QoS AtMostOnce"),
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

					LatentIt(TEXT("Client A should successfully send a message AtLeastOnce"),
							[this](const FDoneDelegate& TestDone)
							{
								const TArray<uint8> Payload = TArray<uint8>{0, 1, 2, 3, 4};
								MqttClientA->PublishAsync(FMqttifyMessage{
									FString{kTopic}, TArray<uint8>{Payload}, false,
									EMqttifyQualityOfService::AtLeastOnce
								}).Then([this, TestDone](const TFuture<TMqttifyResult<void>>& InFuture)
								{
									TestTrue(TEXT("Future should have result"), InFuture.IsReady());
									TestTrue(TEXT("Future should have be successful"), InFuture.Get().HasSucceeded());
									TestDone.Execute();
								});
							});

					LatentIt(TEXT("Client A should successfully send a message ExactlyOnce"),
							[this](const FDoneDelegate& TestDone)
							{
								const TArray<uint8> Payload = TArray<uint8>{0, 1, 2, 3, 4};

								MqttClientA->PublishAsync(FMqttifyMessage{
									FString{kTopic}, TArray<uint8>{Payload}, false,
									EMqttifyQualityOfService::ExactlyOnce
								}).Then([this, TestDone](const TFuture<TMqttifyResult<void>>& InFuture)
								{
									TestTrue(TEXT("Future should have result"), InFuture.IsReady());
									TestTrue(TEXT("Future should have be successful"), InFuture.Get().HasSucceeded());
									TestDone.Execute();
								});
							});

					AfterEach([this]()
					{
						LOG_MQTTIFY(Verbose, TEXT("FMqttifyClientTests::LatentAfterEach"));
						if (nullptr == MqttClientA)
						{
							LOG_MQTTIFY(Error, TEXT("MqttClientA is nullptr."));
							return;
						}

						if (nullptr == MqttClientB)
						{
							LOG_MQTTIFY(Error, TEXT("MqttClientB is nullptr."));
							return;
						}

						MqttClientA->DisconnectAsync();
						MqttClientB->DisconnectAsync();
					});
				});
	}
}

#endif // WITH_AUTOMATION_TESTS
