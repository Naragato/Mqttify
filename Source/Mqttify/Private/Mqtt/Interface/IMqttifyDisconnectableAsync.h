#pragma once
#include "Mqtt/MqttifyResult.h"

namespace Mqttify
{
	using FDisconnectFuture = TFuture<TMqttifyResult<void>>;
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
		virtual FDisconnectFuture DisconnectAsync() = 0;
	};
} // namespace Mqttify
