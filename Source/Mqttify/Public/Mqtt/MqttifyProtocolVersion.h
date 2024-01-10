#pragma once

#include "CoreMinimal.h"
#include "MqttifyEnumToString.h"
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

namespace Mqttify
{
	template <>
	FORCEINLINE const TCHAR* EnumToTCharString<EMqttifyProtocolVersion>(const EMqttifyProtocolVersion InValue)
	{
		switch (InValue)
		{
			case EMqttifyProtocolVersion::Mqtt_3_1_1:
				return TEXT("MQTT Version 3.1.1");
			case EMqttifyProtocolVersion::Mqtt_5:
				return TEXT("MQTT Version 5");
			default:
				return TEXT("Unknown");
		}
	}
}

namespace MqttifyProtocolVersion
{
	constexpr TCHAR InvalidProtocolVersion[] = TEXT("Invalid Protocol version.");
} // namespace MqttifyProtocolVersion
