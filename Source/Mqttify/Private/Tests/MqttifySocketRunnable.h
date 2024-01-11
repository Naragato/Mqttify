#pragma once

#if WITH_AUTOMATION_TESTS

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

		~FMqttifySocketRunnable() override
		{
			Thread->Kill(true);
			delete Thread;
		}

		FMqttifySocketPtr& GetSocket()
		{
			return SocketUnderTest;
		}

		uint32 Run() override
		{
			while (!bIsStopping.load(std::memory_order_acquire) ||
				(SocketUnderTest.IsValid() && SocketUnderTest->IsConnected()))
			{
				SocketUnderTest->Tick();
				FPlatformProcess::YieldThread();
			}
			return 0;
		}

		void Stop() override
		{
			FRunnable::Stop();
			SocketUnderTest->Disconnect();
			bIsStopping.store(true, std::memory_order_release);
		}
	};
} // namespace Mqttify
#endif // WITH_AUTOMATION_TESTS
