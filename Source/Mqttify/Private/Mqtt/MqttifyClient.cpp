#include "Mqtt/MqttifyClient.h"

#include "LogMqttify.h"
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

		return MakeFulfilledPromise<TMqttifyResult<void>>(TMqttifyResult<void>{false}).GetFuture();
	}

	TFuture<TMqttifyResult<void>> FMqttifyClient::DisconnectAsync()
	{
		FScopeLock Lock{&StateLock};
		if (CurrentState.IsValid())
		{
			if (IMqttifyDisconnectableAsync* Disconnectable = CurrentState->AsDisconnectable())
			{
				return Disconnectable->DisconnectAsync();
			}
		}

		return MakeFulfilledPromise<TMqttifyResult<void>>(TMqttifyResult<void>{false}).GetFuture();
	}

	TFuture<TMqttifyResult<void>> FMqttifyClient::PublishAsync(FMqttifyMessage&& InMessage)
	{
		FScopeLock Lock{&StateLock};
		if (CurrentState.IsValid())
		{
			if (IMqttifyPublishableAsync* Publishable = CurrentState->AsPublishable())
			{
				return Publishable->PublishAsync(MoveTemp(InMessage));
			}
		}

		return MakeFulfilledPromise<TMqttifyResult<void>>(TMqttifyResult<void>{false}).GetFuture();
	}

	TFuture<TMqttifyResult<TArray<FMqttifySubscribeResult>>> FMqttifyClient::SubscribeAsync(
		const TArray<FMqttifyTopicFilter>& InTopicFilters
		)
	{
		FScopeLock Lock{&StateLock};
		if (CurrentState.IsValid())
		{
			if (IMqttifySubscribableAsync* Subscribable = CurrentState->AsSubscribable())
			{
				return Subscribable->SubscribeAsync(InTopicFilters);
			}
		}

		return MakeFulfilledPromise<TMqttifyResult<TArray<FMqttifySubscribeResult>>>(
			TMqttifyResult<TArray<FMqttifySubscribeResult>>{false}).GetFuture();
	}


	TFuture<TMqttifyResult<FMqttifySubscribeResult>> FMqttifyClient::SubscribeAsync(FMqttifyTopicFilter&& InTopicFilter)
	{
		FScopeLock Lock{&StateLock};
		if (CurrentState.IsValid())
		{
			if (IMqttifySubscribableAsync* Subscribable = CurrentState->AsSubscribable())
			{
				return Subscribable->SubscribeAsync(MoveTemp(InTopicFilter));
			}
		}
		return MakeFulfilledPromise<TMqttifyResult<FMqttifySubscribeResult>>(
			TMqttifyResult<FMqttifySubscribeResult>{false}).GetFuture();
	}

	TFuture<TMqttifyResult<FMqttifySubscribeResult>> FMqttifyClient::SubscribeAsync(FString& InTopicFilter)
	{
		FScopeLock Lock{&StateLock};
		if (CurrentState.IsValid())
		{
			if (IMqttifySubscribableAsync* Subscribable = CurrentState->AsSubscribable())
			{
				return Subscribable->SubscribeAsync(InTopicFilter);
			}
		}

		return MakeFulfilledPromise<TMqttifyResult<FMqttifySubscribeResult>>(
			TMqttifyResult<FMqttifySubscribeResult>{false}).GetFuture();
	}

	TFuture<TMqttifyResult<TArray<FMqttifyUnsubscribeResult>>> FMqttifyClient::UnsubscribeAsync(
		const TSet<FString>& InTopicFilters
		)
	{
		FScopeLock Lock{&StateLock};
		if (CurrentState.IsValid())
		{
			if (IMqttifyUnsubscribableAsync* Unsubscribable = CurrentState->AsUnsubscribable())
			{
				return Unsubscribable->UnsubscribeAsync(InTopicFilters);
			}
		}
		return MakeFulfilledPromise<TMqttifyResult<TArray<FMqttifyUnsubscribeResult>>>(
			TMqttifyResult<TArray<FMqttifyUnsubscribeResult>>{false}).GetFuture();
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

	void FMqttifyClient::TransitionTo(const TSharedPtr<FMqttifyClientState>& InState)
	{
		FScopeLock Lock{&StateLock};
		LOG_MQTTIFY(
			Verbose,
			TEXT("Client transitioning from %s to %s"),
			EnumToTCharString(CurrentState->GetState()),
			EnumToTCharString(InState->GetState()));
		CurrentState = InState;
	}

	void FMqttifyClient::OnSocketConnect(bool bWasSuccessful) const
	{
		FScopeLock Lock{&StateLock};
		if (CurrentState.IsValid())
		{
			if (IMqttifySocketConnectedHandler* ConnectedHandler = CurrentState->AsSocketConnectedHandler())
			{
				ConnectedHandler->OnSocketConnect(bWasSuccessful);
				return;
			}
		}

		LOG_MQTTIFY(
			Warning,
			TEXT("No connected handler found current %s"),
			EnumToTCharString(CurrentState->GetState()));
	}

	void FMqttifyClient::OnSocketDisconnect() const
	{
		FScopeLock Lock{&StateLock};
		if (CurrentState.IsValid())
		{
			if (IMqttifySocketDisconnectHandler* DisconnectHandler = CurrentState->AsSocketDisconnectHandler())
			{
				DisconnectHandler->OnSocketDisconnect();
				return;
			}
		}
		LOG_MQTTIFY(
			Warning,
			TEXT("No disconnected handler found current %s"),
			EnumToTCharString(CurrentState->GetState()));
	}

	void FMqttifyClient::OnReceivePacket(const TSharedPtr<FArrayReader>& InPacket) const
	{
		FScopeLock Lock{&StateLock};
		if (CurrentState.IsValid())
		{
			if (IMqttifyPacketReceiver* PacketReceiver = CurrentState->AsPacketReceiver())
			{
				PacketReceiver->OnReceivePacket(InPacket);
				return;
			}
		}
		LOG_MQTTIFY(
			Warning,
			TEXT("No Packet Receiver handler found current %s"),
			EnumToTCharString(CurrentState->GetState()));
	}
} // namespace Mqttify
