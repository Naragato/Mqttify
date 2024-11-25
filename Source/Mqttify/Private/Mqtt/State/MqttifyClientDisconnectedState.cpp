#include "Mqtt/State/MqttifyClientDisconnectedState.h"

#include "Mqtt/State/MqttifyClientConnectingState.h"

namespace Mqttify
{
	FMqttifyClientDisconnectedState::FMqttifyClientDisconnectedState(
		const FOnStateChangedDelegate& InOnStateChanged,
		const TSharedRef<FMqttifyClientContext>& InContext,
		const FMqttifySocketRef& InSocket
		)
		: FMqttifyClientState{InOnStateChanged, InContext}
		, Socket{InSocket}
	{
	}

	TFuture<TMqttifyResult<void>> FMqttifyClientDisconnectedState::ConnectAsync(bool bCleanSession)
	{
		const TSharedPtr<TPromise<TMqttifyResult<void>>> Promise = Context->GetConnectPromise();
		TransitionTo(MakeShared<FMqttifyClientConnectingState>(OnStateChanged, Context, bCleanSession, Socket));
		return Promise->GetFuture();
	}
} // namespace Mqttify
