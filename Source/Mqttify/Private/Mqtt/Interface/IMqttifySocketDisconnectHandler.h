#pragma once
#include "Mqtt/MqttifyResult.h"

namespace Mqttify
{
	using FDisconnectFuture = TFuture<TMqttifyResult<void>>;
	/**
	 * @brief Interface for a client that can handle when the socket is disconnected
	 */
	class MQTTIFY_API IMqttifySocketDisconnectHandler
	{
	public:
		virtual ~IMqttifySocketDisconnectHandler() = default;

		/**
		 * Used to notify when the socket is disconnected
		 */
		virtual void OnSocketDisconnect() = 0;
	};
} // namespace Mqttify
