#pragma once
#include "Mqtt/MqttifyResult.h"

namespace Mqttify
{
	/**
	 * @brief Interface for a client that can disconnect from the MQTT broker
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
		virtual TFuture<TMqttifyResult<void>> ConnectAsync(bool bCleanSession = false) = 0;
	};
} // namespace Mqttify
