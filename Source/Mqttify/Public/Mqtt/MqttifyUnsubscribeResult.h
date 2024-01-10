#pragma once

#include "MqttifyTopicFilter.h"

/**
 * @brief Result of a unsubscribe attempt.
 */
struct MQTTIFY_API FMqttifyUnsubscribeResult final
{
	explicit FMqttifyUnsubscribeResult(const FMqttifyTopicFilter& InFilter, const bool bInWasSuccessful)
		: Filter{ InFilter }
		, bWasSuccessful(bInWasSuccessful) {}

	~FMqttifyUnsubscribeResult() = default;

	/**
	 * @brief Get the topic filter.
	 * @return The topic filter.
	 */
	FMqttifyTopicFilter GetFilter() const
	{
		return Filter;
	}

	/**
	 * @brief Whether the operation was successful.
	 * @return Whether the operation was successful.
	 */
	bool WasSuccessful() const
	{
		return bWasSuccessful;
	}

private:
	/**
	 * @brief Topic filter string, e.g., "home/livingroom/temperature".
	 * with the QoS etc ...
	 */
	FMqttifyTopicFilter Filter;

	/**
	 * @brief Whether the operation was successful.
	 */
	bool bWasSuccessful;
};
