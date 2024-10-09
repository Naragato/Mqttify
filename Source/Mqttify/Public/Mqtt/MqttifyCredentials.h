#pragma once

#include "CoreMinimal.h"

/// @brief Represents MQTT credentials.
struct MQTTIFY_API FMqttifyCredentials final
{
	/// @brief the username for the connection.
	FString Username;
	/// @brief the password for the connection.
	FString Password;
};