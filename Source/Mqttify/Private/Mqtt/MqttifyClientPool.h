#pragma once
#include "Containers/Ticker.h"
#include "HAL/Runnable.h"
#include "Misc/SingleThreadRunnable.h"
#include "Mqtt/MqttifyConnectionSettings.h"

enum class EMqttifyThreadMode : uint8;
enum class EMqttifyProtocolVersion : uint8;
class IMqttifyClient;

namespace Mqttify
{
	class ITickableMqttifyClient;

	// TODO refactor to optionally include conditional code based upon the thread mode
	/**
	 * @brief A pool of MQTT clients. Clients are created on demand and destroyed when they are no longer needed.
	 * Clients can be created on the game thread or on a thread from the thread pool.
	 */
	class FMqttifyClientPool final : public FRunnable, public FSingleThreadRunnable, public TSharedFromThis<FMqttifyClientPool>
	{
	private:
		/// @brief A functor that is called when a client is destroyed.
		using FDeleter = TFunction<void(ITickableMqttifyClient*)>;

		/**
		 * @brief Create a client for the given connection settings.
		 * @tparam InProtocolVersion The MQTT protocol version of the client.
		 * @tparam InThreadMode The thread mode of the client.
		 * @return A shared pointer to the MQTT client if the URL was valid
		 */
		static TSharedPtr<ITickableMqttifyClient> Create(const FMqttifyConnectionSettingsRef& InConnectionSettings,
														FDeleter&& InDeleter);

	public:
		explicit FMqttifyClientPool();
		virtual ~FMqttifyClientPool() override;

		/**
		 * @brief Get or create a client for the given connection settings.
		 * @param InConnectionSettings The connection settings to use.
		 * @return A shared pointer to the MQTT client if the URL was valid
		 */
		TSharedPtr<IMqttifyClient> GetOrCreateClient(const FMqttifyConnectionSettingsRef& InConnectionSettings);

		void Kill();

		/* Implement FRunnable Begin */
		/// @brief Run the thread.
		virtual uint32 Run() override;
		/// @brief Exits the runnable object. Called in the context of the aggregating thread to perform any cleanup.
		virtual void Exit() override;
		/// @brief Initializes the runnable object.
		virtual bool Init() override;
		/// @brief Stops the runnable object. If a thread is requested to terminate early.
		virtual void Stop() override;
		virtual FSingleThreadRunnable* GetSingleThreadInterface() override;
		/* Implement FRunnable End */

		/* Implement FSingleThreadRunnable Begin */
		/// @brief Tick the connection (e.g., poll for incoming data, check for timeouts, etc.)
		virtual void Tick() override;
		/* Implement FSingleThreadRunnable End */
	private:

		/// @brief Tick the connection  (e.g., poll for incoming data, check for timeouts, etc.)
		void TickInternal();

		/// @brief Tick the connection on the main thread
		bool GameThreadTick(float DeltaTime);

		mutable FCriticalSection ClientMapLock;
		TMap<int32, TWeakPtr<ITickableMqttifyClient>> MqttifyClients;

		FRunnableThread* Thread;
		std::atomic<bool> bIsRunning;

		/** Delegate for callbacks to GameThreadTick */
		FTSTicker::FDelegateHandle TickHandle;
	};
} // namespace Mqttify
