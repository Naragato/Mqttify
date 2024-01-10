#pragma once
#include "Mqtt/MqttifyResult.h"

struct FMqttifyUnsubscribeResult;

namespace Mqttify
{
	/**
	 * @brief Interface for a client that can subscribe to topics
	 */
	class MQTTIFY_API IMqttifyUnsubscribableAsync
	{
	public:
		virtual ~IMqttifyUnsubscribableAsync() = default;
		/**
		 * @return A future that contains the result of the connection, which can be checked for success.
		 * the result contains a true or false for each topic filter
		 */
		virtual TFuture<TMqttifyResult<TArray<FMqttifyUnsubscribeResult>>> UnsubscribeAsync(
			const TSet<FString>& InTopicFilters) = 0;
	}; // namespace Mqttify
}
