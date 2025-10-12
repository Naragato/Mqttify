#pragma once

#include "Mqtt/MqttifyState.h"
#include "Mqtt/State/MqttifyClientContext.h"
#include "Packets/MqttifyConnAckPacket.h"

class FArrayReader;

namespace Mqttify
{
	class FMqttifyClientState
	{
	public:
		DECLARE_DELEGATE_TwoParams(
			FOnStateChangedDelegate,
			const FMqttifyClientState*,
			const TSharedPtr<FMqttifyClientState>&);

		explicit FMqttifyClientState(
			const FOnStateChangedDelegate& InOnStateChanged,
			const TSharedRef<FMqttifyClientContext>& InContext
			);

		virtual ~FMqttifyClientState() = default;

	protected:
		/// @brief Create a new packet
		static FMqttifyPacketPtr CreatePacket(const TSharedPtr<FArrayReader>& InData);
		/**
		 * @brief Transition to a new state
		 * @param InState The new state to transition to
		 */
		void TransitionTo(const TSharedPtr<FMqttifyClientState>& InState) const;
		FOnStateChangedDelegate OnStateChanged;

		TSharedRef<FMqttifyClientContext> Context;

	public:
		/**
		 * @brief Get the state as an IMqttifyConnectableAsync
		 * @return A pointer to the state as a IMqttifyConnectableAsync
		 */
		virtual class IMqttifyConnectableAsync* AsConnectable() { return nullptr; }
		/**
		 * @brief Get the state as an IMqttifyDisconnectableAsync
		 * @return A pointer to the state as a IMqttifyDisconnectableAsync
		 */
		virtual class IMqttifyDisconnectableAsync* AsDisconnectable() { return nullptr; }
		/**
		 * @brief Get the state as an IMqttifyPublishableAsync
		 * @return A pointer to the state as a IMqttifyPublishableAsync
		 */
		virtual class IMqttifyPublishableAsync* AsPublishable() { return nullptr; }
		/**
		 * @brief Get the state as an IMqttifySubscribableAsync
		 * @return A pointer to the state as a IMqttifySubscribableAsync
		 */
		virtual class IMqttifySubscribableAsync* AsSubscribable() { return nullptr; }
		/**
		 * @brief Get the state as an IMqttifyUnsubscribableAsync
		 * @return A pointer to the state as a IMqttifyUnsubscribableAsync
		 */
		virtual class IMqttifyUnsubscribableAsync* AsUnsubscribable() { return nullptr; }
		/**
		 * @brief Get the state as an IMqttifySocketTickable
		 * @return A pointer to the state as a IMqttifySocketTickable
		 */
		virtual class IMqttifySocketTickable* AsSocketTickable() { return nullptr; }
		/**
		 * @brief Get the state as an IMqttifySocketDisconnectHandler
		 * @return A pointer to the state as a IMqttifySocketDisconnectHandler
		 */
		virtual class IMqttifySocketDisconnectHandler* AsSocketDisconnectHandler() { return nullptr; }
		/**
		 * @brief Get the state as an IMqttifySocketConnectedHandler
		 * @return A pointer to the state as a IMqttifySocketConnectedHandler
		 */
		virtual class IMqttifySocketConnectedHandler* AsSocketConnectedHandler() { return nullptr; }
		/**
		 * @brief Get the current state of the client
		 * @return The current state of the client
		 */
		virtual EMqttifyState GetState() = 0;
		/**
		 * @brief Get the state as an IMqttifyPacketReceiver
		 * @return A pointer to the state as a IMqttifyPacketReceiver
		 */
		virtual class IMqttifyPacketReceiver* AsPacketReceiver() { return nullptr; }
	};
} // namespace Mqttify