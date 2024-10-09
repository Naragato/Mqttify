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
		std::atomic<bool> bIsStopping;

	public:
		TSharedPtr<FMqttifyConnectionSettings> ConnectionSettings;

		FMqttifySocketPtr SocketUnderTest;
		FRunnableThread* Thread;

		explicit FMqttifySocketRunnable(const FString& InUrl)
		{
			ConnectionSettings = FMqttifyConnectionSettingsBuilder(InUrl).Build();
			SocketUnderTest    = FMqttifySocketBase::Create(ConnectionSettings.ToSharedRef());
			Thread             = FRunnableThread::Create(this,
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

		FMqttifySocketPtr& GetSocket()
		{
			return SocketUnderTest;
		}

		virtual uint32 Run() override
		{
			while (!bIsStopping.load(std::memory_order_acquire) ||
				(SocketUnderTest.IsValid() && SocketUnderTest->IsConnected()))
			{
				SocketUnderTest->Tick();
				FPlatformProcess::YieldThread();
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
