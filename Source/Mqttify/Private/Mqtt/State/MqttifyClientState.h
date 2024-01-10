#pragma once

#include "Mqtt/MqttifyState.h"
#include "Mqtt/State/MqttifyClientContext.h"
#include "Packets/MqttifyConnAckPacket.h"

class FArrayReader;

namespace Mqttify
{
	class IMqttifyControlPacket;
}

namespace Mqttify
{
	class IMqttifySocketTickable;
}

namespace Mqttify
{
	class FMqttifyClient;
	class IMqttifySubscribableAsync;
	class IMqttifyUnsubscribableAsync;
	class IMqttifyPublishableAsync;
	class IMqttifyDisconnectableAsync;
	class IMqttifyConnectableAsync;

	class FMqttifyClientState
	{
	public:
		DECLARE_DELEGATE_OneParam(FOnStateChangedDelegate, TUniquePtr<FMqttifyClientState>&&);

		explicit FMqttifyClientState(const FOnStateChangedDelegate& InOnStateChanged,
									const TSharedRef<FMqttifyClientContext>& InContext);

		virtual ~FMqttifyClientState() = default;

	protected:
		static FMqttifyPacketPtr CreatePacket(const TSharedPtr<FArrayReader>& InData);

		TSharedRef<FMqttifyClientContext> Context;
		/**
		 * @brief Transition to a new state
		 * @param InState The new state to transition to
		 */
		void TransitionTo(TUniquePtr<FMqttifyClientState>&& InState);
		FOnStateChangedDelegate OnStateChanged;

	private:
		FCriticalSection StateTransitionLock;

	public:
		/**
		 * @brief Get the state as a IMqttifyConnectableAsync
		 * @return A pointer to the state as a IMqttifyConnectableAsync
		 */
		virtual IMqttifyConnectableAsync* AsConnectable() { return nullptr; }
		/**
		 * @brief Get the state as a IMqttifyDisconnectableAsync
		 * @return A pointer to the state as a IMqttifyDisconnectableAsync
		 */
		virtual IMqttifyDisconnectableAsync* AsDisconnectable() { return nullptr; }
		/**
		 * @brief Get the state as a IMqttifyPublishableAsync
		 * @return A pointer to the state as a IMqttifyPublishableAsync
		 */
		virtual IMqttifyPublishableAsync* AsPublishable() { return nullptr; }
		/**
		 * @brief Get the state as a IMqttifySubscribableAsync
		 * @return A pointer to the state as a IMqttifySubscribableAsync
		 */
		virtual IMqttifySubscribableAsync* AsSubscribable() { return nullptr; }
		/**
		 * @brief Get the state as a IMqttifyUnsubscribableAsync
		 * @return A pointer to the state as a IMqttifyUnsubscribableAsync
		 */
		virtual IMqttifyUnsubscribableAsync* AsUnsubscribable() { return nullptr; }
		/**
		 * @brief Get the state as a IMqttifySocketTickable
		 * @return A pointer to the state as a IMqttifySocketTickable
		 */
		virtual IMqttifySocketTickable* AsSocketTickable() { return nullptr; }
		/**
		 * @brief Get the current state of the client
		 * @return The current state of the client
		 */
		virtual EMqttifyState GetState() = 0;
	};
} // namespace Mqttify
