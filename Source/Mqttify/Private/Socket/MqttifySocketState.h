#pragma once

#include "MqttifyEnumToString.h"

namespace Mqttify
{
	/// @brief Enum representing the state of a socket
	enum class EMqttifySocketState
	{
		Disconnected,
		Connecting,
		Connected,
		Disconnecting
	};

	template <>
	FORCEINLINE const TCHAR* EnumToTCharString<EMqttifySocketState>(const EMqttifySocketState InState)
	{
		switch (InState)
		{
			case EMqttifySocketState::Disconnected:
				return TEXT("Disconnected");
			case EMqttifySocketState::Connecting:
				return TEXT("Connecting");
			case EMqttifySocketState::Connected:
				return TEXT("Connected");
			case EMqttifySocketState::Disconnecting:
				return TEXT("Disconnecting");
			default:
				return TEXT("Unknown");
		}
	}
}
