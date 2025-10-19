#pragma once

#include "Mqtt/MqttifyCredentials.h"
#include "Templates/SharedPointer.h"

/// @brief Interface for MQTT credentials providers. To allow for different methods of obtaining credentials.
class IMqttifyCredentialsProvider
{
public:
	virtual ~IMqttifyCredentialsProvider() = default;
	virtual FMqttifyCredentials GetCredentials() = 0;

	/**
	 * @brief // Enhanced authentication support (MQTT v5, AUTH packets)
	 * @return the authentication method name if using enhanced auth (e.g., "SCRAM-SHA-1", "GS2-KRB5").
	 * empty string if not using enhanced auth.
	 **/
	virtual FString GetAuthMethod() { return FString(); }

	/**
	 * 
	 * @return Optional initial authentication data to include in CONNECT when the method requires client-first data.
	 */
	virtual TArray<uint8> GetInitialAuthData() { return TArray<uint8>(); }

	/**
	* @brief Handle a server AUTH challenge and return the next client authentication data.
	 * @return The response to the server challenge.
	 * The default implementation returns an empty array, meaning no additional data.
	 */
	virtual TArray<uint8> OnAuthChallenge(const TArray<uint8>& /*ServerData*/) { return TArray<uint8>(); }
};

using FMqttifyCredentialsProviderPtr = TSharedPtr<IMqttifyCredentialsProvider>;
using FMqttifyCredentialsProviderRef = TSharedRef<IMqttifyCredentialsProvider>;