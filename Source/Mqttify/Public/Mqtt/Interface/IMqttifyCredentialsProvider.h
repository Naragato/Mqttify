#pragma once

#include "Mqtt/MqttifyCredentials.h"
#include "Templates/SharedPointer.h"

/// @brief Interface for MQTT credentials providers. To allow for different methods of obtaining credentials.
class IMqttifyCredentialsProvider
{
public:
	virtual ~IMqttifyCredentialsProvider() = default;
	virtual FMqttifyCredentials GetCredentials() = 0;
};

using FMqttifyCredentialsProviderPtr = TSharedPtr<IMqttifyCredentialsProvider>;
using FMqttifyCredentialsProviderRef = TSharedRef<IMqttifyCredentialsProvider>;