#pragma once

#include "Delegates/OnMessage.h"
#include "Mqtt/MqttifyTopicFilter.h"

/**
 * @brief Result of a subscription attempt.
 */
struct MQTTIFY_API FMqttifySubscribeResult final
{
	explicit FMqttifySubscribeResult(const FMqttifyTopicFilter& InFilter,
									const bool bInWasSuccessful,
									const TSharedPtr<Mqttify::FOnMessage>& InOnMessage = nullptr)
		: Filter{ InFilter }
		, bWasSuccessful(bInWasSuccessful)
		, OnMessage{ InOnMessage } {}

	~FMqttifySubscribeResult() = default;

	/**
	 * @brief Get the topic filter.
	 * @return The topic filter.
	 */
	FMqttifyTopicFilter GetFilter() const
	{
		return Filter;
	}

	/**
	 * @brief Whether the subscription was successful.
	 * @return Whether the subscription was successful.
	 */
	bool WasSuccessful() const
	{
		return bWasSuccessful;
	}

	/**
	 * @brief Get the delegate which is called when a message is received matching the subscription.
	 * @return The delegate which is called when a message is received matching the subscription.
	 */
	TSharedPtr<Mqttify::FOnMessage> GetOnMessage() const
	{
		return OnMessage;
	}

private:
	/**
	 * @brief Topic filter string, e.g., "home/livingroom/temperature".
	 * with the QoS etc ...
	 */
	FMqttifyTopicFilter Filter;

	/**
	 * @brief Whether the subscription was successful.
	 */
	bool bWasSuccessful;

	/**
	 * @brief Delegate which is called when a message is received matching the subscription.
	 */
	TSharedPtr<Mqttify::FOnMessage> OnMessage;
};
