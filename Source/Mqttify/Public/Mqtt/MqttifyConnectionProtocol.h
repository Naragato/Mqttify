#pragma once

#include "CoreMinimal.h"
#include "MqttifyEnumToString.h"
#include "MqttifyConnectionProtocol.generated.h"

/**
 * @enum EMqttifyConnectionProtocol
 * @brief Enumerates supported MQTT connection protocols.
 */
UENUM(BlueprintType)
enum class EMqttifyConnectionProtocol : uint8
{
	/** 
	 * @brief MQTT protocol over TCP.
	 * Plain MQTT protocol without any additional layer of security.
	 */
	Mqtt = 0,

	/** 
	 * @brief MQTT protocol over TLS.
	 * MQTT with Transport Layer Security.
	 */
	Mqtts = 1,

	/** 
	 * @brief MQTT protocol over Websockets.
	 * MQTT over a WebSocket protocol.
	 */
	Ws = 2,

	/** 
	 * @brief MQTT protocol over secure Websockets.
	 * MQTT over a WebSocket protocol with TLS security.
	 */
	Wss = 3,
};

namespace MqttifyConnectionProtocol
{
	constexpr TCHAR InvalidConnectionProtocol[] = TEXT("Invalid Connection protocol.");
}

namespace Mqttify
{
	template <>
	FORCEINLINE const TCHAR* EnumToTCharString<EMqttifyConnectionProtocol>(const EMqttifyConnectionProtocol InValue)
	{
		switch (InValue)
		{
			case EMqttifyConnectionProtocol::Mqtt:
				return TEXT("mqtt");
			case EMqttifyConnectionProtocol::Mqtts:
				return TEXT("mqtts");
			case EMqttifyConnectionProtocol::Ws:
				return TEXT("ws");
			case EMqttifyConnectionProtocol::Wss:
				return TEXT("wss");
			default:
				return TEXT("INVALID");
		}
	}
} // namespace Mqttify
