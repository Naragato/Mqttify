#include "Mqtt/MqttifyQualityOfService.h"

#include "MqttifyEnumToString.h"

namespace Mqttify
{
	template <>
	const TCHAR* EnumToTCharString<EMqttifyQualityOfService>(const EMqttifyQualityOfService InValue)
	{
		switch (InValue)
		{
		case EMqttifyQualityOfService::AtMostOnce:
			return TEXT("QoS 0 - At most once");
		case EMqttifyQualityOfService::AtLeastOnce:
			return TEXT("QoS 1 - At least once");
		case EMqttifyQualityOfService::ExactlyOnce:
			return TEXT("QoS 2 - Exactly once");
		default:
			return MqttifyQualityOfService::InvalidQualityOfService;
		}
	}
} // namespace Mqttify
