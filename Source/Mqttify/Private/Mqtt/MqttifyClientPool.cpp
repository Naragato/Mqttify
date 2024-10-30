#include "Mqtt/MqttifyClientPool.h"

#include "MqttifyClient.h"
#include "MqttifyConstants.h"
#include "Containers/BackgroundableTicker.h"
#include "HAL/RunnableThread.h"
#include "Mqtt/Interface/ITickableMqttifyClient.h"

namespace Mqttify
{
	TSharedPtr<ITickableMqttifyClient> FMqttifyClientPool::Create(
		const FMqttifyConnectionSettingsRef& InConnectionSettings,
		FDeleter&& InDeleter
		)
	{
		return MakeShareable<FMqttifyClient>(new FMqttifyClient(InConnectionSettings), MoveTemp(InDeleter));
	}

	TSharedPtr<IMqttifyClient> FMqttifyClientPool::GetOrCreateClient(
		const FMqttifyConnectionSettingsRef& InConnectionSettings
		)
	{
		FScopeLock Lock(&ClientMapLock);
		const uint32 Hash = InConnectionSettings->GetHashCode();

		if (const TWeakPtr<ITickableMqttifyClient>* Client = MqttifyClients.Find(Hash))
		{
			if (Client->IsValid())
			{
				return Client->Pin();
			}
		}

		auto Deleter = [this](const IMqttifyClient* InClient)
		{
			FScopeLock DeleterLock(&ClientMapLock);
			const FMqttifyConnectionSettingsRef ConnectionSettings = InClient->GetConnectionSettings();
			// Find a client and remove it from the list of clients.
			const uint32 DeleterHash = ConnectionSettings->GetHashCode();
			MqttifyClients.Remove(DeleterHash);
			delete InClient;

			if (MqttifyClients.IsEmpty())
			{
				Kill();
			}
		};

		TSharedPtr<ITickableMqttifyClient> OutClient = Create(InConnectionSettings, MoveTemp(Deleter));

		if (nullptr == OutClient)
		{
			return nullptr;
		}

		bool bExpected = false;

		if constexpr (GMqttifyThreadMode != EMqttifyThreadMode::GameThread)
		{
			if (nullptr == Thread && bIsRunning.compare_exchange_strong(bExpected, true, std::memory_order_acq_rel))
			{
				// Here we create the thread that will tick the clients.
				Thread = FRunnableThread::Create(
					this,
					TEXT("FMqttifyClientPool"),
					0,
					TPri_Normal,
					FPlatformAffinity::GetPoolThreadMask());
			}
		}

		MqttifyClients.Add(Hash, OutClient);
		return OutClient;
	}

	FMqttifyClientPool::FMqttifyClientPool()
		: Thread{nullptr}
	{
		if constexpr (GMqttifyThreadMode == EMqttifyThreadMode::GameThread)
		{
			// Setup our game thread tick
			const FTickerDelegate TickDelegate = FTickerDelegate::CreateRaw(this, &FMqttifyClientPool::GameThreadTick);
			// This will tick even when the game is suspended (e.g. in the background on mobile) where possible
			TickHandle = FTSBackgroundableTicker::GetCoreTicker().AddTicker(TickDelegate, 0.0f);
		}
	}

	FMqttifyClientPool::~FMqttifyClientPool()
	{
		Kill();
	}

	void FMqttifyClientPool::Kill()
	{
		bool bExpected = true;
		if (nullptr != Thread && bIsRunning.compare_exchange_strong(bExpected, false, std::memory_order_acq_rel))
		{
			if (nullptr == Thread)
			{
				return;
			}
			Thread->Kill(true);
			delete Thread;
			Thread = nullptr;
		}
	}

	uint32 FMqttifyClientPool::Run()
	{
		constexpr float MaxTickRate = 1.0f / 60.0f;
		constexpr float MinWaitTime = 0.001f;
		while (bIsRunning.load(std::memory_order_acquire))
		{
			const double StartTime = FPlatformTime::Seconds();
			Tick();
			const double EndTime = FPlatformTime::Seconds();
			const float SleepTime = MaxTickRate - static_cast<float>(EndTime - StartTime);
			FPlatformProcess::SleepNoStats(FMath::Max(MinWaitTime, SleepTime));
		}
		return 0;
	}

	void FMqttifyClientPool::Exit()
	{
		FRunnable::Exit();
	}

	FSingleThreadRunnable* FMqttifyClientPool::GetSingleThreadInterface()
	{
		return this;
	}

	bool FMqttifyClientPool::Init()
	{
		return FRunnable::Init();
	}

	void FMqttifyClientPool::Stop()
	{
		FScopeLock Lock(&ClientMapLock);
		MqttifyClients.Empty();
	}

	void FMqttifyClientPool::TickInternal()
	{
		for (auto& Client : MqttifyClients)
		{
			if (const auto ClientPtr = Client.Value.Pin())
			{
				ClientPtr->Tick();
			}
		}
	}

	void FMqttifyClientPool::Tick()
	{
		FScopeLock Lock(&ClientMapLock);
		TickInternal();
	}

	bool FMqttifyClientPool::GameThreadTick(float DeltaTime)
	{
		check(IsInGameThread());
		FScopeLock Lock(&ClientMapLock);
		for (auto& Client : MqttifyClients)
		{
			if (const auto ClientPtr = Client.Value.Pin())
			{
				ClientPtr->Tick();
			}
		}
		return true;
	}
} // namespace Mqttify
