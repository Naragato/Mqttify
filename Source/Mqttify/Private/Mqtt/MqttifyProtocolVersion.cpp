#include "Mqtt/MqttifyProtocolVersion.h"

#include "MqttifyEnumToString.h"

namespace Mqttify
{
	template <>
	const TCHAR* EnumToTCharString<EMqttifyProtocolVersion>(const EMqttifyProtocolVersion InValue)
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