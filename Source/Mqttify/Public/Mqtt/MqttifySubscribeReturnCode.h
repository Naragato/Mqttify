#pragma once

#include "CoreMinimal.h"
#include "MqttifySubscribeReturnCode.generated.h"

/** Mqtt v3.1.1 */
/**
 * @enum EMqttifySubscribeReturnCode
 * @brief Enum class for MQTT subscription return codes
 */
UENUM(BlueprintType)
enum class EMqttifySubscribeReturnCode : uint8
{
	/** 
	 * @brief Success with Quality of Service level 0
	 * 
	 * Indicates a successful subscription acknowledgment with QoS level 0.
	 */
	SuccessQualityOfService0 = 0,

	/** 
	 * @brief Success with Quality of Service level 1
	 * 
	 * Indicates a successful subscription acknowledgment with QoS level 1.
	 */
	SuccessQualityOfService1 = 1,

	/** 
	 * @brief Success with Quality of Service level 2
	 * 
	 * Indicates a successful subscription acknowledgment with QoS level 2.
	 */
	SuccessQualityOfService2 = 2,

	/** 
	 * @brief Failure
	 * 
	 * Indicates a failure in subscription acknowledgment.
	 */
	Failure = 128
};

namespace MqttifySubscribeReturnCode
{
	constexpr TCHAR InvalidSubscribeReturnCode[] = TEXT("Invalid Subscribe return code.");
} // namespace MqttifySubscribeReturnCode
