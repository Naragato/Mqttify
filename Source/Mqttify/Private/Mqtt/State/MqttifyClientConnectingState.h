#pragma once
#include "Mqtt/Interface/IMqttifyConnectableAsync.h"
#include "Mqtt/Interface/IMqttifyDisconnectableAsync.h"
#include "Mqtt/Interface/IMqttifyPacketReceiver.h"
#include "Mqtt/Interface/IMqttifySocketConnectedHandler.h"
#include "Mqtt/Interface/IMqttifySocketDisconnectHandler.h"
#include "Mqtt/State/MqttifyClientState.h"
#include "Socket/Interface/IMqttifySocketTickable.h"
#include "Socket/Interface/MqttifySocketBase.h"

namespace Mqttify
{
	class IMqttifyControlPacket;

	class FMqttifyClientConnectingState final : public FMqttifyClientState,
	                                            public IMqttifyConnectableAsync,
	                                            public IMqttifyDisconnectableAsync,
	                                            public IMqttifySocketTickable,
	                                            public IMqttifyPacketReceiver,
	                                            public IMqttifySocketDisconnectHandler,
	                                            public IMqttifySocketConnectedHandler,
	                                            public TSharedFromThis<FMqttifyClientConnectingState>
	{
	private:
		bool bCleanSession;
		FMqttifySocketPtr Socket;

		FDateTime LastConnectAttempt;
		uint8 AttemptCount;
		std::atomic<bool> bIsConnecting;
		template <typename TClientState>
		void SocketTransitionToState();
		void TryConnect();

	public:
		explicit FMqttifyClientConnectingState(
			const FOnStateChangedDelegate& InOnStateChanged,
			const TSharedRef<FMqttifyClientContext>& InContext,
			const bool bInCleanSession,
			const FMqttifySocketPtr& InSocket
			)
			: FMqttifyClientState{InOnStateChanged, InContext}
			, bCleanSession{bInCleanSession}
			, Socket{InSocket}
			, LastConnectAttempt{FDateTime::MinValue()}
			, AttemptCount{0}
		{
			checkf(Socket.IsValid(), TEXT("Socket must be valid."));
			if (Socket.IsValid())
			{
				Socket->Connect();
			}
		};

		explicit FMqttifyClientConnectingState(
			const FOnStateChangedDelegate& InOnStateChanged,
			const TSharedRef<FMqttifyClientContext>& InContext,
			const FMqttifySocketPtr& InSocket
			)
			: FMqttifyClientState{InOnStateChanged, InContext}
			, bCleanSession{false}
			, Socket{InSocket}
			, LastConnectAttempt{FDateTime::MinValue()}
			, AttemptCount{0}
		{
			checkf(Socket.IsValid(), TEXT("Socket must be valid."));
			if (Socket.IsValid())
			{
				Socket->Connect();
			}
		}

		// IFMqttifyClientState
		virtual EMqttifyState GetState() override { return EMqttifyState::Connecting; }
		virtual IMqttifyConnectableAsync* AsConnectable() override { return this; }
		virtual IMqttifySocketConnectedHandler* AsSocketConnectedHandler() override { return this; }
		virtual IMqttifyDisconnectableAsync* AsDisconnectable() override { return this; }
		virtual IMqttifySocketDisconnectHandler* AsSocketDisconnectHandler() override { return this; }
		virtual IMqttifySocketTickable* AsSocketTickable() override { return this; }
		virtual IMqttifyPacketReceiver* AsPacketReceiver() override { return this; }
		// ~ IFMqttifyClientState

		// IMqttifyConnectableAsync
		virtual FConnectFuture ConnectAsync(bool bInCleanSession) override;
		// ~ IMqttifyConnectableAsync

		// IMqttifySocketConnectedHandler
		virtual void OnSocketConnect(bool bWasSuccessful) override;
		// ~ IMqttifySocketConnectedHandler

		// IMqttifyDisconnectableAsync
		virtual FDisconnectFuture DisconnectAsync() override;
		// ~ IMqttifyDisconnectableAsync

		// IMqttifySocketDisconnectHandler
		virtual void OnSocketDisconnect() override;
		// ~ IMqttifySocketDisconnectHandler

		// IMqttifySocketTickable
		virtual void Tick() override;
		// ~ IMqttifySocketTickable

		// IMqttifyPacketReceiver
		virtual void OnReceivePacket(const TSharedPtr<FArrayReader>& InPacket) override;
		// ~ IMqttifyPacketReceiver
	};

	template <typename TClientState>
	void FMqttifyClientConnectingState::SocketTransitionToState()
	{
		static_assert(
			std::is_base_of_v<FMqttifyClientState, TClientState>,
			"FMqttifyClientConnectingState::TransitionToState: TClientState must inherit from FMqttifyClientState.");

		TransitionTo(MakeShared<TClientState>(OnStateChanged, Context, Socket));
	}
} // namespace Mqttify
