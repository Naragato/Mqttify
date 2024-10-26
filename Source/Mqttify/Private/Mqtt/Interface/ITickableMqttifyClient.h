#pragma once
#include "Mqtt/Interface/IMqttifyClient.h"

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
