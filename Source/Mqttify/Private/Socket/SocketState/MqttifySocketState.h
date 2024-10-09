#pragma once

#include "SocketSubsystem.h"
#include "Socket/MqttifySocketState.h"

#define UI UI_ST
#include <openssl/ssl.h>
#undef UI

class IMqttifySocket;

namespace Mqttify
{
	class FMqttifySocket;
	class IMqttifyClient;
	class IMqttifyControlPacket;
	class IMqttifySocketConnectable;
	class IMqttifySocketDisconnectable;
	class IMqttifySocketSendable;
	class IMqttifySocketTickable;

	/// @breif Interface for MQTT socket subsystem state
	class FMqttifySocketState
	{
	protected:
		SSL* Ssl;
		/// @brief a pointer to the socket
		FUniqueSocket Socket;
		/// @brief a pointer to the FSM that owns this state
		FMqttifySocket* MqttifySocket;

		/**
		 * @brief Transition to a new state
		 * @param InNewState the new state to transition to
		 */
		void TransitionTo(TUniquePtr<FMqttifySocketState>&& InNewState) const;

	public:
		explicit FMqttifySocketState(FUniqueSocket&& InSocket, FMqttifySocket* InMqttifySocket, SSL* InSsl = nullptr);
		virtual ~FMqttifySocketState() {}

		/// @brief cast to a connectable state
		virtual IMqttifySocketConnectable* AsConnectable() { return nullptr; }
		/// @brief cast to a disconnectable state
		virtual IMqttifySocketDisconnectable* AsDisconnectable() { return nullptr; }
		/// @brief cast to a sendable state
		virtual IMqttifySocketSendable* AsSendable() { return nullptr; }
		/// @brief cast to a tickable state
		virtual IMqttifySocketTickable* AsTickable() { return nullptr; }

		/// @brief get the socket state
		virtual EMqttifySocketState GetState() const = 0;

		/**
		 * @brief Check if the socket is connected
		 * @return true if the socket is connected, false otherwise
		 */
		bool IsConnected() const;
	};
} // namespace Mqttify
