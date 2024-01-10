#include "Socket/MqttifyWebSocket.h"

#include "LogMqttify.h"
#include "MqttifySocketState.h"
#include "WebSocketsModule.h"

namespace Mqttify
{
	FMqttifyWebSocket::FMqttifyWebSocket(const FMqttifyConnectionSettingsRef& InConnectionSettings)
		: ConnectionSettings{ InConnectionSettings }
		, CurrentState{ EMqttifySocketState::Disconnected }
		, DisconnectTime{ FDateTime::MaxValue() }
		, Packet{ MakeShared<FArrayReader>(false) } {}

	void FMqttifyWebSocket::Connect()
	{
		FScopeLock Lock(&SocketAccessLock);
		if (CurrentState != EMqttifySocketState::Disconnected)
		{
			LOG_MQTTIFY(Display, TEXT("Socket already connecting or connected."));
			return;
		}

		CurrentState = EMqttifySocketState::Connecting;

		const TArray<FString> Protocols             = { "mqtt" };
		static const TMap<FString, FString> Headers = {
			{ "Connection", "Upgrade" },
			{ "Upgrade", "websocket" },
			{ "Sec-WebSocket-Protocol", "mqtt" }
		};

		Socket = FWebSocketsModule::Get().CreateWebSocket(ConnectionSettings->ToString(), Protocols, Headers);

		if (!Socket.IsValid())
		{
			LOG_MQTTIFY(Error, TEXT("Error creating web socket."));
			OnConnectDelegate.Broadcast(false);
		}

		Socket->OnConnected().AddRaw(this, &FMqttifyWebSocket::HandleWebSocketConnected);
		Socket->OnConnectionError().AddRaw(this, &FMqttifyWebSocket::HandleWebSocketConnectionError);
		Socket->OnClosed().AddRaw(this, &FMqttifyWebSocket::HandleWebSocketConnectionClosed);
		Socket->OnRawMessage().AddRaw(this, &FMqttifyWebSocket::HandleWebSocketData);
		Socket->Connect();
	}

	void FMqttifyWebSocket::Disconnect()
	{
		FScopeLock Lock(&SocketAccessLock);
		if (IsConnected())
		{
			CurrentState = EMqttifySocketState::Disconnecting;
			LOG_MQTTIFY(Display, TEXT("Disconnecting from socket on: %s."), *ConnectionSettings->ToString());
			Socket->Close();
			const FDateTime Now = FDateTime::Now();
			DisconnectTime      = Now + FTimespan::FromSeconds(ConnectionSettings->GetSocketConnectionTimoutSeconds());
		}
	}

	void FMqttifyWebSocket::Send(const uint8* Data, const uint32 Size)
	{
		FScopeLock Lock(&SocketAccessLock);
		if (!IsConnected())
		{
			LOG_MQTTIFY(Warning, TEXT("Socket not connected."));
			return;
		}
		Socket->Send(Data, Size, true);
	}

	void FMqttifyWebSocket::Tick()
	{
		FScopeLock Lock(&SocketAccessLock);

		if (!IsConnected() && CurrentState == EMqttifySocketState::Connected)
		{
			OnDisconnectDelegate.Broadcast();
			FinalizeDisconnect();
			LOG_MQTTIFY(Warning, TEXT("Socket unexpectedly disconnected."));
			return;
		}

		if (DisconnectTime < FDateTime::Now())
		{
			LOG_MQTTIFY(Error, TEXT("Socket Connection Timeout: Address %s."), *ConnectionSettings->ToString());
			OnDisconnectDelegate.Broadcast();
			FinalizeDisconnect();
		}
	}

	bool FMqttifyWebSocket::IsConnected() const
	{
		FScopeLock Lock(&SocketAccessLock);
		return Socket.IsValid() && Socket->IsConnected() && CurrentState == EMqttifySocketState::Connected;
	}

	void FMqttifyWebSocket::HandleWebSocketConnected()
	{
		FScopeLock Lock(&SocketAccessLock);
		LOG_MQTTIFY(Display, TEXT("Connected to socket on: %s."), *ConnectionSettings->ToString());
		CurrentState   = EMqttifySocketState::Connected;
		DisconnectTime = FDateTime::MaxValue();
		OnConnectDelegate.Broadcast(true);
	}

	void FMqttifyWebSocket::HandleWebSocketConnectionError(const FString& Error)
	{
		FScopeLock Lock(&SocketAccessLock);
		LOG_MQTTIFY(Error, TEXT("Socket Connection Error: %s."), *Error);
		OnConnectDelegate.Broadcast(false);
		FinalizeDisconnect();
	}

	void FMqttifyWebSocket::FinalizeDisconnect()
	{
		FScopeLock Lock(&SocketAccessLock);
		CurrentState   = EMqttifySocketState::Disconnected;
		DisconnectTime = FDateTime::MaxValue();
		if (Socket.IsValid())
		{
			Socket->OnConnected().RemoveAll(this);
			Socket->OnConnectionError().RemoveAll(this);
			Socket->OnClosed().RemoveAll(this);
			Socket->OnRawMessage().RemoveAll(this);
			Socket.Reset();
		}
	}

	void FMqttifyWebSocket::HandleWebSocketConnectionClosed(const int32 Status,
															const FString& Reason,
															const bool bWasClean)
	{
		FScopeLock Lock(&SocketAccessLock);

		if (bWasClean)
		{
			LOG_MQTTIFY(
				Display,
				TEXT("Socket Connection Closed: Status %d, Reason %s, bWasClean %d."),
				Status,
				*Reason,
				bWasClean);
		}
		else
		{
			LOG_MQTTIFY(
				Error,
				TEXT("Socket Connection Closed: Status %d, Reason %s, bWasClean %d."),
				Status,
				*Reason,
				bWasClean);
		}
		OnDisconnectDelegate.Broadcast();
		FinalizeDisconnect();
	}

	void FMqttifyWebSocket::HandleWebSocketData(const void* Data, const SIZE_T Length, const SIZE_T BytesRemaining)
	{
		FScopeLock Lock(&SocketAccessLock);
		LOG_MQTTIFY(
			Verbose,
			TEXT("Socket Data Received: Length %d, BytesRemaining %d."),
			Length,
			BytesRemaining);

		Packet->Append(static_cast<const uint8*>(Data), Length);
		if (BytesRemaining == 0)
		{
			LOG_MQTTIFY_PACKET_DATA(VeryVerbose, static_cast<const uint8*>(Data), Length);
			GetOnDataReceivedDelegate().Broadcast(Packet);
			Packet = MakeShared<FArrayReader>(false);
		}
	}
} // namespace Mqttify
