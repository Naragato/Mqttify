#pragma once
#include "MqttifyAsync.h"
#include "Async/Future.h"
#include "Mqtt/MqttifyState.h"
#include "Mqtt/Interface/IMqttifyConnectableAsync.h"
#include "Mqtt/Interface/IMqttifyDisconnectableAsync.h"
#include "Mqtt/State/MqttifyClientState.h"
#include "Socket/Interface/MqttifySocketBase.h"

enum class EMqttifyProtocolVersion : uint8;

namespace Mqttify
{
	class IMqttifyControlPacket;

	class FMqttifyClientDisconnectedState final : public FMqttifyClientState,
	                                              public IMqttifyConnectableAsync,
	                                              public IMqttifyDisconnectableAsync
	{
	private:
		FMqttifySocketRef Socket;

	public:
		explicit FMqttifyClientDisconnectedState(
			const FOnStateChangedDelegate& InOnStateChanged,
			const TSharedRef<FMqttifyClientContext>& InContext,
			const FMqttifySocketRef& InSocket
			);

		// FMqttifyClientState
		virtual EMqttifyState GetState() override { return EMqttifyState::Disconnected; }
		virtual IMqttifyConnectableAsync* AsConnectable() override { return this; }
		virtual IMqttifyDisconnectableAsync* AsDisconnectable() override { return this; }
		// ~ FMqttifyClientState

		// IMqttifyConnectableAsync
		virtual TFuture<TMqttifyResult<void>> ConnectAsync(bool bCleanSession = false) override;
		// ~ IMqttifyConnectableAsync

		// IMqttifyDisconnectableAsync
		virtual TFuture<TMqttifyResult<void>> DisconnectAsync() override
		{
			return MakeThreadAwareFulfilledPromise<TMqttifyResult<void>>(TMqttifyResult<void>{true});
		}

		// ~ IMqttifyDisconnectableAsync
	};
} // namespace Mqttify