#pragma once
#include "Mqtt/MqttifyResult.h"
#include "Mqtt/Interface/IMqttifyClient.h"
#include "Socket/SocketState/IMqttifySocketTickable.h"

struct FMqttifyUnsubscribeResult;

namespace Mqttify
{
	/**
	 * @brief Interface for a MQTT client that can be ticked.
	 */
	class ITickableMqttifyClient : public IMqttifyClient
	{
	public:
		virtual void Tick() = 0;
	}; // namespace Mqttify
}
