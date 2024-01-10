#pragma once
#include "Mqtt/MqttifySubscribeResult.h"

namespace Mqttify
{
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSubscribe,
										const TSharedPtr<TArray<FMqttifySubscribeResult>>& /* Result per topic filter */);
} // namespace Mqttify
