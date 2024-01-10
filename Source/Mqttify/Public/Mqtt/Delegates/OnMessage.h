#pragma once
#include "Mqtt/MqttifyMessage.h"

namespace Mqttify
{
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnMessage, const FMqttifyMessage& /* Packet */);
} // namespace Mqttify
