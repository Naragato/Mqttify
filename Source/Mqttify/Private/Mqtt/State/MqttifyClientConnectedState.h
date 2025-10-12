#pragma once
#include "Mqtt/Commands/MqttifyPingReq.h"
#include "Mqtt/Interface/IMqttifyConnectableAsync.h"
#include "Mqtt/Interface/IMqttifyDisconnectableAsync.h"
#include "Mqtt/Interface/IMqttifyPacketReceiver.h"
#include "Mqtt/Interface/IMqttifySocketDisconnectHandler.h"
#include "Mqtt/State/MqttifyClientState.h"
#include "Socket/Interface/IMqttifySocketTickable.h"
#include "Socket/Interface/MqttifySocketBase.h"

enum class EMqttifyQualityOfService : uint8;

namespace Mqttify
{
	class IMqttifyControlPacket;

	class FMqttifyClientConnectedState final : public FMqttifyClientState,
	                                           public IMqttifyConnectableAsync,
	                                           public IMqttifyDisconnectableAsync,
	                                           public IMqttifySocketTickable,
	                                           public IMqttifyPacketReceiver,
	                                           public IMqttifySocketDisconnectHandler,
	                                           public TSharedFromThis<FMqttifyClientConnectedState>
	{
	private:
		FMqttifySocketRef Socket;
		bool bTransitioning = false;
		FDateTime LastPingTime;
		TSharedPtr<FMqttifyPingReq> PingReqCommand;

	public:
		explicit FMqttifyClientConnectedState(
			const FOnStateChangedDelegate& InOnStateChanged,
			const TSharedRef<FMqttifyClientContext>& InContext,
			const FMqttifySocketRef& InSocket
			)
			: FMqttifyClientState{InOnStateChanged, InContext}
			, Socket{InSocket} {}

		// FMqttifyClientState
		virtual EMqttifyState GetState() override { return EMqttifyState::Connected; }
		virtual IMqttifyConnectableAsync* AsConnectable() override { return this; }
		virtual IMqttifyDisconnectableAsync* AsDisconnectable() override { return this; }
		virtual IMqttifySocketDisconnectHandler* AsSocketDisconnectHandler() override { return this; }
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
	}
} // namespace Mqttify
