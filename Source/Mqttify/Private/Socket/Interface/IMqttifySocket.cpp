#include "Socket/Interface/IMqttifySocket.h"

#include "Socket/MqttifySocket.h"
#include "Socket/MqttifyWebSocket.h"

namespace Mqttify
{
	IMqttifySocket::~IMqttifySocket()
	{
		FScopeLock Lock(&SocketAccessLock);
		OnConnectDelegate.Clear();
		OnDisconnectDelegate.Clear();
		OnDataReceiveDelegate.Clear();
	}

	FMqttifySocketPtr IMqttifySocket::Create(const FMqttifyConnectionSettingsRef& InConnectionSettings)
	{

		switch (InConnectionSettings->GetTransportProtocol())
		{

			case EMqttifyConnectionProtocol::Mqtt:
			case EMqttifyConnectionProtocol::Mqtts:
				return MakeShared<FMqttifySocket>(InConnectionSettings);
			case EMqttifyConnectionProtocol::Ws:
			case EMqttifyConnectionProtocol::Wss:
			default:
				return MakeShared<FMqttifyWebSocket>(InConnectionSettings);
		}
	}
} // namespace Mqttify
