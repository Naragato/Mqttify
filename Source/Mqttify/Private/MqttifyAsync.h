#pragma once
#include "LogMqttify.h"
#include "MqttifyConstants.h"
#include "Async/Async.h"

namespace Mqttify
{
	/**
	 * @brief Dispatches the provided function with thread handling based on the current thread mode.
	 *
	 * If the thread mode is not set to `BackgroundThreadWithCallbackMarshalling`, the function
	 * is executed immediately on the current thread. Otherwise, the function is dispatched
	 * asynchronously to the game thread.
	 *
	 * @param InFunc The function to be dispatched.
	 */
	inline void DispatchWithThreadHandling(TUniqueFunction<void()> InFunc)
	{
		if constexpr (GMqttifyThreadMode != EMqttifyThreadMode::BackgroundThreadWithCallbackMarshalling)
		{
			InFunc();
		}
		else
		{
			AsyncTask(
				ENamedThreads::GameThread,
				[Func = MoveTemp(InFunc)]() mutable
				{
					LOG_MQTTIFY(VeryVerbose, TEXT("Dispatching with thread handling"));
					Func();
				});
		}
	}

	template <typename TResult>
	/**
	 * @brief Creates a fulfilled promise that adapts to the current thread mode.
	 *
	 * If the thread mode is not set to `BackgroundThreadWithCallbackMarshalling`, the promise is fulfilled and the future is returned immediately on the current thread.
	 * Otherwise, the promise is fulfilled asynchronously on the game thread.
	 *
	 * @param Value The value to set for the promise fulfillment.
	 * @return A future that holds the result of the fulfilled promise.
	 */
	TFuture<TResult> MakeThreadAwareFulfilledPromise(TResult&& Value)
	{
		if constexpr (GMqttifyThreadMode != EMqttifyThreadMode::BackgroundThreadWithCallbackMarshalling)
		{
			// Directly fulfill the promise and return the future
			return MakeFulfilledPromise<TResult>(Forward<TResult>(Value)).GetFuture();
		}
		else
		{
			// Create a shared promise and fulfill it on the main thread
			TSharedPtr<TPromise<TResult>> Promise = MakeShared<TPromise<TResult>>();
			TFuture<TResult> Future = Promise->GetFuture();

			AsyncTask(
				ENamedThreads::GameThread,
				[Promise, Value = Forward<TResult>(Value)]() mutable
				{
					if (Promise.IsValid())
					{
						Promise->SetValue(Forward<TResult>(Value));
					}
				});

			return Future;
		}
	}
}
