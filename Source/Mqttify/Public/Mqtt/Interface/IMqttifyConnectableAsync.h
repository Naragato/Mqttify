#pragma once

#include "Async/Future.h"
#include "Mqtt/MqttifyResult.h"

namespace Mqttify
{
	using FConnectFuture = TFuture<TMqttifyResult<void>>;
	/**
	 * @brief Interface for a client that can connect to the MQTT broker
	 */
	class MQTTIFY_API IMqttifyConnectableAsync
	{
	public:
		virtual ~IMqttifyConnectableAsync() = default;
		/**
		 * @brief Connect to the MQTT broker
		 * @param bCleanSession Whether to start a clean session, or resume an existing one true starts a clean session
		 * @return A future that contains the result of the connection, which can be checked for success.
		 */
		virtual FConnectFuture ConnectAsync(bool bCleanSession) = 0;
	};
} // namespace Mqttify
