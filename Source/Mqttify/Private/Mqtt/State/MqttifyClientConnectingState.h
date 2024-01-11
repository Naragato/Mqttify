#pragma once
#include "LogMqttify.h"
#include "Mqtt/Interface/IMqttifyConnectableAsync.h"
#include "Mqtt/State/MqttifyClientState.h"
#include "Socket/Interface/MqttifySocketBase.h"
#include "Socket/SocketState/IMqttifySocketTickable.h"

namespace Mqttify
{
	class IMqttifyClient;
	class IMqttifyControlPacket;

	class FMqttifyClientConnectingState final : public FMqttifyClientState,
												public IMqttifyConnectableAsync,
												IMqttifySocketTickable
	{
	private:
		bool bCleanSession;
		FMqttifySocketPtr Socket;

		FDateTime LastConnectAttempt;
		uint8 AttemptCount;
		std::atomic<bool> bIsConnecting;

		void OnSocketConnect(const bool bInSuccess);
		void OnSocketDisconnect();
		void OnSocketDataReceive(TSharedPtr<FArrayReader> InData);
		void TryConnect();

	public:
		explicit FMqttifyClientConnectingState(
			const FOnStateChangedDelegate& InOnStateChanged,
			const TSharedRef<FMqttifyClientContext>& InContext,
			const bool bInCleanSession)
			: FMqttifyClientState{ InOnStateChanged, InContext }
			, bCleanSession{ bInCleanSession }
			, Socket{ FMqttifySocketBase::Create(InContext->GetConnectionSettings()) }
			, LastConnectAttempt{ FDateTime::MinValue() }
			, AttemptCount{ 0 }
		{
			if (Socket == nullptr)
			{
				LOG_MQTTIFY(Error, TEXT("Failed to create Socket."));
				return;
			}

			Socket->GetOnConnectDelegate().AddRaw(this, &FMqttifyClientConnectingState::OnSocketConnect);
			Socket->GetOnDisconnectDelegate().AddRaw(this, &FMqttifyClientConnectingState::OnSocketDisconnect);
			Socket->GetOnDataReceivedDelegate().AddRaw(this, &FMqttifyClientConnectingState::OnSocketDataReceive);
			Socket->Connect();
		};

		explicit FMqttifyClientConnectingState(
			const FOnStateChangedDelegate& InOnStateChanged,
			const TSharedRef<FMqttifyClientContext>& InContext,
			const FMqttifySocketPtr& InSocket)
			: FMqttifyClientState{ InOnStateChanged, InContext }
			, bCleanSession{ false }
			, Socket{ InSocket }
			, LastConnectAttempt{ FDateTime::MinValue() }
			, AttemptCount{ 0 }
		{
			if (Socket == nullptr)
			{
				LOG_MQTTIFY(Error, TEXT("Failed to create Socket."));
				return;
			}

			Socket->GetOnConnectDelegate().AddRaw(this, &FMqttifyClientConnectingState::OnSocketConnect);
			Socket->GetOnDisconnectDelegate().AddRaw(this, &FMqttifyClientConnectingState::OnSocketDisconnect);
			Socket->GetOnDataReceivedDelegate().AddRaw(this, &FMqttifyClientConnectingState::OnSocketDataReceive);
			Socket->Connect();
		}

		~FMqttifyClientConnectingState() override
		{
			if (Socket == nullptr)
			{
				return;
			}
			Socket->GetOnConnectDelegate().RemoveAll(this);
			Socket->GetOnDisconnectDelegate().RemoveAll(this);
			Socket->GetOnDataReceivedDelegate().RemoveAll(this);
		}

		IMqttifyConnectableAsync* AsConnectable() override { return this; }
		IMqttifySocketTickable* AsSocketTickable() override { return this; }
		EMqttifyState GetState() override { return EMqttifyState::Connecting; }
		TFuture<TMqttifyResult<void>> ConnectAsync(bool bInCleanSession) override;
		void Tick() override;
	};
} // namespace Mqttify
