#include "Mqtt/State/MqttifyClientDisconnectedState.h"

#include "LogMqttify.h"
#include "Mqtt/State/MqttifyClientConnectingState.h"

namespace Mqttify
{

	FMqttifyDisconnectedState::FMqttifyDisconnectedState(const FOnStateChangedDelegate& InOnStateChanged,
														const TSharedRef<FMqttifyClientContext>& InContext)
		: FMqttifyClientState{ InOnStateChanged, InContext } {}


	TFuture<TMqttifyResult<void>> FMqttifyDisconnectedState::ConnectAsync(bool bCleanSession)
	{
		const TSharedPtr<TPromise<TMqttifyResult<void>>> Promise = Context->GetConnectPromise();
		TransitionTo(MakeUnique<FMqttifyClientConnectingState>(
			OnStateChanged,
			Context,
			bCleanSession));
		return Promise->GetFuture();
	}
} // namespace Mqttify
