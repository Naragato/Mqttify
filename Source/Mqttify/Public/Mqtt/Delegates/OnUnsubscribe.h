#pragma once
#include "Mqtt/MqttifyUnsubscribeResult.h"

namespace Mqttify
{
	DECLARE_TS_MULTICAST_DELEGATE_OneParam(FOnUnsubscribe,
										const TSharedPtr<TArray<FMqttifyUnsubscribeResult>>& /* Result per topic filter */)
} // namespace Mqttify
