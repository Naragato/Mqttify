#pragma once
#include "Async/Future.h"
#include "Mqtt/MqttifyState.h"
#include "Mqtt/Interface/IMqttifyDisconnectableAsync.h"
#include "Mqtt/State/MqttifyClientState.h"
#include "Socket/Interface/MqttifySocketBase.h"

enum class EMqttifyProtocolVersion : uint8;

namespace Mqttify
{
	class FMqttifyClientDisconnectingState final : public FMqttifyClientState,
													public IMqttifyDisconnectableAsync
	{
	private:
		FMqttifySocketPtr Socket;

	public:
		explicit FMqttifyClientDisconnectingState(const FOnStateChangedDelegate& InOnStateChanged,
												const TSharedRef<FMqttifyClientContext>& InContext,
												const FMqttifySocketPtr& InSocket);

		EMqttifyState GetState() override { return EMqttifyState::Disconnecting; }
		TFuture<TMqttifyResult<void>> DisconnectAsync() override;
		IMqttifyDisconnectableAsync* AsDisconnectable() override { return this; }

	private:
		void OnSocketDisconnect();
	};
} // namespace Mqttify
