#pragma once
#include "Mqtt/MqttifyResult.h"

struct FMqttifyTopicFilter;
struct FMqttifySubscribeResult;

namespace Mqttify
{
	/**
	 * @brief Interface for a client that can subscribe to topics
	 */
	class MQTTIFY_API IMqttifySubscribableAsync
	{
	public:
		virtual ~IMqttifySubscribableAsync() = default;
		/**
		 * @brief Subscribe to a topic on the connected MQTT broker
		 * @param InTopicFilters The topics to subscribe to
		 * @return A future that contains the result of the connection, which can be checked for success.
		 * the result contains a result for each topic filter
		 */
		virtual TFuture<TMqttifyResult<TArray<FMqttifySubscribeResult>>> SubscribeAsync(
			const TArray<FMqttifyTopicFilter>& InTopicFilters) = 0;

		/**
		 * @param InTopicFilter The topic to subscribe to
		 * @return A future that contains the result of the connection, which can be checked for success.
		 * the result contains a result for this topic filter
		 */
		virtual TFuture<TMqttifyResult<FMqttifySubscribeResult>> SubscribeAsync(FMqttifyTopicFilter& InTopicFilter) = 0;


		/**
		 * @param InTopicFilter The topic to subscribe to
		 * @return A future that contains the result of the connection, which can be checked for success.
		 * the result contains a result for this topic filter
		 */
		 TFuture<TMqttifyResult<FMqttifySubscribeResult>> SubscribeAsync(FString&& InTopicFilter)
		 {
			 return SubscribeAsync(InTopicFilter);
		 }

		/**
		 * @param InTopicFilter The topic to subscribe to
		 * @return A future that contains the result of the connection, which can be checked for success.
		 * the result contains a result for this topic filter
		 */
		virtual TFuture<TMqttifyResult<FMqttifySubscribeResult>> SubscribeAsync(FString& InTopicFilter) = 0;
	};
} // namespace Mqttify
