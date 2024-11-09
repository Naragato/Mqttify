#pragma once

#include "HAL/Platform.h"

namespace Mqttify
{
	/**
	 * @brief Convert enum value to string
	 * @tparam T Enum type to convert to string
	 * @param InValue Enum value to convert to string
	 * @return	Enum value as string
	 */
	template <typename T>
	const TCHAR* EnumToTCharString(T InValue);
}
