#include "Mqtt/MqttifyClient.h"

#include "LogMqttify.h"
#include "MqttifyAsync.h"
#include "Interface/IMqttifyPacketReceiver.h"
#include "Interface/IMqttifySocketConnectedHandler.h"
#include "Interface/IMqttifySocketDisconnectHandler.h"
#include "Mqtt/MqttifyMessage.h"
#include "Mqtt/MqttifyTopicFilter.h"
#include "Mqtt/Interface/IMqttifyConnectableAsync.h"
#include "Mqtt/Interface/IMqttifyDisconnectableAsync.h"
#include "Mqtt/Interface/IMqttifyPublishableAsync.h"
#include "Mqtt/Interface/IMqttifySubscribableAsync.h"
#include "Mqtt/Interface/IMqttifyUnsubscribableAsync.h"
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
			VeryVerbose,
			TEXT("[lock (Connection %s, ClientId %s)]"),
			*GetConnectionSettings()->GetHost(),
			*GetConnectionSettings()->GetClientId());
		if (CurrentState.IsValid())
		{
			if (IMqttifyPublishableAsync* Publishable = CurrentState->AsPublishable())
			{
				LOG_MQTTIFY(
					VeryVerbose,
					TEXT("[unlock (Connection %s, ClientId %s)] is in a publishable state"),
					*GetConnectionSettings()->GetHost(),
					*GetConnectionSettings()->GetClientId());
				return Publishable->PublishAsync(MoveTemp(InMessage));
			}
		}

		LOG_MQTTIFY(
			VeryVerbose,
			TEXT("[unlock (Connection %s, ClientId %s)] is not in a publishable state"),
			*GetConnectionSettings()->GetHost(),
			*GetConnectionSettings()->GetClientId());
		return MakeThreadAwareFulfilledPromise<TMqttifyResult<void>>(TMqttifyResult<void>{false});
	}

	TFuture<TMqttifyResult<TArray<FMqttifySubscribeResult>>> FMqttifyClient::SubscribeAsync(
		const TArray<FMqttifyTopicFilter>& InTopicFilters
		)
	{
		FScopeLock Lock{&StateLock};
		LOG_MQTTIFY(
			VeryVerbose,
			TEXT("[lock (Connection %s, ClientId %s)]"),
			*GetConnectionSettings()->GetHost(),
			*GetConnectionSettings()->GetClientId());
		if (CurrentState.IsValid())
		{
			if (IMqttifySubscribableAsync* Subscribable = CurrentState->AsSubscribable())
			{
				LOG_MQTTIFY(
					VeryVerbose,
					TEXT("[unlock (Connection %s, ClientId %s)] is in a subscribable state"),
					*GetConnectionSettings()->GetHost(),
					*GetConnectionSettings()->GetClientId());
				return Subscribable->SubscribeAsync(InTopicFilters);
			}
		}

		LOG_MQTTIFY(
			VeryVerbose,
			TEXT("[unlock (Connection %s, ClientId %s)] is not in a subscribable state"),
			*GetConnectionSettings()->GetHost(),
			*GetConnectionSettings()->GetClientId());
		return MakeThreadAwareFulfilledPromise<TMqttifyResult<TArray<FMqttifySubscribeResult>>>(
			TMqttifyResult<TArray<FMqttifySubscribeResult>>{false});
	}


	TFuture<TMqttifyResult<FMqttifySubscribeResult>> FMqttifyClient::SubscribeAsync(FMqttifyTopicFilter&& InTopicFilter)
	{
		FScopeLock Lock{&StateLock};
		LOG_MQTTIFY(
			VeryVerbose,
			TEXT("[lock (Connection %s, ClientId %s)]"),
			*GetConnectionSettings()->GetHost(),
			*GetConnectionSettings()->GetClientId());
		if (CurrentState.IsValid())
		{
			if (IMqttifySubscribableAsync* Subscribable = CurrentState->AsSubscribable())
			{
				LOG_MQTTIFY(
					VeryVerbose,
					TEXT("[unlock (Connection %s, ClientId %s)] is in a subscribable state"),
					*GetConnectionSettings()->GetHost(),
					*GetConnectionSettings()->GetClientId());
				return Subscribable->SubscribeAsync(MoveTemp(InTopicFilter));
			}
		}

		LOG_MQTTIFY(
			VeryVerbose,
			TEXT("[unlock (Connection %s, ClientId %s)] is not in a subscribable state"),
			*GetConnectionSettings()->GetHost(),
			*GetConnectionSettings()->GetClientId());
		return MakeThreadAwareFulfilledPromise<TMqttifyResult<FMqttifySubscribeResult>>(
			TMqttifyResult<FMqttifySubscribeResult>{false});
	}

	TFuture<TMqttifyResult<FMqttifySubscribeResult>> FMqttifyClient::SubscribeAsync(FString& InTopicFilter)
	{
		FScopeLock Lock{&StateLock};
		LOG_MQTTIFY(
			VeryVerbose,
			TEXT("[lock (Connection %s, ClientId %s)]"),
			*GetConnectionSettings()->GetHost(),
			*GetConnectionSettings()->GetClientId());
		if (CurrentState.IsValid())
		{
			if (IMqttifySubscribableAsync* Subscribable = CurrentState->AsSubscribable())
			{
				LOG_MQTTIFY(
					VeryVerbose,
					TEXT("[unlock (Connection %s, ClientId %s)] is in a subscribable state"),
					*GetConnectionSettings()->GetHost(),
					*GetConnectionSettings()->GetClientId());
				return Subscribable->SubscribeAsync(InTopicFilter);
			}
		}

		LOG_MQTTIFY(
			VeryVerbose,
			TEXT("[unlock (Connection %s, ClientId %s)] is not in a subscribable state"),
			*GetConnectionSettings()->GetHost(),
			*GetConnectionSettings()->GetClientId());
		return MakeThreadAwareFulfilledPromise<TMqttifyResult<FMqttifySubscribeResult>>(
			TMqttifyResult<FMqttifySubscribeResult>{false});
	}

	TFuture<TMqttifyResult<TArray<FMqttifyUnsubscribeResult>>> FMqttifyClient::UnsubscribeAsync(
		const TSet<FString>& InTopicFilters
		)
	{
		FScopeLock Lock{&StateLock};
		LOG_MQTTIFY(
			VeryVerbose,
			TEXT("[lock (Connection %s, ClientId %s)]"),
			*GetConnectionSettings()->GetHost(),
			*GetConnectionSettings()->GetClientId());
		if (CurrentState.IsValid())
		{
			if (IMqttifyUnsubscribableAsync* Unsubscribable = CurrentState->AsUnsubscribable())
			{
				LOG_MQTTIFY(
					VeryVerbose,
					TEXT("[unlock (Connection %s, ClientId %s)] is in an unsubscribable state"),
					*GetConnectionSettings()->GetHost(),
					*GetConnectionSettings()->GetClientId());
				return Unsubscribable->UnsubscribeAsync(InTopicFilters);
			}
		}


		return MakeThreadAwareFulfilledPromise<TMqttifyResult<TArray<FMqttifyUnsubscribeResult>>>(
			TMqttifyResult<TArray<FMqttifyUnsubscribeResult>>{false});
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
				Warning,
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
