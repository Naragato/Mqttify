#pragma once

#include "MqttifyThreadMode.h"
#include "Mqtt/MqttifyProtocolVersion.h"

namespace Mqttify
{
#if MQTTIFY_PROTOCOL_VERSION == 3
	constexpr EMqttifyProtocolVersion GMqttifyProtocol = EMqttifyProtocolVersion::Mqtt_3_1_1;
#elif MQTTIFY_PROTOCOL_VERSION == 5
	constexpr EMqttifyProtocolVersion GMqttifyProtocol = EMqttifyProtocolVersion::Mqtt_5;
#else
#error "Invalid protocol version."
#endif

#if MQTTIFY_THREAD == 0
	constexpr EMqttifyThreadMode GMqttifyThreadMode = EMqttifyThreadMode::GameThread;
#elif MQTTIFY_THREAD == 1
	constexpr EMqttifyThreadMode GMqttifyThreadMode = EMqttifyThreadMode::BackgroundThreadWithCallbackMarshalling;
#elif MQTTIFY_THREAD == 2
	constexpr EMqttifyThreadMode GMqttifyThreadMode = EMqttifyThreadMode::BackgroundThreadWithoutCallbackMarshalling;
#else
	constexpr EMqttifyThreadMode GMqttifyThreadMode = EMqttifyThreadMode::GameThread;
#error "Invalid thread mode."
#endif

	constexpr TCHAR GProtocolName[] = TEXT("Mqtt");
}
