#pragma once

#include "CoreMinimal.h"
#include "Async/Async.h"
#include "Mqtt/MqttifyResult.h"
#include "Socket/Interface/MqttifySocketBase.h"
#include "Templates/SharedPointer.h"
#include "Templates/UniquePtr.h"

#include <atomic>

#include "Packets/Interface/IMqttifyControlPacket.h"

enum class EMqttifyProtocolVersion : uint8;

namespace Mqttify
{
	class IMqttifyControlPacket;
	/**
	 * @brief A wrapper for packets which require acknowledgement. With built in retry logic.
	 */
	class FMqttifyAcknowledgeable
	{
	public:
		virtual ~FMqttifyAcknowledgeable() = default;

		explicit FMqttifyAcknowledgeable(const uint16 InPacketId,
										const TWeakPtr<FMqttifySocketBase>& InSocket,
										const TSharedRef<FMqttifyConnectionSettings>& InConnectionSettings)
			: Settings{ InConnectionSettings }
			, Socket{ InSocket }
			, RetryWaitTime{ FDateTime::MinValue() }
			, PacketId{ InPacketId }
			, PacketTries{ 0 } {}

		/**
		 * @brief Trigger the action
		 * @return True if the command is done.
		 */
		bool Next();

		/**
		 * @brief Abandon the command.
		 */
		virtual void Abandon() = 0;

		/**
		 * @brief Acknowledge the command
		 * @param InPacket The acknowledge packet
		 * @return True if the command is done.
		 */
		virtual bool Acknowledge(const FMqttifyPacketPtr& InPacket) = 0;

		virtual uint32 GetKey() const
		{
			return PacketId;
		}

	protected:
		/**
		 * @brief Send the packet over the Socket.
		 * @param InPacket The packet to send.
		 */
		void SendPacketInternal(const TSharedPtr<IMqttifyControlPacket>& InPacket);

		/**
		 * @brief Trigger the action
		 * @return True if the command is done.
		 */
		virtual bool NextImpl() = 0;

		/**
		 * @brief Check if the command is done.
		 * @return True if the command is done.
		 */
		virtual bool IsDone() const = 0;

		FMqttifyConnectionSettingsRef Settings;
		TWeakPtr<FMqttifySocketBase> Socket;
		FDateTime RetryWaitTime;
		mutable FCriticalSection CriticalSection;
		uint16 PacketId;
		uint8 PacketTries;
	};

	template <typename TReturnValue>
	class TMqttifyAcknowledgeable : public FMqttifyAcknowledgeable
	{
	private:
		TPromise<TMqttifyResult<TReturnValue>> CommandPromise;
		std::atomic_bool bIsDone;

	public:
		explicit TMqttifyAcknowledgeable(const uint16 InPacketId,
										const TWeakPtr<FMqttifySocketBase>& InSocket,
										const FMqttifyConnectionSettingsRef& InConnectionSettings)
			: FMqttifyAcknowledgeable{ InPacketId, InSocket, InConnectionSettings }
			, bIsDone{ false } {}

		~TMqttifyAcknowledgeable() override;

		/**
		 * @brief Get the future result of the command
		 * @return The future result of the command
		 */
		TFuture<TMqttifyResult<TReturnValue>> GetFuture()
		{
			return CommandPromise.GetFuture();
		}

	protected:
		void SetPromiseValue(TMqttifyResult<TReturnValue>&& InValue);
	};

	template <typename TReturnValue>
	TMqttifyAcknowledgeable<TReturnValue>::~TMqttifyAcknowledgeable()
	{
		SetPromiseValue(TMqttifyResult<TReturnValue>{ false });
	}

	template <typename TReturnValue>
	void TMqttifyAcknowledgeable<TReturnValue>::SetPromiseValue(TMqttifyResult<TReturnValue>&& InValue)
	{
		bool Expected = false;
		if (bIsDone.compare_exchange_strong(Expected, true, std::memory_order_relaxed))
		{
			if constexpr (GMqttifyThreadMode != EMqttifyThreadMode::BackgroundThreadWithCallbackMarshalling)
			{
				CommandPromise.SetValue(MoveTemp(InValue));
			}
			else
			{
				AsyncTask(ENamedThreads::GameThread,
						[CommandPromise = MoveTemp(CommandPromise), InValue]() mutable {
							CommandPromise.SetValue(MoveTemp(InValue));
						});
			}
		}
	}
} // namespace Mqttify
