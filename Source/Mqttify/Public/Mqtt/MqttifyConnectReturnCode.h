#pragma once

#include "CoreMinimal.h"
#include "MqttifyEnumToString.h"
#include "MqttifyConnectReturnCode.generated.h"

/**
 * @brief http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc385349257
 * Byte 2 in the Variable header,
 */
UENUM(BlueprintType)
enum class EMqttifyConnectReturnCode : uint8
{
	/**
	 * @brief The Connection is accepted by the Server.
	 */
	Accepted = 0,
	/**
     * @brief The Server does not support the level of the Mqtt protocol requested by the Client.
     */
	RefusedProtocolVersion = 1,
	/**
	 * @brief The Client identifier is correct UTF-8 but not allowed by the Server.
	 */
	RefusedIdentifierRejected = 2,
	/**
	 * @brief The Network Connection has been made but the Mqtt service is unavailable.
	 */
	RefusedServerUnavailable = 3,
	/**
	 * @brief The data in the user name or password is malformed.
	 */
	RefusedBadUserNameOrPassword = 4,
	/**
	 * @brief The Client is not authorized to connect.
	 */
	RefusedNotAuthorized = 5,

	/**
	 * @brief Connection Request Cancelled
	 */
	Cancelled = 255,
};

namespace MqttifyConnectReturnCode
{
	constexpr TCHAR InvalidReturnCode[] = TEXT("Invalid Return code.");
}

namespace Mqttify
{
	template <>
	FORCEINLINE const TCHAR* EnumToTCharString<EMqttifyConnectReturnCode>(const EMqttifyConnectReturnCode InValue)
	{
		switch (InValue)
		{
			case EMqttifyConnectReturnCode::Accepted:
				return TEXT("Accepted");
			case EMqttifyConnectReturnCode::RefusedProtocolVersion:
				return TEXT("Refused: Protocol version");
			case EMqttifyConnectReturnCode::RefusedIdentifierRejected:
				return TEXT("Refused: Identifier rejected");
			case EMqttifyConnectReturnCode::RefusedServerUnavailable:
				return TEXT("Refused: Server unavailable");
			case EMqttifyConnectReturnCode::RefusedBadUserNameOrPassword:
				return TEXT("Refused: Bad user name or password");
			case EMqttifyConnectReturnCode::RefusedNotAuthorized:
				return TEXT("Refused: Not authorized");
			default:
				return TEXT("Unknown");
		}
	}
} // namespace Mqttify
