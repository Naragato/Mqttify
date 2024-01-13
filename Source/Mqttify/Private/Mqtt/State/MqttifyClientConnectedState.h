#pragma once
#include "Mqtt/Commands/MqttifyPingReq.h"
#include "Mqtt/Interface/IMqttifyConnectableAsync.h"
#include "Mqtt/Interface/IMqttifyDisconnectableAsync.h"
#include "Mqtt/Interface/IMqttifyPublishableAsync.h"
#include "Mqtt/Interface/IMqttifySubscribableAsync.h"
#include "Mqtt/Interface/IMqttifyUnsubscribableAsync.h"
#include "Mqtt/State/MqttifyClientState.h"
#include "Socket/Interface/MqttifySocketBase.h"
#include "Socket/SocketState/IMqttifySocketTickable.h"

enum class EMqttifyQualityOfService : uint8;

namespace Mqttify
{
	class FMqttifyAcknowledgeable;
	class IMqttifyClient;
	class IMqttifyControlPacket;

	class FMqttifyClientConnectedState final : public FMqttifyClientState,
												public IMqttifyConnectableAsync,
												public IMqttifyDisconnectableAsync,
												public IMqttifyPublishableAsync,
												public IMqttifySubscribableAsync,
												public IMqttifyUnsubscribableAsync,
												public IMqttifySocketTickable
	{
	private:
		FMqttifySocketPtr Socket;

		FDateTime LastPingTime;
		TSharedPtr<FMqttifyPingReq> PingReqCommand;

		void OnSocketDisconnect();
		void OnSocketDataReceive(TSharedPtr<FArrayReader> InData);

	public:
		explicit FMqttifyClientConnectedState(const FOnStateChangedDelegate& InOnStateChanged,
											const TSharedRef<FMqttifyClientContext>& InContext,
											const FMqttifySocketPtr& InSocket)
			: FMqttifyClientState{ InOnStateChanged, InContext }
			, Socket{ InSocket }
		{
			if (Socket == nullptr)
			{
				LOG_MQTTIFY(Error, TEXT("Failed to create Socket."));
				return;
			}

			Socket->GetOnDisconnectDelegate().AddRaw(this, &FMqttifyClientConnectedState::OnSocketDisconnect);
			Socket->GetOnDataReceivedDelegate().AddRaw(this, &FMqttifyClientConnectedState::OnSocketDataReceive);
		}

		~FMqttifyClientConnectedState() override
		{
			if (Socket == nullptr)
			{
				return;
			}
			Socket->GetOnDisconnectDelegate().RemoveAll(this);
			Socket->GetOnDataReceivedDelegate().RemoveAll(this);
		}

		EMqttifyState GetState() override { return EMqttifyState::Connected; }

		TFuture<TMqttifyResult<void>> ConnectAsync(bool bCleanSession = false) override;
		IMqttifyConnectableAsync* AsConnectable() override { return this; }

		TFuture<TMqttifyResult<void>> DisconnectAsync() override;
		IMqttifyDisconnectableAsync* AsDisconnectable() override { return this; }

		IMqttifyPublishableAsync* AsPublishable() override { return this; }
		TFuture<TMqttifyResult<void>> PublishAsync(FMqttifyMessage&& InMessage) override;

		IMqttifySubscribableAsync* AsSubscribable() override { return this; }
		TFuture<TMqttifyResult<TArray<FMqttifySubscribeResult>>> SubscribeAsync(
			const TArray<FMqttifyTopicFilter>& InTopicFilters) override;
		TFuture<TMqttifyResult<FMqttifySubscribeResult>> SubscribeAsync(FMqttifyTopicFilter& InTopicFilter) override;
		TFuture<TMqttifyResult<FMqttifySubscribeResult>> SubscribeAsync(FString& InTopicFilter) override;

		IMqttifyUnsubscribableAsync* AsUnsubscribable() override { return this; }
		TFuture<TMqttifyResult<TArray<FMqttifyUnsubscribeResult>>>
		UnsubscribeAsync(const TSet<FString>& InTopicFilters) override;

		IMqttifySocketTickable* AsSocketTickable() override { return this; }
		void Tick() override;
	};
} // namespace Mqttify
