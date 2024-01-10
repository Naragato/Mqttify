#include "Socket/SocketState/MqttifySocketDisconnectedState.h"

#include "MqttifySocketConnectingState.h"

namespace Mqttify
{
	void FMqttifySocketDisconnectedState::Connect()
	{
		TransitionTo(MakeUnique<FMqttifySocketConnectingState>(MqttifySocket));
	}
} // namespace Mqttify
