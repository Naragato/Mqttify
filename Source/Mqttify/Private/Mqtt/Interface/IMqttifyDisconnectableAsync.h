#pragma once
#include "Mqtt/MqttifyResult.h"

namespace Mqttify
{
	/**
	 * @brief Interface for a client that can disconnect from the MQTT broker
	 */
	class MQTTIFY_API IMqttifyDisconnectableAsync
	{
	public:
		virtual ~IMqttifyDisconnectableAsync() = default;
		/**
		 * @brief Disconnect from to the MQTT broker
		 * @return A future that contains the result of the disconnection, which can be checked for success.
		 */
		virtual TFuture<TMqttifyResult<void>> DisconnectAsync() = 0;
	};
} // namespace Mqttify
