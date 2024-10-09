#pragma once

#include "Async/Future.h"
#include "Mqtt/MqttifyResult.h"

namespace Mqttify
{
	using FConnectFuture = TFuture<TMqttifyResult<void>>;
	/**
	 * @brief Interface for a client that can handle when the socket is connected
	 */
	class MQTTIFY_API IMqttifySocketConnectedHandler
	{
	public:
		virtual ~IMqttifySocketConnectedHandler() = default;

		/**
		 * @brief Used to notify when the socket is connected
		 * @param bWasSuccessful true if the connection was successful
		 */
		virtual void OnSocketConnect(bool bWasSuccessful) = 0;
	};
} // namespace Mqttify
