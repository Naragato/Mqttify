#pragma once

#include "CoreMinimal.h"
#include "MqttifyEnumToString.h"

#include "MqttifyQualityOfService.generated.h"

/**
 * @brief Enum to represent MQTT Quality of Service levels.
 */
UENUM(BlueprintType)
enum class EMqttifyQualityOfService : uint8
{
	/** 
	 * @brief Quality Of Service 0: Once (not guaranteed)
	 * The message is delivered at most once, and delivery is not confirmed.
	 */
	AtMostOnce = 0 UMETA(DisplayName = "QoS 0", Description = "Once (not guaranteed)"),

	/**
	 * @brief Quality Of Service 1: At Least Once (guaranteed)
	 * The message is delivered at least once, and delivery is confirmed.
	 */
	AtLeastOnce = 1 UMETA(DisplayName = "QoS 1", Description = "At Least Once (guaranteed)"),

	/**
	 * @brief Quality Of Service 2: Exactly Once (guaranteed)
	 * The message is delivered exactly once by using a four-step handshake.
	 */
	ExactlyOnce = 2 UMETA(DisplayName = "QoS 2", Description = "Exactly Once (guaranteed)")

};

namespace MqttifyQualityOfService
{
	constexpr TCHAR InvalidQualityOfService[] = TEXT("Invalid Quality of Service");
} // namespace MqttifyQualityOfService

namespace Mqttify
{
	template <>
	FORCEINLINE const TCHAR* EnumToTCharString<EMqttifyQualityOfService>(const EMqttifyQualityOfService InValue)
	{
		switch (InValue)
		{
		case EMqttifyQualityOfService::AtMostOnce:
			return TEXT("QoS 0 - At most once");
		case EMqttifyQualityOfService::AtLeastOnce:
			return TEXT("QoS 1 - At least once");
		case EMqttifyQualityOfService::ExactlyOnce:
			return TEXT("QoS 2 - Exactly once");
		default:
			return MqttifyQualityOfService::InvalidQualityOfService;
		}
	}
} // namespace Mqttify
