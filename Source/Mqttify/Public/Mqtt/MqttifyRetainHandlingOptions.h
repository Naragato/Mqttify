#pragma once

#include "CoreMinimal.h"
#include "MqttifyRetainHandlingOptions.generated.h"

/**
 * @enum EMqttifyRetainHandlingOptions
 * @brief Enum class for MQTT retain handling options. For Mqtt v5.
 */
UENUM(BlueprintType)
enum class EMqttifyRetainHandlingOptions : uint8
{

	SendRetainedMessagesAtSubscribeTime = 0 UMETA(DisplayName = "Send retained messages",
												Description = "Send retained messages at the time of the subscribe"),
	SendRetainedMessagesAtSubscribeTimeIfSubscriptionDoesNotExist = 1 UMETA(
		DisplayName = "Send retained messages if subscription does not exist",
		Description = "Send retained messages at the time of the subscribe if the subscription does not exist"),
	DontSendRetainedMessagesAtSubscribeTime = 2 UMETA(DisplayName = "Do not send retained messages",
													Description =
													"Do not send retained messages at the time of the subscribe"),
	Max = 3 UMETA(Hidden)
};

namespace MqttifyRetainHandlingOptions
{
	constexpr TCHAR InvalidRetainHandlingOption[] = TEXT("Invalid Retain handling option");
} // namespace MqttifyRetainHandlingOptions
