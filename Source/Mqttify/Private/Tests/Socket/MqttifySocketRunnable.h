#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "CoreMinimal.h"
#include "Mqtt/MqttifyConnectionSettingsBuilder.h"
#include "Socket/Interface/MqttifySocketBase.h"

namespace Mqttify
{
	class FMqttifySocketRunnable final : public FRunnable
	{
		std::atomic<bool> bIsStopping = false;

	public:
		TSharedPtr<FMqttifyConnectionSettings> ConnectionSettings;

		FMqttifySocketRef SocketUnderTest;
		FRunnableThread* Thread;

		explicit FMqttifySocketRunnable(const FString& InUrl) : SocketUnderTest{FMqttifySocketBase::Create(FMqttifyConnectionSettingsBuilder(InUrl).Build().ToSharedRef())}
		{
			SocketUnderTest->GetOnConnectDelegate().AddLambda(
				[this](const bool bWasSuccessful)
				{
					if (!bWasSuccessful)
					{
						bIsStopping.store(true, std::memory_order_release);
					}
				});

			SocketUnderTest->GetOnDisconnectDelegate().AddLambda(
				[this]
				{
					bIsStopping.store(true, std::memory_order_release);
				});

			Thread = FRunnableThread::Create(
				this,
				TEXT("SocketRunnable"),
				0,
				TPri_Normal,
				FPlatformAffinity::GetPoolThreadMask());
		}

		virtual ~FMqttifySocketRunnable() override
		{
			Thread->Kill(true);
			delete Thread;
		}

		FMqttifySocketRef& GetSocket()
		{
			return SocketUnderTest;
		}

		virtual uint32 Run() override
		{
			while (!bIsStopping.load(std::memory_order_acquire))
			{
				SocketUnderTest->Tick();
				FPlatformProcess::SleepNoStats(0.0001f);
			}
			return 0;
		}

		virtual void Stop() override
		{
			FRunnable::Stop();
			SocketUnderTest->Disconnect();
			bIsStopping.store(true, std::memory_order_release);
		}
	};
} // namespace Mqttify
#endif // WITH_DEV_AUTOMATION_TESTS
