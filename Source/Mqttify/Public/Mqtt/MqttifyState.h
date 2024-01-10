#pragma once

#include "MqttifyEnumToString.h"

/// @brief The state of the MQTT client
UENUM(BlueprintType)
enum class EMqttifyState : uint8
{
	/// @brief The client is disconnected and will only reconnect when triggered manually
	Disconnected,
	/// @brief The client is connecting if we fail to connect, we will try to reconnect based upon the reconnect policy
	Connecting,
	/// @brief The client is connected
	Connected,
	/// @brief The client is disconnecting, only used for graceful disconnects
	Disconnecting
};

namespace Mqttify
{
	template <>
	FORCEINLINE const TCHAR* EnumToTCharString<EMqttifyState>(const EMqttifyState InValue)
	{
		switch (InValue)
		{
			case EMqttifyState::Disconnected:
				return TEXT("Disconnected");
			case EMqttifyState::Connecting:
				return TEXT("Connecting");
			case EMqttifyState::Connected:
				return TEXT("Connected");
			case EMqttifyState::Disconnecting:
				return TEXT("Disconnecting");
			default:
				return TEXT("Unknown");
		}
	}
}
