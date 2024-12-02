#pragma once
#include "Mqtt/MqttifyConnectionSettings.h"
#include "Serialization/ArrayReader.h"

namespace Mqttify
{
	class IMqttifyControlPacket;
}

class MqttifyMqttifySocketSpec;
class MqttifyMqttifyWebSocketSpec;

namespace Mqttify
{
	using FMqttifySocketRef = TSharedRef<class FMqttifySocketBase>;
	using FMqttifySocketWeakPtr = TWeakPtr<FMqttifySocketBase>;

	/// @brief Interface for MQTT socket.
	class FMqttifySocketBase
	{
	public:
		DECLARE_TS_MULTICAST_DELEGATE_OneParam(FOnConnectDelegate, const bool /* WasSuccessful */);
		/// @return The OnConnect event.
		FOnConnectDelegate& GetOnConnectDelegate() { return OnConnectDelegate; }

		DECLARE_TS_MULTICAST_DELEGATE(FOnDisconnectDelegate);
		/// @return The OnDisconnect event.
		FOnDisconnectDelegate& GetOnDisconnectDelegate() { return OnDisconnectDelegate; }

		DECLARE_TS_MULTICAST_DELEGATE_OneParam(FOnDataReceivedDelegate, const TSharedPtr<FArrayReader>& /* Data */)
		/// @return The OnDataReceived event.
		FOnDataReceivedDelegate& GetOnDataReceivedDelegate() { return OnDataReceiveDelegate; }

		explicit FMqttifySocketBase(const FMqttifyConnectionSettingsRef& InConnectionSettings)
			: ConnectionSettings{InConnectionSettings}
		{
		}

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


		/// @brief Tick the connection (e.g., poll for incoming data, check for timeouts, etc.)
		virtual void Tick() = 0;

		/// @brief Create a Socket based on the connection settings.
		static FMqttifySocketRef Create(const FMqttifyConnectionSettingsRef& InConnectionSettings);

		void ReadPacketsFromBuffer();

		/**
		 * @brief Send a packet to the server.
		 * @param InPacket The packet to send.
		 */
		virtual void Send(const TSharedRef<IMqttifyControlPacket>& InPacket) = 0;

	protected:
		mutable FCriticalSection SocketAccessLock{};
		FOnDataReceivedDelegate OnDataReceiveDelegate{};
		FOnConnectDelegate OnConnectDelegate{};
		FOnDisconnectDelegate OnDisconnectDelegate{};
		TArray<uint8> DataBuffer;
		const FMqttifyConnectionSettingsRef ConnectionSettings;

	protected:
		/**
		* @brief Send data to the server.
		* @param Data The data to send.
		* @param Size The size of the data to send.
		* @return True if the data was sent successfully, false otherwise.
		*/
		virtual void Send(const uint8* Data, uint32 Size) = 0;
		friend  ::MqttifyMqttifySocketSpec;
		friend ::MqttifyMqttifyWebSocketSpec;
	};
} // namespace Mqttify
