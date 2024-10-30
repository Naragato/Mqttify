#include "Socket/MqttifyWebSocket.h"

#include "LogMqttify.h"
#include "MqttifySocketState.h"
#include "WebSocketsModule.h"

namespace Mqttify
{
	FMqttifyWebSocket::FMqttifyWebSocket(const FMqttifyConnectionSettingsRef& InConnectionSettings)
		: ConnectionSettings{InConnectionSettings}
		, CurrentState{EMqttifySocketState::Disconnected}
		, DisconnectTime{FDateTime::MaxValue()}
	{
	}

	void FMqttifyWebSocket::Connect()
	{
		FScopeLock Lock{&SocketAccessLock};
		LOG_MQTTIFY(
			VeryVerbose,
			TEXT("Connecting to socket on %s, ClientId %s"),
			*ConnectionSettings->ToString(),
			*ConnectionSettings->GetClientId());
		if (CurrentState != EMqttifySocketState::Disconnected)
		{
			LOG_MQTTIFY(
				Display,
				TEXT("Socket already connected %s, ClientId %s"),
				*ConnectionSettings->ToString(),
				*ConnectionSettings->GetClientId());
			return;
		}

		CurrentState = EMqttifySocketState::Connecting;

		const TArray<FString> Protocols = {TEXT("mqtt")};
		static const TMap<FString, FString> Headers = {
			{TEXT("Connection"), TEXT("Upgrade")},
			{TEXT("Upgrade"), TEXT("websocket")}
		};

		Socket = FWebSocketsModule::Get().CreateWebSocket(ConnectionSettings->ToString(), Protocols, Headers);

		if (!Socket.IsValid())
		{
			LOG_MQTTIFY(
				Error,
				TEXT("Error creating socket socket for %s, ClientId %s"),
				*ConnectionSettings->ToString(),
				*ConnectionSettings->GetClientId());
			OnConnectDelegate.Broadcast(false);
			return;
		}

		Socket->OnConnected().AddThreadSafeSP(this, &FMqttifyWebSocket::HandleWebSocketConnected);
		Socket->OnConnectionError().AddThreadSafeSP(this, &FMqttifyWebSocket::HandleWebSocketConnectionError);
		Socket->OnClosed().AddThreadSafeSP(this, &FMqttifyWebSocket::HandleWebSocketConnectionClosed);
		Socket->OnRawMessage().AddThreadSafeSP(this, &FMqttifyWebSocket::HandleWebSocketData);
		Socket->Connect();
	}

	void FMqttifyWebSocket::Disconnect()
	{
		Disconnect_Internal();
	}

	void FMqttifyWebSocket::Disconnect_Internal()
	{
		FScopeLock Lock{&SocketAccessLock};
		CurrentState = EMqttifySocketState::Disconnecting;
		LOG_MQTTIFY(
			Display,
			TEXT("Disconnecting from socket on %s, ClientId %s"),
			*ConnectionSettings->ToString(),
			*ConnectionSettings->GetClientId());
		if (Socket.IsValid() && Socket->IsConnected())
		{
			Socket->Close();
			const FDateTime Now = FDateTime::Now();
			DisconnectTime = Now + FTimespan::FromSeconds(ConnectionSettings->GetSocketConnectionTimeoutSeconds());
		}
		else
		{
			FinalizeDisconnect();
		}
	}

	void FMqttifyWebSocket::Send(const uint8* Data, const uint32 Size)
	{
		FScopeLock Lock{&SocketAccessLock};
		LOG_MQTTIFY(
			VeryVerbose,
			TEXT("Sending data to socket %s, ClientId %s"),
			*ConnectionSettings->ToString(),
			*ConnectionSettings->GetClientId());
		if (!IsConnected())
		{
			LOG_MQTTIFY(
				Warning,
				TEXT("Socket not connected %s, ClientId %s"),
				*ConnectionSettings->ToString(),
				*ConnectionSettings->GetClientId());
			return;
		}

		LOG_MQTTIFY_PACKET_DATA(VeryVerbose, Data, Size);
		Socket->Send(Data, Size, true);
	}

	void FMqttifyWebSocket::Tick()
	{
		if (!IsConnected() && CurrentState == EMqttifySocketState::Connected)
		{
			FinalizeDisconnect();
			LOG_MQTTIFY(
				Error,
				TEXT("Unexpected Disconnection %s, ClientId %s"),
				*ConnectionSettings->ToString(),
				*ConnectionSettings->GetClientId());
			return;
		}

		if (DisconnectTime < FDateTime::Now())
		{
			LOG_MQTTIFY(
				Error,
				TEXT("Timeout Socket Connection %s, ClientId %s"),
				*ConnectionSettings->ToString(),
				*ConnectionSettings->GetClientId());
			FinalizeDisconnect();
		}
	}

	bool FMqttifyWebSocket::IsConnected() const
	{
		FScopeLock Lock{&SocketAccessLock};
		return Socket.IsValid() && Socket->IsConnected() && CurrentState == EMqttifySocketState::Connected;
	}

	void FMqttifyWebSocket::HandleWebSocketConnected()
	{
		FScopeLock Lock{&SocketAccessLock};
		LOG_MQTTIFY(Display, TEXT("Connected to socket on: %s"), *ConnectionSettings->ToString());
		CurrentState = EMqttifySocketState::Connected;
		DisconnectTime = FDateTime::MaxValue();
		OnConnectDelegate.Broadcast(true);
	}

	void FMqttifyWebSocket::HandleWebSocketConnectionError(const FString& Error)
	{
		FScopeLock Lock{&SocketAccessLock};
		LOG_MQTTIFY(
			Error,
			TEXT("Socket Connection %s, ClientId %s. Error: %s"),
			*ConnectionSettings->ToString(),
			*ConnectionSettings->GetClientId(),
			*Error);
		FinalizeDisconnect();
		OnConnectDelegate.Broadcast(false);
	}

	void FMqttifyWebSocket::FinalizeDisconnect()
	{
		FScopeLock Lock{&SocketAccessLock};
		LOG_MQTTIFY(
			VeryVerbose,
			TEXT("Finalizing Disconnect Connection %s, ClientId %s"),
			*ConnectionSettings->ToString(),
			*ConnectionSettings->GetClientId());
		CurrentState = EMqttifySocketState::Disconnected;
		DisconnectTime = FDateTime::MaxValue();
		OnDisconnectDelegate.Broadcast();
		if (Socket.IsValid())
		{
			Socket->OnConnected().RemoveAll(this);
			Socket->OnConnectionError().RemoveAll(this);
			Socket->OnClosed().RemoveAll(this);
			Socket->OnRawMessage().RemoveAll(this);
			Socket.Reset();
		}
	}

	void FMqttifyWebSocket::HandleWebSocketConnectionClosed(
		const int32 Status,
		const FString& Reason,
		const bool bWasClean
		)
	{
		FScopeLock Lock{&SocketAccessLock};
		LOG_MQTTIFY(
			Verbose,
			TEXT("Socket Connection Closed Connection %s, ClientId %s: Status %d, Reason %s, bWasClean %d"),
			*ConnectionSettings->ToString(),
			*ConnectionSettings->GetClientId(),
			Status,
			*Reason,
			bWasClean);

		if (bWasClean)
		{
			LOG_MQTTIFY(
				Display,
				TEXT("Socket Connection Closed Connection %s, ClientId %s: Status %d, Reason %s, bWasClean %d"),
				*ConnectionSettings->ToString(),
				*ConnectionSettings->GetClientId(),
				Status,
				*Reason,
				bWasClean);
		}
		else
		{
			LOG_MQTTIFY(
				Error,
				TEXT("Socket Connection Closed Connection %s, ClientId %s: Status %d, Reason %s, bWasClean %d"),
				*ConnectionSettings->ToString(),
				*ConnectionSettings->GetClientId(),
				Status,
				*Reason,
				bWasClean);
		}

		FinalizeDisconnect();
	}

	void FMqttifyWebSocket::HandleWebSocketData(const void* Data, SIZE_T Length, SIZE_T BytesRemaining)
	{
		LOG_MQTTIFY(
			Verbose,
			TEXT("Socket Data Received (%s , %s): Length %llu, BytesRemaining %llu"),
			*ConnectionSettings->ToString(),
			*ConnectionSettings->GetClientId(),
			Length,
			BytesRemaining);

		DataBuffer.Append(static_cast<const uint8*>(Data), Length);
		if (BytesRemaining > 0)
		{
			return;
		}
		ParsePacket();
	}

	void FMqttifyWebSocket::ParsePacket()
	{
		while (DataBuffer.Num() >= 1)
		{
			constexpr int32 PacketStartIndex = 0;
			uint32 RemainingLength = 0;
			uint32 Multiplier = 1;
			int32 Index = 1;
			bool bHaveRemainingLength = false;

			for (; Index < 5; ++Index)
			{
				if (Index >= DataBuffer.Num())
				{
					return;
				}

				const uint8 EncodedByte = DataBuffer[Index];
				RemainingLength += (EncodedByte & 127) * Multiplier;
				Multiplier *= 128;

				if ((EncodedByte & 128) == 0)
				{
					bHaveRemainingLength = true;
					++Index;
					break;
				}
			}

			if (!bHaveRemainingLength)
			{
				LOG_MQTTIFY(VeryVerbose, TEXT("Header not complete yet"));
				return;
			}

			if (RemainingLength > ConnectionSettings->GetMaxPacketSize())
			{
				LOG_MQTTIFY(
					Error,
					TEXT("Packet too large: %d, Max: %d"),
					RemainingLength,
					ConnectionSettings->GetMaxPacketSize());
				Disconnect();
				return;
			}

			const int32 FixedHeaderSize = Index;
			const int32 TotalPacketSize = FixedHeaderSize + RemainingLength;

			if (DataBuffer.Num() < TotalPacketSize)
			{
				// We don't have the full packet yet
				return;
			}

			TSharedPtr<FArrayReader> Packet = MakeShared<FArrayReader>(false);
			Packet->Append(&DataBuffer[PacketStartIndex], TotalPacketSize);

			DataBuffer.RemoveAt(0, TotalPacketSize);

			if (GetOnDataReceivedDelegate().IsBound())
			{
				GetOnDataReceivedDelegate().Broadcast(Packet);
			}

			LOG_MQTTIFY(VeryVerbose, TEXT("Packet received of size %d"), TotalPacketSize);
		}
	}
} // namespace Mqttify
