#pragma once

#include "CoreMinimal.h"

#include "MqttifyProtocolVersion.generated.h"

/**
 * @enum EMqttifyProtocolVersion
 * @brief Enum representing MQTT protocol versions.
 */
UENUM(BlueprintType)
enum class EMqttifyProtocolVersion : uint8
{
	None = 0,
	/**
	 * @brief MQTT version 3.1.1
	 * Represented by the integer value 4.
	 */
	Mqtt_3_1_1 = 4,

	/**
	 * @brief MQTT version 5
	 * Represented by the integer value 5.
	 */
	Mqtt_5 = 5
};

namespace MqttifyProtocolVersion
{
	constexpr TCHAR InvalidProtocolVersion[] = TEXT("Invalid Protocol version");
} // namespace MqttifyProtocolVersion
