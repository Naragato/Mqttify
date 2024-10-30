#include "MqttifySocket.h"

#include "LogMqttify.h"
#include "MqttifySocketState.h"

namespace Mqttify
{
	template <typename TSocketType>
	TMqttifySocket<TSocketType>::TMqttifySocket(const FMqttifyConnectionSettingsRef& InConnectionSettings)
		: ConnectionSettings{InConnectionSettings}
		, Resolver{IoContext}
		, RemainingLength{0}
		, CurrentState{EMqttifySocketState::Disconnected}
	{
		DataBuffer.Reserve(ConnectionSettings->GetMaxPacketSize() + ReadBuffer.GetAllocatedSize());
	}

	template <typename TSocketType>
	void TMqttifySocket<TSocketType>::Connect()
	{
		FScopeLock Lock{&SocketAccessLock};
		if (CurrentState != EMqttifySocketState::Disconnected)
		{
			LOG_MQTTIFY(Error, TEXT("Socket is already connected"));
			return;
		}

		CurrentState = EMqttifySocketState::Connecting;

		if constexpr (std::is_same_v<TSocketType, FSslSocket>)
		{
			LOG_MQTTIFY(VeryVerbose, TEXT("Creating SSL context"));
			this->SslContext->set_default_verify_paths();
			if (ConnectionSettings->ShouldVerifyServerCertificate())
			{
				this->SslContext->set_verify_mode(boost::asio::ssl::verify_peer);
				this->SslContext->set_default_verify_paths();
			}
			else
			{
				this->SslContext->set_verify_mode(boost::asio::ssl::verify_none);
			}

			Socket = MakeShared<TSocketType>(IoContext, *this->SslContext);
		}
		else
		{
			LOG_MQTTIFY(VeryVerbose, TEXT("Creating TCP socket"));
			Socket = MakeShared<TSocketType>(IoContext);
		}

		if (!Socket.IsValid())
		{
			LOG_MQTTIFY(Error, TEXT("Socket is invalid"));
			return;
		}

		const FString Host = ConnectionSettings->GetHost();
		const FString Port = FString::FromInt(ConnectionSettings->GetPort());

		if (Host.IsEmpty() || Port.IsEmpty())
		{
			LOG_MQTTIFY(Error, TEXT("Host or Port is empty. Host: '%s', Port: '%s'"), *Host, *Port);
			Disconnect();
			return;
		}

		LOG_MQTTIFY(VeryVerbose, TEXT("Initiating async_resolve for %s:%s"), *Host, *Port);
		TWeakPtr<TMqttifySocket> ThisWeakPtr = this->AsWeak();
		Resolver.async_resolve(
			boost::asio::string_view{StringCast<ANSICHAR>(*Host).Get()},
			boost::asio::string_view{StringCast<ANSICHAR>(*Port).Get()},
			[ThisWeakPtr](
			const boost::system::error_code& Ec,
			const boost::asio::ip::tcp::resolver::results_type& Endpoints
			)
			{
				LOG_MQTTIFY(VeryVerbose, TEXT("HandleResolve"));
				if (const TSharedPtr<TMqttifySocket> ReadSocketPtr = ThisWeakPtr.Pin())
				{
					ReadSocketPtr->HandleResolve(Ec, Endpoints);
				}
			});
	}

	template <typename TSocketType>
	void TMqttifySocket<TSocketType>::HandleResolve(
		const boost::system::error_code& InError,
		const boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp>& InEndpoints
		)
	{
		if (InError)
		{
			LOG_MQTTIFY(Error, TEXT("Error resolving host: %s"), StringCast<TCHAR>(InError.message().c_str()).Get());
			Disconnect();
			return;
		}

		if (InEndpoints.empty())
		{
			LOG_MQTTIFY(Error, TEXT("No endpoints found"));
			Disconnect();
			return;
		}

		LOG_MQTTIFY(
			Verbose,
			TEXT("Resolved endpoints: %s, number of endpoints: %d"),
			StringCast<TCHAR>(InEndpoints->endpoint().address().to_string().c_str()).Get(),
			InEndpoints.size());

		TWeakPtr<TMqttifySocket> ThisWeakPtr = this->AsWeak();

		Socket->lowest_layer().async_connect(
			InEndpoints->endpoint(),
			[ThisWeakPtr](const boost::system::error_code& Ec)
			{
				if (const TSharedPtr<TMqttifySocket> ReadSocketPtr = ThisWeakPtr.Pin())
				{
					ReadSocketPtr->OnAsyncConnect(Ec);
				}
			});
	}

	template <typename TSocketType>
	void TMqttifySocket<TSocketType>::OnAsyncConnect(const boost::system::error_code& InError)
	{
		if (InError)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("Error connecting to %s during socket connection. message: %s, code: %d, category: %s"),
				*ConnectionSettings->ToString(),
				StringCast<TCHAR>(InError.message().c_str()).Get(),
				InError.value(),
				StringCast<TCHAR>(InError.category().name()).Get(),
				StringCast<TCHAR>(InError.what().c_str()).Get());
			Disconnect();
			return;
		}

		LOG_MQTTIFY(VeryVerbose, TEXT("Socket connected, starting handshake if necessary"));

		if constexpr (std::is_same_v<TSocketType, FSslSocket>)
		{
			TWeakPtr<TMqttifySocket> ThisWeakPtr = this->AsWeak();
			Socket->async_handshake(
				boost::asio::ssl::stream_base::client,
				[ThisWeakPtr](const boost::system::error_code& HandshakeEc)
				{
					LOG_MQTTIFY(VeryVerbose, TEXT("HandleHandshake"));
					if (const TSharedPtr<TMqttifySocket> ReadSocketPtr = ThisWeakPtr.Pin())
					{
						ReadSocketPtr->OnSocketConnected(HandshakeEc);
					}
				});
		}
		else
		{
			OnSocketConnected(InError);
		}
	}

	template <typename TSocketType>
	void TMqttifySocket<TSocketType>::OnSocketConnected(const boost::system::error_code& InError)
	{
		if (!InError)
		{
			LOG_MQTTIFY(
				Display,
				TEXT("Socket connected to %s"),
				StringCast<TCHAR>(Socket->lowest_layer().remote_endpoint().address().to_string().c_str()).Get());
			CurrentState = EMqttifySocketState::Connected;
			OnConnectDelegate.Broadcast(true);
			boost::asio::ip::tcp::socket X{IoContext};
			StartAsyncRead();
		}
		else
		{
			LOG_MQTTIFY(
				Error,
				TEXT("Error during SSL handshake: %s"),
				StringCast<TCHAR>(InError.message().c_str()).Get());
			Disconnect();
		}
	}

	template <typename TSocketType>
	void TMqttifySocket<TSocketType>::StartAsyncRead()
	{
		if (CurrentState != EMqttifySocketState::Connected)
		{
			return;
		}

		LOG_MQTTIFY(VeryVerbose, TEXT("Starting async read"));

		TWeakPtr<TMqttifySocket> ThisWeakPtr = this->AsWeak();
		Socket->async_read_some(
			boost::asio::buffer(ReadBuffer.GetData(), ReadBuffer.Num()),
			[ThisWeakPtr](const boost::system::error_code& Ec, std::size_t BytesTransferred)
			{
				if (const TSharedPtr<TMqttifySocket> ReadSocketPtr = ThisWeakPtr.Pin())
				{
					ReadSocketPtr->HandleRead(Ec, BytesTransferred);
				}
			});
	}

	template <typename TSocketType>
	void TMqttifySocket<TSocketType>::HandleRead(
		const boost::system::error_code& InError,
		std::size_t InBytesTransferred
		)
	{
		if (!InError)
		{
			// Append the received data to the data buffer
			DataBuffer.Append(ReadBuffer.GetData(), InBytesTransferred);

			// Process received data to extract complete packets
			ParsePacket();

			// Continue reading from the socket
			StartAsyncRead();
		}
		else
		{
			LOG_MQTTIFY(
				Error,
				TEXT("Error reading from socket: %s"),
				StringCast<TCHAR>(InError.message().c_str()).Get());
			Disconnect();
		}
	}

	template <typename TSocketType>
	void TMqttifySocket<TSocketType>::ParsePacket()
	{
		// We need at least 2 bytes to start parsing (Fixed header)
		if (DataBuffer.Num() < 2)
		{
			return;
		}

		constexpr int32 PacketStartIndex = 0;

		// Remaining Length parsing as per MQTT protocol
		RemainingLength = 0;
		uint32 Multiplier = 1;
		int32 Index = 1;
		bool bHaveRemainingLength = false;

		for (; Index < 5; ++Index)
		{
			if (Index >= DataBuffer.Num())
			{
				// We don't have enough data yet
				return;
			}

			const uint8 EncodedByte = DataBuffer[Index];
			RemainingLength += (EncodedByte & 127) * Multiplier;
			Multiplier *= 128;

			// Check if the MSB is 0, indicating the end of the Remaining Length field
			if ((EncodedByte & 128) == 0 || Index == 4)
			{
				bHaveRemainingLength = true;
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

		const int32 FixedHeaderSize = Index + 1; // Index starts from 1
		const int32 TotalPacketSize = FixedHeaderSize + RemainingLength;

		if (DataBuffer.Num() < TotalPacketSize)
		{
			// We don't have the full packet yet
			return;
		}

		const TSharedPtr<FArrayReader> Packet = MakeShared<FArrayReader>(false);
		Packet->Append(&DataBuffer[PacketStartIndex], TotalPacketSize);
		DataBuffer.RemoveAt(0, TotalPacketSize);
		RemainingLength = 0;
		if (GetOnDataReceivedDelegate().IsBound())
		{
			GetOnDataReceivedDelegate().Broadcast(Packet);
		}

		LOG_MQTTIFY(VeryVerbose, TEXT("Packet received of size %d"), TotalPacketSize);
	}

	template <typename TSocketType>
	void TMqttifySocket<TSocketType>::Disconnect()
	{
		Disconnect_Internal();
	}

	template <typename TSocketType>
	void TMqttifySocket<TSocketType>::Send(const uint8* InData, uint32 InSize)
	{
		FScopeLock Lock{&SocketAccessLock};

		if (!Socket.IsValid())
		{
			LOG_MQTTIFY(Error, TEXT("Socket is null"));
			return;
		}

		if (!IsConnected())
		{
			LOG_MQTTIFY(Error, TEXT("Socket is not connected"));
			Disconnect();
			return;
		}

		boost::system::error_code Ec;
		Socket->write_some(boost::asio::buffer(InData, InSize), Ec);
		if (Ec)
		{
			LOG_MQTTIFY(Error, TEXT("Error writing to socket: %s"), StringCast<TCHAR>(Ec.message().c_str()).Get());
			Disconnect();
		}
	}

	template <typename TSocketType>
	void TMqttifySocket<TSocketType>::Tick()
	{
		FScopeLock Lock{&SocketAccessLock};
		if (CurrentState == EMqttifySocketState::Disconnected)
		{
			return;
		}

		const size_t Executed = IoContext.poll();
		if (Executed > 0)
		{
			LOG_MQTTIFY(VeryVerbose, TEXT("Executed %d handlers"), Executed);
		}
	}

	template <typename TSocketType>
	bool TMqttifySocket<TSocketType>::IsConnected() const
	{
		FScopeLock Lock{&SocketAccessLock};
		return Socket->lowest_layer().is_open() && CurrentState == EMqttifySocketState::Connected;
	}

	template <typename TSocketType>
	void TMqttifySocket<TSocketType>::Disconnect_Internal()
	{
		FScopeLock Lock{&SocketAccessLock};

		if (CurrentState == EMqttifySocketState::Connecting)
		{
			OnConnectDelegate.Broadcast(false);
		}

		if (Socket.IsValid())
		{
			boost::system::error_code Ec;
			Socket->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, Ec);
			if (Ec)
			{
				LOG_MQTTIFY(
					Warning,
					TEXT("Error disconnecting from socket: %s"),
					StringCast<TCHAR>(Ec.message().c_str()).Get());
			}

			Socket->lowest_layer().close(Ec);
			if (Ec)
			{
				LOG_MQTTIFY(
					Warning,
					TEXT("Error disconnecting from socket: %s"),
					StringCast<TCHAR>(Ec.message().c_str()).Get());
			}
		}

		CurrentState = EMqttifySocketState::Disconnected;
		DataBuffer.Empty();
		RemainingLength = 0;
		OnDisconnectDelegate.Broadcast();
	}

	template class TMqttifySocket<FTcpSocket>;
	template class TMqttifySocket<FSslSocket>;
} // namespace Mqttify
