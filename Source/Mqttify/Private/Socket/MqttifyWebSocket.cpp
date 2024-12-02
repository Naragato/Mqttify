#include "Socket/MqttifyWebSocket.h"

#include "LogMqttify.h"
#include "MqttifyConstants.h"
#include "MqttifySocketState.h"
#include "WebSocketsModule.h"
#include "Async/Async.h"

namespace Mqttify
{
	FMqttifyWebSocket::FMqttifyWebSocket(const FMqttifyConnectionSettingsRef& InConnectionSettings)
		: FMqttifySocketBase{InConnectionSettings}
		, CurrentState{EMqttifySocketState::Disconnected}
		, DisconnectTime{FDateTime::MaxValue()}
	{
	}

	FMqttifyWebSocket::~FMqttifyWebSocket()
	{
		Disconnect_Internal();
		{
			FScopeLock Lock{&SocketAccessLock};
			if (Socket.IsValid())
			{
				Socket->OnConnected().RemoveAll(this);
				Socket->OnConnectionError().RemoveAll(this);
				Socket->OnClosed().RemoveAll(this);
				Socket->OnRawMessage().RemoveAll(this);
				Socket.Reset();
			}
		}
	}

	void FMqttifyWebSocket::Connect_Internal()
	{
		{
			FScopeLock Lock{&SocketAccessLock};
			if (CurrentState.load(std::memory_order_acquire) != EMqttifySocketState::Connecting)
			{
				return;
			}
			if (!Socket.IsValid())
			{
				const TArray<FString> Protocols = {TEXT("mqtt")};
				static const TMap<FString, FString> Headers = {
					{TEXT("Connection"), TEXT("Upgrade")},
					{TEXT("Upgrade"), TEXT("websocket")}
				};
				Socket = FWebSocketsModule::Get().CreateWebSocket(ConnectionSettings->ToString(), Protocols, Headers);
			}

			if (Socket.IsValid())
			{
				if (!Socket->OnConnected().IsBoundToObject(this))
				{
					Socket->OnConnected().AddThreadSafeSP(this, &FMqttifyWebSocket::HandleWebSocketConnected);
				}

				if (!Socket->OnConnectionError().IsBoundToObject(this))
				{
					Socket->OnConnectionError().AddThreadSafeSP(
						this,
						&FMqttifyWebSocket::HandleWebSocketConnectionError);
				}

				if (!Socket->OnClosed().IsBoundToObject(this))
				{
					Socket->OnClosed().AddThreadSafeSP(this, &FMqttifyWebSocket::HandleWebSocketConnectionClosed);
				}

				if (!Socket->OnRawMessage().IsBoundToObject(this))
				{
					Socket->OnRawMessage().AddThreadSafeSP(this, &FMqttifyWebSocket::HandleWebSocketData);
				}

				LOG_MQTTIFY(
					Verbose,
					TEXT("Connecting to socket on %s, ClientId %s"),
					*ConnectionSettings->ToString(),
					*ConnectionSettings->GetClientId());
				Socket->Connect();
				return;
			}
		}

		LOG_MQTTIFY(
			Error,
			TEXT("Error creating socket socket for %s, ClientId %s"),
			*ConnectionSettings->ToString(),
			*ConnectionSettings->GetClientId());
		OnConnectDelegate.Broadcast(false);
	}

	void FMqttifyWebSocket::Connect()
	{
		LOG_MQTTIFY(
			VeryVerbose,
			TEXT("Connecting to socket on %s, ClientId %s"),
			*ConnectionSettings->ToString(),
			*ConnectionSettings->GetClientId());

		EMqttifySocketState Expected = EMqttifySocketState::Disconnected;
		if (!CurrentState.compare_exchange_strong(
			Expected,
			EMqttifySocketState::Connecting,
			std::memory_order_acq_rel,
			std::memory_order_acquire))
		{
			LOG_MQTTIFY(
				Warning,
				TEXT("Socket already connecting %s, ClientId %s"),
				*ConnectionSettings->ToString(),
				*ConnectionSettings->GetClientId());
			return;
		}

		if constexpr (GMqttifyThreadMode == EMqttifyThreadMode::GameThread)
		{
			Connect_Internal();
		}
		else
		{
			TWeakPtr<FMqttifyWebSocket> WeakSelf = AsShared();
			AsyncTask(
				ENamedThreads::GameThread,
				[WeakSelf]
				{
					if (const TSharedPtr<FMqttifyWebSocket> StrongThis = WeakSelf.Pin())
					{
						StrongThis->Connect_Internal();
					}
				});
		}
	}

	void FMqttifyWebSocket::Disconnect()
	{
		EMqttifySocketState Expected = CurrentState.load(std::memory_order_acquire);

		if (Expected == EMqttifySocketState::Disconnected || Expected == EMqttifySocketState::Disconnecting)
		{
			LOG_MQTTIFY(
				Warning,
				TEXT("Socket already disconnected %s, ClientId %s"),
				*ConnectionSettings->ToString(),
				*ConnectionSettings->GetClientId());
			return;
		}

		if (!CurrentState.compare_exchange_strong(
			Expected,
			EMqttifySocketState::Disconnecting,
			std::memory_order_acq_rel,
			std::memory_order_acquire))
		{
			LOG_MQTTIFY(
				Warning,
				TEXT("Socket already disconnecting %s, ClientId %s"),
				*ConnectionSettings->ToString(),
				*ConnectionSettings->GetClientId());
			return;
		}

		if constexpr (GMqttifyThreadMode == EMqttifyThreadMode::GameThread)
		{
			Disconnect_Internal();
		}
		else
		{
			TWeakPtr<FMqttifyWebSocket> WeakSelf = AsShared();
			AsyncTask(
				ENamedThreads::GameThread,
				[WeakSelf]
				{
					if (const TSharedPtr<FMqttifyWebSocket> StrongThis = WeakSelf.Pin())
					{
						StrongThis->Disconnect_Internal();
					}
				});
		}
	}

	void FMqttifyWebSocket::Disconnect_Internal()
	{
		FScopeLock Lock{&SocketAccessLock};
		CurrentState.store(EMqttifySocketState::Disconnecting, std::memory_order_release);
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
		if (!IsConnected())
		{
			LOG_MQTTIFY(
				Warning,
				TEXT("Socket not connected %s, ClientId %s"),
				*ConnectionSettings->ToString(),
				*ConnectionSettings->GetClientId());
			return;
		}

		LOG_MQTTIFY_PACKET_DATA(
			VeryVerbose,
			Data,
			Size,
			TEXT("Sending data to socket %s, ClientId %s"),
			*ConnectionSettings->ToString(),
			*ConnectionSettings->GetClientId());
		Socket->Send(Data, Size, true);
	}

	void FMqttifyWebSocket::Tick()
	{
		if (!IsConnected() && CurrentState.load(std::memory_order_acquire) == EMqttifySocketState::Connected)
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
		return Socket.IsValid() && Socket->IsConnected() && CurrentState.load(std::memory_order_acquire) ==
			EMqttifySocketState::Connected;
	}

	void FMqttifyWebSocket::Send(const TSharedRef<IMqttifyControlPacket>& InPacket)
	{
		if constexpr (GMqttifyThreadMode == EMqttifyThreadMode::GameThread)
		{
			TArray<uint8> ActualBytes;
			FMemoryWriter Writer(ActualBytes);
			InPacket->Encode(Writer);
			Send(ActualBytes.GetData(), ActualBytes.Num());
		}
		else
		{
			TWeakPtr<FMqttifyWebSocket> WeakSelf = AsShared();
			AsyncTask(
				ENamedThreads::GameThread,
				[this, InPacket=InPacket, WeakSelf]
				{
					if (const TSharedPtr<FMqttifyWebSocket> StrongThis = WeakSelf.Pin())
					{
						TArray<uint8> ActualBytes;
						FMemoryWriter Writer(ActualBytes);
						InPacket->Encode(Writer);
						Send(ActualBytes.GetData(), ActualBytes.Num());
					}
				});
		}
	}

	void FMqttifyWebSocket::HandleWebSocketConnected()
	{
		{
			FScopeLock Lock{&SocketAccessLock};
			LOG_MQTTIFY(Display, TEXT("Connected to socket on: %s"), *ConnectionSettings->ToString());
			CurrentState.store(EMqttifySocketState::Connected, std::memory_order_release);
			DisconnectTime = FDateTime::MaxValue();
		}
		OnConnectDelegate.Broadcast(true);
	}

	void FMqttifyWebSocket::HandleWebSocketConnectionError(const FString& Error)
	{
		{
			FScopeLock Lock{&SocketAccessLock};
			LOG_MQTTIFY(
				Error,
				TEXT("Socket Connection %s, ClientId %s. Error: %s"),
				*ConnectionSettings->ToString(),
				*ConnectionSettings->GetClientId(),
				*Error);
			FinalizeDisconnect();
		}
		OnConnectDelegate.Broadcast(false);
	}

	void FMqttifyWebSocket::FinalizeDisconnect()
	{
		{
			FScopeLock Lock{&SocketAccessLock};
			LOG_MQTTIFY(
				VeryVerbose,
				TEXT("Finalizing Disconnect Connection %s, ClientId %s"),
				*ConnectionSettings->ToString(),
				*ConnectionSettings->GetClientId());
			CurrentState.store(EMqttifySocketState::Disconnected, std::memory_order_release);
			DisconnectTime = FDateTime::MaxValue();
		}
		OnDisconnectDelegate.Broadcast();
	}

	void FMqttifyWebSocket::HandleWebSocketConnectionClosed(
		const int32 Status,
		const FString& Reason,
		const bool bWasClean
		)
	{
		FScopeLock Lock{&SocketAccessLock};

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

	void FMqttifyWebSocket::HandleWebSocketData(const void* Data, const SIZE_T Length, const SIZE_T BytesRemaining)
	{
		LOG_MQTTIFY(
			VeryVerbose,
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
		ReadPacketsFromBuffer();
	}
} // namespace Mqttify
