#pragma once
#include "Mqtt/MqttifyResult.h"

struct FMqttifyMessage;

namespace Mqttify
{
	using FPublishFuture = TFuture<TMqttifyResult<void>>;
	/**
	 * @brief Interface for a client that can subscribe to topics
	 */
	class MQTTIFY_API IMqttifyPublishableAsync
	{
	public:
		virtual ~IMqttifyPublishableAsync() = default;

		/**
		 * @brief Publish to a topic on the connected MQTT broker
		 * @param InMessage The message to publish
		 * @return A future that contains the result of the publish, which can be checked for success.
		 */
		virtual FPublishFuture PublishAsync(FMqttifyMessage&& InMessage) = 0;

	};
} // namespace Mqttify
