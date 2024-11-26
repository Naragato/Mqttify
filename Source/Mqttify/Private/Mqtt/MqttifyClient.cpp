#include "Mqtt/MqttifyClient.h"

#include "LogMqttify.h"
#include "MqttifyAsync.h"
#include "Commands/MqttifyPublish.h"
#include "Commands/MqttifySubscribe.h"
#include "Commands/MqttifyUnsubscribe.h"
#include "Interface/IMqttifyPacketReceiver.h"
#include "Interface/IMqttifySocketConnectedHandler.h"
#include "Interface/IMqttifySocketDisconnectHandler.h"
#include "Mqtt/MqttifyMessage.h"
#include "Mqtt/MqttifyTopicFilter.h"
#include "Mqtt/Interface/IMqttifyConnectableAsync.h"
#include "Mqtt/Interface/IMqttifyDisconnectableAsync.h"
#include "Mqtt/State/MqttifyClientDisconnectedState.h"
#include "Socket/Interface/IMqttifySocketTickable.h"

namespace Mqttify
{
	FMqttifyClient::FMqttifyClient(const FMqttifyConnectionSettingsRef& InConnectionSettings)
		: Context{MakeShared<FMqttifyClientContext>(InConnectionSettings)}
		, Socket{FMqttifySocketBase::Create(InConnectionSettings)}
	{
	}

	void FMqttifyClient::Tick()
	{
		FScopeLock Lock{&StateLock};
		if (!CurrentState.IsValid())
		{
			return;
		}
		if (IMqttifySocketTickable* Tickable = CurrentState->AsSocketTickable())
		{
			Tickable->Tick();
		}
	}

	TFuture<TMqttifyResult<void>> FMqttifyClient::ConnectAsync(const bool bCleanSession)
	{
		FScopeLock Lock{&StateLock};
		LOG_MQTTIFY(
			VeryVerbose,
			TEXT("[Connect (Connection %s, ClientId %s)]"),
			*GetConnectionSettings()->GetHost(),
			*GetConnectionSettings()->GetClientId());
		if (!CurrentState.IsValid())
		{
			TWeakPtr<FMqttifyClient> ThisWeakPtr = AsWeak();
			auto OnStateChanged = FMqttifyClientState::FOnStateChangedDelegate::CreateSP(
				this,
				&FMqttifyClient::TransitionTo);

			Socket->GetOnConnectDelegate().AddSP(this, &FMqttifyClient::OnSocketConnect);
			Socket->GetOnDisconnectDelegate().AddSP(this, &FMqttifyClient::OnSocketDisconnect);
			Socket->GetOnDataReceivedDelegate().AddSP(this, &FMqttifyClient::OnReceivePacket);
			CurrentState = MakeShared<FMqttifyClientDisconnectedState>(OnStateChanged, Context, Socket);
		}

		if (IMqttifyConnectableAsync* Connectable = CurrentState->AsConnectable())
		{
			return Connectable->ConnectAsync(bCleanSession);
		}

		return MakeThreadAwareFulfilledPromise<TMqttifyResult<void>>(TMqttifyResult<void>{false});
	}

	TFuture<TMqttifyResult<void>> FMqttifyClient::DisconnectAsync()
	{
		FScopeLock Lock{&StateLock};
		LOG_MQTTIFY(
			VeryVerbose,
			TEXT("[disconnect (Connection %s, ClientId %s)]"),
			*GetConnectionSettings()->GetHost(),
			*GetConnectionSettings()->GetClientId());

		if (CurrentState.IsValid())
		{
			if (IMqttifyDisconnectableAsync* Disconnectable = CurrentState->AsDisconnectable())
			{
				return Disconnectable->DisconnectAsync();
			}
		}

		return MakeThreadAwareFulfilledPromise<TMqttifyResult<void>>(TMqttifyResult<void>{false});
	}

	TFuture<TMqttifyResult<void>> FMqttifyClient::PublishAsync(FMqttifyMessage&& InMessage)
	{
		FScopeLock Lock{&StateLock};
		LOG_MQTTIFY(
        			Verbose,
        			TEXT("[Publishing (Connection %s, ClientId %s)] %s, QoS %s"),
        			*GetConnectionSettings()->GetHost(),
        			*GetConnectionSettings()->GetClientId(),
        			*InMessage.GetTopic(),
        			EnumToTCharString(InMessage.GetQualityOfService()));
		switch (InMessage.GetQualityOfService())
		{
		case EMqttifyQualityOfService::AtMostOnce:
			{
				const FMqttifyPubAtMostOnceRef PublishCommand = MakeShared<FMqttifyPubAtMostOnce, ESPMode::ThreadSafe>(
					MoveTemp(InMessage),
					Socket,
					Context->GetConnectionSettings());
				Context->AddOneShotCommand(PublishCommand);
				return PublishCommand->GetFuture();
			}
		case EMqttifyQualityOfService::AtLeastOnce:
			{
				const uint16 PacketId = Context->GetNextId();
				const FMqttifyPubAtLeastOnceRef PublishCommand = MakeShared<
					FMqttifyPubAtLeastOnce, ESPMode::ThreadSafe>(
					MoveTemp(InMessage),
					PacketId,
					Socket,
					Context->GetConnectionSettings());
				Context->AddAcknowledgeableCommand(PublishCommand);
				return PublishCommand->GetFuture();
			}
		case EMqttifyQualityOfService::ExactlyOnce:
			{
				const uint16 PacketId = Context->GetNextId();
				const FMqttifyPubExactlyOnceRef PublishCommand = MakeShared<
					FMqttifyPubExactlyOnce, ESPMode::ThreadSafe>(
					MoveTemp(InMessage),
					PacketId,
					Socket,
					Context->GetConnectionSettings());
				Context->AddAcknowledgeableCommand(PublishCommand);
				return PublishCommand->GetFuture();
			}
		default:
			{
				ensureMsgf(false, TEXT("Invalid quality of service"));
				return MakeThreadAwareFulfilledPromise<TMqttifyResult<void>>(TMqttifyResult<void>{false});
			}
		}
	}

	TFuture<TMqttifyResult<TArray<FMqttifySubscribeResult>>> FMqttifyClient::SubscribeAsync(
		const TArray<FMqttifyTopicFilter>& InTopicFilters
		)
	{
		const uint16 PacketId = Context->GetNextId();

		TArray<TTuple<FMqttifyTopicFilter, TSharedRef<FOnMessage>>> TopicFilters;
		for (const FMqttifyTopicFilter& TopicFilter : InTopicFilters)
		{
			auto Tuple = MakeTuple(TopicFilter, Context->GetMessageDelegate(TopicFilter.GetFilter()));
			TopicFilters.Emplace(Tuple);
		}

		const TSharedRef<FMqttifySubscribe> SubscribeCommand = MakeShared<FMqttifySubscribe>(
			TopicFilters,
			PacketId,
			Socket,
			Context->GetConnectionSettings());

		Context->AddAcknowledgeableCommand(SubscribeCommand);
		TWeakPtr<FMqttifyClientContext> WeakContext = Context;
		return SubscribeCommand->GetFuture().Next(
			[WeakContext](const TMqttifyResult<TArray<FMqttifySubscribeResult>>& InResult)
			{
				if (InResult.HasSucceeded())
				{
					if (const TSharedPtr<FMqttifyClientContext> ContextPtr = WeakContext.Pin())
					{
						ContextPtr->OnSubscribe().Broadcast(InResult.GetResult());
					}
				}
				else
				{
					LOG_MQTTIFY(Error, TEXT("Failed to subscribe to topic filter"));
				}

				return InResult;
			});
	}


	TFuture<TMqttifyResult<FMqttifySubscribeResult>> FMqttifyClient::SubscribeAsync(FMqttifyTopicFilter&& InTopicFilter)
	{
		return SubscribeAsync(TArray{InTopicFilter}).Next(
			[](const TMqttifyResult<TArray<FMqttifySubscribeResult>>& InResult)
			{
				if (InResult.HasSucceeded() && InResult.GetResult()->Num() == 1)
				{
					FMqttifySubscribeResult OutResult = InResult.GetResult()->operator[](0);
					return TMqttifyResult{true, MoveTemp(OutResult)};
				}

				LOG_MQTTIFY(
					Error,
					TEXT("Failed to subscribe to topic filter. Success: %s"),
					InResult.HasSucceeded() ? TEXT("true") : TEXT("false"));
				return TMqttifyResult<FMqttifySubscribeResult>{false};
			});
	}

	TFuture<TMqttifyResult<FMqttifySubscribeResult>> FMqttifyClient::SubscribeAsync(FString& InTopicFilter)
	{
		FMqttifyTopicFilter TopicFilter{InTopicFilter};
		return SubscribeAsync(MoveTemp(TopicFilter));
	}

	TFuture<TMqttifyResult<TArray<FMqttifyUnsubscribeResult>>> FMqttifyClient::UnsubscribeAsync(
		const TSet<FString>& InTopicFilters
		)
	{
		TArray<FMqttifyTopicFilter> TopicFilters;
		for (const FString& TopicFilter : InTopicFilters)
		{
			TopicFilters.Emplace(TopicFilter);
		}

		const uint16 PacketId = Context->GetNextId();
		const TSharedRef<FMqttifyUnsubscribe> UnsubscribeCommand = MakeShared<FMqttifyUnsubscribe>(
			TopicFilters,
			PacketId,
			Socket,
			Context->GetConnectionSettings());

		Context->AddAcknowledgeableCommand(UnsubscribeCommand);
		TWeakPtr<FMqttifyClientContext> WeakContext = Context;
		return UnsubscribeCommand->GetFuture().Next(
			[WeakContext](const TMqttifyResult<TArray<FMqttifyUnsubscribeResult>>& InResult)
			{
				if (InResult.HasSucceeded())
				{
					if (const TSharedPtr<FMqttifyClientContext> ContextPtr = WeakContext.Pin())
					{
						ContextPtr->OnUnsubscribe().Broadcast(InResult.GetResult());
						ContextPtr->ClearMessageDelegates(InResult.GetResult());
					}
				}
				else
				{
					LOG_MQTTIFY(Warning, TEXT("Failed to unsubscribe from topic filter"));
				}

				return InResult;
			});
	}

	FOnConnect& FMqttifyClient::OnConnect()
	{
		return Context->OnConnect();
	}

	FOnDisconnect& FMqttifyClient::OnDisconnect()
	{
		return Context->OnDisconnect();
	}

	FOnPublish& FMqttifyClient::OnPublish()
	{
		return Context->OnPublish();
	}

	FOnSubscribe& FMqttifyClient::OnSubscribe()
	{
		return Context->OnSubscribe();
	}

	FOnUnsubscribe& FMqttifyClient::OnUnsubscribe()
	{
		return Context->OnUnsubscribe();
	}

	FOnMessage& FMqttifyClient::OnMessage()
	{
		return Context->OnMessage();
	}

	const FMqttifyConnectionSettingsRef FMqttifyClient::GetConnectionSettings() const
	{
		return Context->GetConnectionSettings();
	}

	bool FMqttifyClient::IsConnected() const
	{
		FScopeLock Lock{&StateLock};
		return CurrentState->GetState() == EMqttifyState::Connected;
	}

	void FMqttifyClient::TransitionTo(
		const FMqttifyClientState* InPrevious,
		const TSharedPtr<FMqttifyClientState>& InState
		)
	{
		FScopeLock Lock{&StateLock};
		if (InPrevious != CurrentState.Get())
		{
			LOG_MQTTIFY(
				VeryVerbose,
				TEXT("Transitioning from a different state than the current state. Ignoring transition."));
			return;
		}
		LOG_MQTTIFY(
			Verbose,
			TEXT("(Connection %s, ClientId %s)  Client transitioning from %s to %s"),
			*GetConnectionSettings()->GetHost(),
			*GetConnectionSettings()->GetClientId(),
			EnumToTCharString(CurrentState->GetState()),
			EnumToTCharString(InState->GetState()));
		CurrentState = InState;
	}

	void FMqttifyClient::OnSocketConnect(const bool bWasSuccessful) const
	{
		FScopeLock Lock{&StateLock};
		LOG_MQTTIFY(
			VeryVerbose,
			TEXT("[On Connect (Connection %s, ClientId %s)]"),
			*GetConnectionSettings()->GetHost(),
			*GetConnectionSettings()->GetClientId());

		if (CurrentState.IsValid())
		{
			if (IMqttifySocketConnectedHandler* ConnectedHandler = CurrentState->AsSocketConnectedHandler())
			{
				ConnectedHandler->OnSocketConnect(bWasSuccessful);
			}
		}
	}

	void FMqttifyClient::OnSocketDisconnect() const
	{
		FScopeLock Lock{&StateLock};
		LOG_MQTTIFY(
			VeryVerbose,
			TEXT("[On Disconnect (Connection %s, ClientId %s)]"),
			*GetConnectionSettings()->GetHost(),
			*GetConnectionSettings()->GetClientId());

		if (CurrentState.IsValid())
		{
			if (IMqttifySocketDisconnectHandler* DisconnectHandler = CurrentState->AsSocketDisconnectHandler())
			{
				DisconnectHandler->OnSocketDisconnect();
			}
		}
	}

	void FMqttifyClient::OnReceivePacket(const TSharedPtr<FArrayReader>& InPacket) const
	{
		FScopeLock Lock{&StateLock};
		LOG_MQTTIFY(
			VeryVerbose,
			TEXT("[On Receive Packet (Connection %s, ClientId %s)]"),
			*GetConnectionSettings()->GetHost(),
			*GetConnectionSettings()->GetClientId());

		if (CurrentState.IsValid())
		{
			if (IMqttifyPacketReceiver* PacketReceiver = CurrentState->AsPacketReceiver())
			{
				PacketReceiver->OnReceivePacket(InPacket);
			}
		}
	}
} // namespace Mqttify
