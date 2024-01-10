#pragma once
#include "HAL/Platform.h"

enum class EMqttifyThreadMode : uint8
{
	GameThread = 0,
	BackgroundThreadWithoutCallbackMarshalling = 1,
	BackgroundThreadWithCallbackMarshalling = 2
};
