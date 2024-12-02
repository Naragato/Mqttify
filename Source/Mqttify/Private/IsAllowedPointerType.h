#pragma once

#include "Templates/SharedPointer.h"
#include "Templates/UniquePtr.h"

#include <type_traits>


namespace Mqttify
{
	template <typename T>
	struct IsAllowedPointerType : std::false_type
	{
	};

	// Specializations for TSharedRef, TSharedPtr, and TUniquePtr
	template <typename U>
	struct IsAllowedPointerType<TSharedRef<U>> : std::true_type
	{
	};

	template <typename U>
	struct IsAllowedPointerType<TSharedPtr<U>> : std::true_type
	{
	};

	template <typename U>
	struct IsAllowedPointerType<TUniquePtr<U>> : std::true_type
	{
	};
}
