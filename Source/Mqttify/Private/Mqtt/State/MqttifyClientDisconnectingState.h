#pragma once
#include "Async/Future.h"
#include "Mqtt/MqttifyState.h"
#include "Mqtt/Interface/IMqttifyDisconnectableAsync.h"
#include "Mqtt/Interface/IMqttifySocketDisconnectHandler.h"
#include "Mqtt/State/MqttifyClientState.h"
#include "Socket/Interface/MqttifySocketBase.h"

enum class EMqttifyProtocolVersion : uint8;

namespace Mqttify
{
	class FMqttifyClientDisconnectingState final : public FMqttifyClientState,
	                                               public IMqttifyDisconnectableAsync,
	                                               public IMqttifySocketDisconnectHandler
	{
	private:
		FMqttifySocketPtr Socket;

	public:
		explicit FMqttifyClientDisconnectingState(
			const FOnStateChangedDelegate& InOnStateChanged,
			const TSharedRef<FMqttifyClientContext>& InContext,
			const FMqttifySocketPtr& InSocket
			);

		// FMqttifyClientState
		virtual EMqttifyState GetState() override { return EMqttifyState::Disconnecting; }
		virtual IMqttifyDisconnectableAsync* AsDisconnectable() override { return this; }
		virtual IMqttifySocketDisconnectHandler* AsSocketDisconnectHandler() override { return this; }
		// ~ FMqttifyClientState

		// IMqttifyDisconnectableAsync
		virtual TFuture<TMqttifyResult<void>> DisconnectAsync() override;
		// ~ IMqttifyDisconnectableAsync

		// IMqttifySocketDisconnectHandler
		virtual void OnSocketDisconnect() override;
		// ~ IMqttifySocketDisconnectHandler
	};
} // namespace Mqttify
