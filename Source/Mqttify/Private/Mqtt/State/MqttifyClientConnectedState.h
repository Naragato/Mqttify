#pragma once
#include "Mqtt/Commands/MqttifyPingReq.h"
#include "Mqtt/Interface/IMqttifyConnectableAsync.h"
#include "Mqtt/Interface/IMqttifyDisconnectableAsync.h"
#include "Mqtt/Interface/IMqttifyPacketReceiver.h"
#include "Mqtt/Interface/IMqttifyPublishableAsync.h"
#include "Mqtt/Interface/IMqttifySocketDisconnectHandler.h"
#include "Mqtt/Interface/IMqttifySubscribableAsync.h"
#include "Mqtt/Interface/IMqttifyUnsubscribableAsync.h"
#include "Mqtt/State/MqttifyClientState.h"
#include "Socket/Interface/IMqttifySocketTickable.h"
#include "Socket/Interface/MqttifySocketBase.h"

enum class EMqttifyQualityOfService : uint8;

namespace Mqttify
{
	class FMqttifyAcknowledgeable;
	class IMqttifyControlPacket;

	class FMqttifyClientConnectedState final : public FMqttifyClientState,
	                                           public IMqttifyConnectableAsync,
	                                           public IMqttifyDisconnectableAsync,
	                                           public IMqttifyPublishableAsync,
	                                           public IMqttifySubscribableAsync,
	                                           public IMqttifyUnsubscribableAsync,
	                                           public IMqttifySocketTickable,
	                                           public IMqttifyPacketReceiver,
	                                           public IMqttifySocketDisconnectHandler,
	                                           public TSharedFromThis<FMqttifyClientConnectedState>
	{
	private:
		FMqttifySocketPtr Socket;
		bool bTransitioning = false;
		FDateTime LastPingTime;
		TSharedPtr<FMqttifyPingReq> PingReqCommand;

	public:
		explicit FMqttifyClientConnectedState(
			const FOnStateChangedDelegate& InOnStateChanged,
			const TSharedRef<FMqttifyClientContext>& InContext,
			const FMqttifySocketPtr& InSocket
			)
			: FMqttifyClientState{InOnStateChanged, InContext}
			, Socket{InSocket}
		{
		}

		// FMqttifyClientState
		virtual EMqttifyState GetState() override { return EMqttifyState::Connected; }
		virtual IMqttifyConnectableAsync* AsConnectable() override { return this; }
		virtual IMqttifyDisconnectableAsync* AsDisconnectable() override { return this; }
		virtual IMqttifySocketDisconnectHandler* AsSocketDisconnectHandler() override { return this; }
		virtual IMqttifyPublishableAsync* AsPublishable() override { return this; }
		virtual IMqttifySubscribableAsync* AsSubscribable() override { return this; }
		virtual IMqttifyUnsubscribableAsync* AsUnsubscribable() override { return this; }
		virtual IMqttifySocketTickable* AsSocketTickable() override { return this; }
		virtual IMqttifyPacketReceiver* AsPacketReceiver() override { return this; }
		// ~ FMqttifyClientState

		// IMqttifyConnectableAsync
		virtual FConnectFuture ConnectAsync(bool bCleanSession = false) override;
		// ~IMqttifySocketTickable

		// IMqttifyDisconnectableAsync
		virtual FDisconnectFuture DisconnectAsync() override;
		// ~IMqttifySocketTickable

		// IMqttifySocketDisconnectHandler
		virtual void OnSocketDisconnect() override;
		// ~IMqttifySocketDisconnectHandler

		// IMqttifyPublishableAsync
		virtual FPublishFuture PublishAsync(FMqttifyMessage&& InMessage) override;
		// ~IMqttifyPublishableAsync

		// IMqttifySubscribableAsync
		virtual FSubscribesFuture SubscribeAsync(const TArray<FMqttifyTopicFilter>& InTopicFilters) override;
		virtual FSubscribeFuture SubscribeAsync(FMqttifyTopicFilter&& InTopicFilter) override;
		virtual FSubscribeFuture SubscribeAsync(FString& InTopicFilter) override;
		// ~IMqttifySubscribableAsync

		// IMqttifyUnsubscribableAsync
		virtual FUnsubscribesFuture UnsubscribeAsync(const TSet<FString>& InTopicFilters) override;
		// ~IMqttifyUnsubscribableAsync

		// IMqttifySocketTickable
		virtual void Tick() override;
		// IMqttifySocketTickable

		// IMqttifyPacketReceiver
		virtual void OnReceivePacket(const TSharedPtr<FArrayReader>& InPacket) override;
		// ~IMqttifyPacketReceiver
	private:
		template <typename TClientState>
		void SocketTransitionToState();
	};

	template <typename TClientState>
	void FMqttifyClientConnectedState::SocketTransitionToState()
	{
		static_assert(
			std::is_base_of_v<FMqttifyClientState, TClientState>,
			"FMqttifyClientConnectingState::TransitionToState: TClientState must inherit from FMqttifyClientState.");
		bTransitioning = true;
		TransitionTo(MakeShared<TClientState>(OnStateChanged, Context, Socket));
		Socket->GetOnConnectDelegate().RemoveAll(this);
		Socket->GetOnDisconnectDelegate().RemoveAll(this);
		Socket->GetOnDataReceivedDelegate().RemoveAll(this);
		Socket.Reset();
	}
} // namespace Mqttify
