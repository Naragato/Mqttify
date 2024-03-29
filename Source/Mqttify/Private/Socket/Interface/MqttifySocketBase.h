#pragma once
#include "Mqtt/MqttifyConnectionSettings.h"
#include "Serialization/ArrayReader.h"

namespace Mqttify
{
	using FMqttifySocketPtr = TSharedPtr<class FMqttifySocketBase>;

	/// @brief Interface for MQTT socket.
	class FMqttifySocketBase
	{
	public:
		DECLARE_MULTICAST_DELEGATE_OneParam(FOnConnectDelegate, const bool /* WasSuccessful */);
		/// @return The OnConnect event.
		FOnConnectDelegate& GetOnConnectDelegate() { return OnConnectDelegate; }

		DECLARE_MULTICAST_DELEGATE(FOnDisconnectDelegate);
		/// @return The OnDisconnect event.
		FOnDisconnectDelegate& GetOnDisconnectDelegate() { return OnDisconnectDelegate; }

		DECLARE_MULTICAST_DELEGATE_OneParam(FOnDataReceivedDelegate, TSharedPtr<FArrayReader> /* Data */)
		/// @return The OnDataReceived event.
		FOnDataReceivedDelegate& GetOnDataReceivedDelegate() { return OnDataReceiveDelegate; }

		virtual ~FMqttifySocketBase();
		/// @brief Connect to the host.
		virtual void Connect() = 0;
		/// @brief Disconnect connect from the host.
		virtual void Disconnect() = 0;
		/// @brief Check if the Socket is connected.
		virtual bool IsConnected() const = 0;


		// TODO: Should we queue packets instead of directly sending to the socket
		// then send on tick ? We could use a thread safe queue for that. Which would
		// allow us to remove the lock on the send function.

		/**
		* @brief Send data to the server.
		* @param Data The data to send.
		* @param Size The size of the data to send.
		* @return True if the data was sent successfully, false otherwise.
		*/
		virtual void Send(const uint8* Data, uint32 Size) = 0;
		/// @brief Tick the connection (e.g., poll for incoming data, check for timeouts, etc.)
		virtual void Tick() = 0;

		/// @brief Create a Socket based on the connection settings.
		static FMqttifySocketPtr Create(const FMqttifyConnectionSettingsRef& InConnectionSettings);

	protected:
		mutable FCriticalSection SocketAccessLock;
		FOnDataReceivedDelegate OnDataReceiveDelegate;
		FOnConnectDelegate OnConnectDelegate;
		FOnDisconnectDelegate OnDisconnectDelegate;
	};
} // namespace Mqttify
