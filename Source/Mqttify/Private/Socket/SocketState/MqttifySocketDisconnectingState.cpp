#include "Socket/SocketState/MqttifySocketDisconnectingState.h"

#include "MqttifySocketConnectingState.h"
#include "Sockets.h"
#include "Socket/MqttifySocket.h"
#include "Socket/SocketState/MqttifySocketDisconnectedState.h"

namespace Mqttify
{
	void FMqttifySocketDisconnectingState::Tick()
	{
		if (Socket.IsValid())
		{
			Socket->Close();
		}

		MqttifySocket->GetOnDisconnectDelegate().Broadcast();
		TransitionTo(MakeUnique<FMqttifySocketDisconnectedState>(MqttifySocket));
	}

	void FMqttifySocketDisconnectingState::Connect()
	{
		TransitionTo(MakeUnique<FMqttifySocketConnectingState>(MqttifySocket));
	}
} // namespace Mqttify
