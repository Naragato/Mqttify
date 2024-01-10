#include "Mqtt/MqttifyClient.h"

#include "LogMqttify.h"
#include "Mqtt/MqttifyMessage.h"
#include "Mqtt/MqttifyTopicFilter.h"
#include "Mqtt/Interface/IMqttifyConnectableAsync.h"
#include "Mqtt/Interface/IMqttifyDisconnectableAsync.h"
#include "Mqtt/Interface/IMqttifyPublishableAsync.h"
#include "Mqtt/Interface/IMqttifySubscribableAsync.h"
#include "Mqtt/Interface/IMqttifyUnsubscribableAsync.h"
#include "Mqtt/State/MqttifyClientDisconnectedState.h"
#include "Socket/SocketState/IMqttifySocketTickable.h"

namespace Mqttify
{
	FMqttifyClient::FMqttifyClient(const FMqttifyConnectionSettingsRef& InConnectionSettings)
		: Context{MakeShared<FMqttifyClientContext>(InConnectionSettings)}
	{
		auto OnStateChanged = FMqttifyClientState::FOnStateChangedDelegate::CreateLambda(
			[this](TUniquePtr<FMqttifyClientState>&& InState)
			{
				this->TransitionTo(MoveTemp(InState));
			});

		CurrentState = MakeUnique<FMqttifyDisconnectedState>(OnStateChanged, Context);
	}

	void FMqttifyClient::Tick()
	{
		if (IMqttifySocketTickable* Tickable = CurrentState->AsSocketTickable())
		{
			Tickable->Tick();
		}
	}

	void FMqttifyClient::UpdatePassword(const FString& InPassword)
	{
		Context->UpdatePassword(InPassword);
	}

	TFuture<TMqttifyResult<void>> FMqttifyClient::ConnectAsync(const bool bCleanSession)
	{
		if (IMqttifyConnectableAsync* Connectable = CurrentState->AsConnectable())
		{
			return Connectable->ConnectAsync(bCleanSession);
		}

		return MakeFulfilledPromise<TMqttifyResult<void>>(TMqttifyResult<void>{false}).GetFuture();
	}

	TFuture<TMqttifyResult<void>> FMqttifyClient::DisconnectAsync()
	{
		if (IMqttifyDisconnectableAsync* Disconnectable = CurrentState->AsDisconnectable())
		{
			return Disconnectable->DisconnectAsync();
		}

		return MakeFulfilledPromise<TMqttifyResult<void>>(TMqttifyResult<void>{false}).GetFuture();
	}

	TFuture<TMqttifyResult<void>> FMqttifyClient::PublishAsync(FMqttifyMessage&& InMessage)
	{
		if (IMqttifyPublishableAsync* Publishable = CurrentState->AsPublishable())
		{
			return Publishable->PublishAsync(MoveTemp(InMessage));
		}

		return MakeFulfilledPromise<TMqttifyResult<void>>(TMqttifyResult<void>{false}).GetFuture();
	}

	TFuture<TMqttifyResult<TArray<FMqttifySubscribeResult>>> FMqttifyClient::SubscribeAsync(
		const TArray<FMqttifyTopicFilter>& InTopicFilters)
	{
		if (IMqttifySubscribableAsync* Subscribable = CurrentState->AsSubscribable())
		{
			return Subscribable->SubscribeAsync(InTopicFilters);
		}

		return MakeFulfilledPromise<TMqttifyResult<TArray<FMqttifySubscribeResult>>>(
			TMqttifyResult<TArray<FMqttifySubscribeResult>>{false}).GetFuture();
	}


	TFuture<TMqttifyResult<FMqttifySubscribeResult>> FMqttifyClient::SubscribeAsync(FMqttifyTopicFilter& InTopicFilter)
	{
		if (IMqttifySubscribableAsync* Subscribable = CurrentState->AsSubscribable())
		{
			return Subscribable->SubscribeAsync(InTopicFilter);
		}
		return MakeFulfilledPromise<TMqttifyResult<FMqttifySubscribeResult>>(
			TMqttifyResult<FMqttifySubscribeResult>{false}).GetFuture();
	}

	TFuture<TMqttifyResult<FMqttifySubscribeResult>> FMqttifyClient::SubscribeAsync(FString& InTopicFilter)
	{
		if (IMqttifySubscribableAsync* Subscribable = CurrentState->AsSubscribable())
		{
			return Subscribable->SubscribeAsync(InTopicFilter);
		}

		return MakeFulfilledPromise<TMqttifyResult<FMqttifySubscribeResult>>(
			TMqttifyResult<FMqttifySubscribeResult>{false}).GetFuture();
	}

	TFuture<TMqttifyResult<TArray<FMqttifyUnsubscribeResult>>> FMqttifyClient::UnsubscribeAsync(
		const TSet<FString>& InTopicFilters)
	{
		if (IMqttifyUnsubscribableAsync* Unsubscribable = CurrentState->AsUnsubscribable())
		{
			return Unsubscribable->UnsubscribeAsync(InTopicFilters);
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
		return CurrentState->GetState() == EMqttifyState::Connected;
	}

	void FMqttifyClient::TransitionTo(TUniquePtr<FMqttifyClientState>&& InState)
	{
		LOG_MQTTIFY(Verbose,
					TEXT("Client transitioning from %s to %s"),
					EnumToTCharString(CurrentState->GetState()),
					EnumToTCharString(InState->GetState()));
		CurrentState = MoveTemp(InState);
	}
} // namespace Mqttify
