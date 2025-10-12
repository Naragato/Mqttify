#include "Socket/MqttifySecureSocket.h"

#include "LogMqttify.h"
#include "MqttifySocketState.h"
#include "Sockets.h"
#include "SslModule.h"
#include "Interfaces/ISslCertificateManager.h"

#if WITH_SSL
#define UI UI_ST
#include <openssl/x509v3.h>
#include <openssl/ssl.h>
#undef UI
#endif // WITH_SSL

namespace Mqttify
{
	FMqttifySecureSocket::FMqttifySecureSocket(const FMqttifyConnectionSettingsRef& InConnectionSettings)
		: FMqttifySocketBase{InConnectionSettings}
		, Socket{nullptr}
		, CurrentState{EMqttifySocketState::Disconnected}
		, bUseSSL{InConnectionSettings->GetTransportProtocol() == EMqttifyConnectionProtocol::Mqtts}
#if WITH_SSL
		, SslCtx{nullptr}
		, Ssl{nullptr}
		, Bio{nullptr}
#endif // WITH_SSL
	{}

	FMqttifySecureSocket::~FMqttifySecureSocket()
	{
		Disconnect_Internal();
	}

	void FMqttifySecureSocket::Connect()
	{
		TSharedRef<FMqttifySecureSocket> Self = AsShared();
		EMqttifySocketState Expected          = EMqttifySocketState::Disconnected;
		if (!CurrentState.compare_exchange_strong(
			Expected,
			EMqttifySocketState::Connecting,
			std::memory_order_acq_rel,
			std::memory_order_acquire))
		{
			LOG_MQTTIFY(Warning, TEXT("Socket is already connecting"));
			return;
		}

		{
			FScopeLock Lock{&SocketAccessLock};
			InitializeSocket();
			if (Socket.IsValid())
			{
				ISocketSubsystem* SocketSubsystem    = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
				FAddressInfoResult AddressInfoResult = SocketSubsystem->GetAddressInfo(
					*ConnectionSettings->GetHost(),
					nullptr,
					EAddressInfoFlags::Default,
					NAME_None,
					SOCKTYPE_Streaming);

				if (AddressInfoResult.ReturnCode != SE_NO_ERROR)
				{
					LOG_MQTTIFY(
						Error,
						TEXT("Invalid address. AddressInfo return code: %d."),
						AddressInfoResult.ReturnCode);
				}
				else if (AddressInfoResult.Results.Num() <= 0)
				{
					LOG_MQTTIFY(Error, TEXT("No results for provided address: %s."), *ConnectionSettings->ToString());
				}
				else
				{
					const TSharedRef<FInternetAddr> Addr = AddressInfoResult.Results[0].Address;

					if (!Addr->IsValid())
					{
						LOG_MQTTIFY(Error, TEXT("Invalid Address %s."), *ConnectionSettings->ToString());
					}
					else
					{
						Addr->SetPort(ConnectionSettings->GetPort());
						if (Socket->Connect(*Addr))
						{
#if WITH_SSL
							if (bUseSSL)
							{
								if (InitializeSSL())
								{
									CurrentState.store(EMqttifySocketState::SslConnecting, std::memory_order_release);
								}
							}
							else
#endif // WITH_SSL
							{
								CurrentState.store(EMqttifySocketState::Connected, std::memory_order_release);
							}
						}
					}
				}
			}
		}

		if (CurrentState.load(std::memory_order_acquire) == EMqttifySocketState::Connected)
		{
			LOG_MQTTIFY(
				Display,
				TEXT("Connected to socket %s, ClientId %s"),
				*ConnectionSettings->ToString(),
				*ConnectionSettings->GetClientId());
			OnConnectDelegate.Broadcast(true);
		}
		else if (CurrentState.load(std::memory_order_acquire) != EMqttifySocketState::SslConnecting)
		{
			LOG_MQTTIFY(Error, TEXT("Failed to connect to %s"), *ConnectionSettings->ToString());
			OnConnectDelegate.Broadcast(false);
			Expected = EMqttifySocketState::Connecting;
			if (CurrentState.compare_exchange_strong(Expected, EMqttifySocketState::Disconnected))
			{
				Disconnect_Internal();
			}
		}
	}

	void FMqttifySecureSocket::Disconnect()
	{
		while (CurrentState.load(std::memory_order_acquire) == EMqttifySocketState::Connecting)
		{
			FPlatformProcess::YieldThread();
		}

		// Allow disconnecting from either Connected or SslConnecting states
		EMqttifySocketState Expected = CurrentState.load(std::memory_order_acquire);
		while (Expected == EMqttifySocketState::Connected || Expected == EMqttifySocketState::SslConnecting)
		{
			if (CurrentState.compare_exchange_weak(Expected, EMqttifySocketState::Disconnected))
			{
				Disconnect_Internal();
				OnDisconnectDelegate.Broadcast();
				break;
			}
		}
	}

	void FMqttifySecureSocket::Close(int32 Code, const FString& Reason)
	{
		unimplemented();
	}

	void FMqttifySecureSocket::Send(const uint8* InData, uint32 InSize)
	{
		LOG_MQTTIFY_PACKET_DATA(VeryVerbose,
		                        InData,
		                        InSize,
		                        TEXT("Sending data to socket %s, ClientId %s"),
		                        *ConnectionSettings->ToString(),
		                        *ConnectionSettings->GetClientId());

		{
			FScopeLock Lock{&SocketAccessLock};
			if (!IsConnected())
			{
				LOG_MQTTIFY(Warning, TEXT("Socket is not connected"));
				return;
			}
#if WITH_SSL
			if (bUseSSL)
			{
				const int32 Ret = SSL_write(Ssl, InData, InSize);;
				if (Ret < 0)
				{
					const int32 ErrCode = SSL_get_error(Ssl, Ret);
					if (ErrCode != SSL_ERROR_WANT_READ && ErrCode != SSL_ERROR_WANT_WRITE)
					{

						LOG_MQTTIFY(Error,
						            TEXT("SSL_write failed: %s, %d"),
						            *GetLastSslErrorString(true),
						            Ret);
						Disconnect();
					}
				}
			}
			else
#endif
			{
				SIZE_T TotalBytesSent = 0;
				while (TotalBytesSent < InSize)
				{

					int32 BytesSent = 0;
					if (!Socket->Send(InData + TotalBytesSent, InSize - TotalBytesSent, BytesSent) || BytesSent == 0)
					{
						Disconnect();
						return;
					}
					TotalBytesSent += BytesSent;
				}
			}
		}
	}

	void FMqttifySecureSocket::Tick()
	{
		bool bShouldDisconnect = false;

		{
			FScopeLock Lock{&SocketAccessLock};

			const EMqttifySocketState State = CurrentState.load(std::memory_order_acquire);
			if (State == EMqttifySocketState::Disconnected)
			{
				return;
			}

#if WITH_SSL
			if (bUseSSL)
			{
#if !UE_BUILD_SHIPPING
				DebugSslBioState(State);
#endif

				if (State == EMqttifySocketState::SslConnecting)
				{
					bShouldDisconnect = !PerformSSLHandshake();
				}

				else if (IsConnected())
				{
					bShouldDisconnect |= !ReadAvailableData(
						[this](const int32 Want, TArray<uint8>& Tmp, size_t& BytesRead) {
							return ReceiveFromSSL(Want, Tmp, BytesRead);
						});
				}
			}
			else
#endif // WITH_SSL
				if (IsConnected())
				{
					bShouldDisconnect = !ReadAvailableData(
						[this](const int32 Want, TArray<uint8>& Tmp, size_t& BytesRead) {
							return ReceiveFromSocket(Want, Tmp, BytesRead);
						});
				}
		}

		if (bShouldDisconnect)
		{
			Disconnect();
		}
	}

	bool FMqttifySecureSocket::IsConnected() const
	{
		FScopeLock Lock{&SocketAccessLock};
		return (CurrentState.load(std::memory_order_acquire) == EMqttifySocketState::Connected)
			&& Socket.IsValid()
			&& Socket->GetConnectionState() == ESocketConnectionState::SCS_Connected;
	}

	void FMqttifySecureSocket::Send(const TSharedRef<IMqttifyControlPacket>& InPacket)
	{
		TArray<uint8> ActualBytes;
		FMemoryWriter Writer(ActualBytes);
		InPacket->Encode(Writer);
		Send(ActualBytes.GetData(), ActualBytes.Num());
	}

	void FMqttifySecureSocket::Disconnect_Internal()
	{
		FScopeLock Lock{&SocketAccessLock};
		LOG_MQTTIFY(Display, TEXT("Disconnect"));
#if WITH_SSL
		if (bUseSSL)
		{
			CleanupSSL();
		}
#endif // WITH_SSL
		if (Socket.IsValid())
		{
			Socket->Close();
			Socket.Reset();
		}
	}

	void FMqttifySecureSocket::InitializeSocket()
	{
		ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
		Socket = SocketSubsystem->CreateUniqueSocket(NAME_Stream, TEXT("MQTT Connection"), false);

		if (!Socket.IsValid())
		{
			LOG_MQTTIFY(Error, TEXT("Failed to create socket"));
			return;
		}

		const bool bSuccess = Socket->SetReuseAddr(true) &&
			Socket->SetNonBlocking(true) &&
			Socket->SetNoDelay(true) &&
			Socket->SetLinger(false, 0) &&
			Socket->SetRecvErr();

		if (!bSuccess)
		{
			LOG_MQTTIFY(Error, TEXT("Failed to set socket"));
			Socket->Close();
			Socket.Reset();
			return;
		}

		int32 BufferSize = 0;
		Socket->SetReceiveBufferSize(kBufferSize, BufferSize);
		LOG_MQTTIFY(VeryVerbose, TEXT("Socket receive buffer size: %d."), BufferSize);
		Socket->SetSendBufferSize(kBufferSize, BufferSize);
		LOG_MQTTIFY(VeryVerbose, TEXT("Socket send buffer size: %d."), BufferSize);
	}

	bool FMqttifySecureSocket::IsSocketReadyForWrite() const
	{
		if (Socket->GetConnectionState() == ESocketConnectionState::SCS_ConnectionError)
		{
			LOG_MQTTIFY(Verbose, TEXT("Socket connection error detected"));
			return false;
		}

		int32 BytesRead = 0;
		uint8 Dummy     = 0;
		if (!Socket->Recv(&Dummy, 1, BytesRead, ESocketReceiveFlags::Peek))
		{
			LOG_MQTTIFY(Verbose, TEXT("Socket is not ready for writing"));
			return false;
		}
		return true;
	}

	bool FMqttifySecureSocket::ReceiveFromSocket(const int32 Want, TArray<uint8>& Tmp, size_t& OutBytesRead) const
	{
		OutBytesRead    = 0;
		int32 BytesRead = 0;
		if (!Socket->Recv(Tmp.GetData(), Want, BytesRead))
		{
			LOG_MQTTIFY(Error, TEXT("Socket Recv failed"));
			return false;
		}

		OutBytesRead = BytesRead;
		// Treat zero bytes as a clean “nothing available” (not a hard error).
		return true;
	}

#if WITH_SSL
#if !UE_BUILD_SHIPPING
	void FMqttifySecureSocket::DebugSslBioState(const EMqttifySocketState State)
	{
		FString Msg;
		Msg.Reserve(256);
		Msg += FString::Printf(TEXT("Tick: bUseSSL=1, MqttifyState=%s, SocketConnState=%d"),
		                       EnumToTCharString(State),
		                       Socket.IsValid()
		                       ? static_cast<int32>(Socket->GetConnectionState())
		                       : static_cast<int32>(ESocketConnectionState::SCS_ConnectionError));

		if (Ssl != nullptr)
		{
			const char* SslStateAnsi = SSL_state_string_long(Ssl);
			const char* VersionAnsi  = SSL_get_version(Ssl);
			const bool bInitFinished = SSL_is_init_finished(Ssl) == 1;
			const int PendingApp     = SSL_pending(Ssl);
			Msg += TEXT(" | ");
			Msg += FString::Printf(TEXT("SSL: state=%s, version=%s, init_finished=%s, pending_app=%d"),
			                       ANSI_TO_TCHAR(SslStateAnsi),
			                       ANSI_TO_TCHAR(VersionAnsi),
			                       bInitFinished ? TEXT("true") : TEXT("false"),
			                       PendingApp);
		}

		if (Bio != nullptr)
		{
			const size_t Pending   = BIO_ctrl_pending(Bio);
			const size_t WPending  = BIO_ctrl_wpending(Bio);
			const bool ShouldRead  = !!BIO_should_read(Bio);
			const bool ShouldWrite = !!BIO_should_write(Bio);
			const bool ShouldRetry = !!BIO_should_retry(Bio);
			Msg += TEXT(" | ");
			Msg += FString::Printf(
				TEXT("BIO: pending=%llu, wpending=%llu, should_read=%d, should_write=%d, should_retry=%d"),
				Pending,
				WPending,
				ShouldRead ? 1 : 0,
				ShouldWrite ? 1 : 0,
				ShouldRetry ? 1 : 0);
		}

		if (Msg != LastSslBioDebugLog)
		{
			LastSslBioDebugLog = Msg;
			LOG_MQTTIFY(Verbose, TEXT("%s"), *Msg);
		}
	}
#endif // !UE_BUILD_SHIPPING

	bool FMqttifySecureSocket::InitializeSSL()
	{

		OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, nullptr);
		SslCtx = SSL_CTX_new(TLS_client_method());
		if (nullptr == SslCtx)
		{
			LOG_MQTTIFY(Error, TEXT("SSL_CTX_new failed: %s"), *GetLastSslErrorString(true));
			return false;
		}

		SSL_CTX_set_min_proto_version(SslCtx, TLS1_2_VERSION);

		if (ConnectionSettings->ShouldVerifyServerCertificate())
		{
			SSL_CTX_set_verify(SslCtx, SSL_VERIFY_PEER, SslCertVerify);
		}
		else
		{
			SSL_CTX_set_verify(SslCtx, SSL_VERIFY_NONE, nullptr);
		}

		FSslModule::Get().GetCertificateManager().AddCertificatesToSslContext(SslCtx);

		Ssl = SSL_new(SslCtx);

		if (nullptr == Ssl)
		{
			SSL_CTX_free(SslCtx);
			SslCtx = nullptr;
			LOG_MQTTIFY(Error, TEXT("Failed to create SSL object %s"), *GetLastSslErrorString(true));
			return false;
		}

		SSL_set_min_proto_version(Ssl, TLS1_2_VERSION);
		SSL_set_hostflags(Ssl, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
		SSL_set1_host(Ssl, TCHAR_TO_ANSI(*ConnectionSettings->GetHost()));

		const auto CipherList =
			"HIGH:"
			"!aNULL:"
			"!eNULL:"
			"!EXPORT:"
			"!DES:"
			"!RC4:"
			"!MD5:"
			"!PSK:"
			"!SRP:"
			"!CAMELLIA:"
			"!SEED:"
			"!IDEA:"
			"!3DES";

		SSL_CTX_set_cipher_list(SslCtx, CipherList);
		SSL_set_cipher_list(Ssl, CipherList);

		// Set cipher for TLS 1.3
		const auto CipherSuites =
			"TLS_AES_256_GCM_SHA384:"
			"TLS_CHACHA20_POLY1305_SHA256:"
			"TLS_AES_128_GCM_SHA256";

		SSL_CTX_set_ciphersuites(SslCtx, CipherSuites);
		SSL_set_ciphersuites(Ssl, CipherSuites);

		// Restrict the curves to approved ones
		SSL_CTX_set1_curves_list(SslCtx, "P-521:P-384:P-256");
		SSL_set1_curves_list(Ssl, "P-521:P-384:P-256");
		SSL_CTX_set_options(SslCtx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
		Bio = BIO_new(GetSocketBioMethod());

		if (nullptr == Bio)
		{
			LOG_MQTTIFY(Error, TEXT("BIO_new Failed to create custom BIOs"));
			CleanupSSL();
			return false;
		}

		BIO_set_data(Bio, this);
		SSL_set_bio(Ssl, Bio, Bio);
		// SSL_set_mode(Ssl, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER | SSL_MODE_AUTO_RETRY);
		SSL_set_connect_state(Ssl);
		SSL_set_app_data(Ssl, this);
		return true;
	}

	void FMqttifySecureSocket::CleanupSSL()
	{
		if (nullptr != Ssl)
		{
			SSL_shutdown(Ssl);
			SSL_free(Ssl);
			Ssl = nullptr;
		}
		if (nullptr != Bio)
		{
			Bio = nullptr;
		}
		if (nullptr != SslCtx)
		{
			SSL_CTX_free(SslCtx);
			SslCtx = nullptr;
		}
	}

	bool FMqttifySecureSocket::PerformSSLHandshake()
	{
		LOG_MQTTIFY(Verbose, TEXT("Performing SSL handshake"));

		const int Ret = SSL_connect(Ssl);
		switch (const int SslError = SSL_get_error(Ssl, Ret))
		{
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
				// Not an error; no bytes now. Let Tick continue later.
				return true;
			case SSL_ERROR_NONE:
				if (ConnectionSettings->ShouldVerifyServerCertificate())
				{
					if (const long VerifyResult = SSL_get_verify_result(Ssl); VerifyResult != X509_V_OK)
					{
						LOG_MQTTIFY(
							Error,
							TEXT("SSL certificate verification failed: %hs"),
							X509_verify_cert_error_string(VerifyResult));
						return false;
					}
				}
				CurrentState.store(EMqttifySocketState::Connected, std::memory_order_release);
				LOG_MQTTIFY(Display, TEXT("TLS handshake completed %d"), Ret);
				OnConnectDelegate.Broadcast(true);
				return true;
			case SSL_ERROR_ZERO_RETURN:
				LOG_MQTTIFY(Error, TEXT("SSL_connect returned SSL_ERROR_ZERO_RETURN"));
				return false;
			case SSL_ERROR_SYSCALL:
				LOG_MQTTIFY(Error, TEXT("SSL_connect returned SSL_ERROR_SYSCALL"));
				return false;
			default:
				if (ERR_peek_last_error() == SSL_R_NO_SHARED_CIPHER)
				{
					LOG_MQTTIFY(
						Error,
						TEXT("SSL_connect returned %s Incompatible cipher suit"),
						*GetLastSslErrorString(true));
					return false;
				}

				LOG_MQTTIFY(Error, TEXT("SSL_connect returned %d %d"), SslError, ERR_peek_last_error());
				return false;
		}
	}

	bool FMqttifySecureSocket::ReceiveFromSSL(const int32 Want, TArray<uint8>& Tmp, size_t& BytesRead) const
	{
		const int32 Ret = SSL_read_ex(Ssl, Tmp.GetData(), Want, &BytesRead);

		LOG_MQTTIFY(VeryVerbose, TEXT("SSL_read returned %d %lld"), Ret, BytesRead);
		if (BytesRead > 0 && Ret == 1)
		{
			return true;
		}

		// Interpret SSL error conditions: only disconnect on hard errors.
		switch (const int Err = SSL_get_error(Ssl, Ret))
		{
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
				return true;
			default:
				LOG_MQTTIFY(Error, TEXT("SSL_read failed, error=%d, %llu, %d"), Err, BytesRead, Ret);
				return false;
		}
	}

	FString FMqttifySecureSocket::GetLastSslErrorString(bool bConsume) noexcept
	{
		// OpenSSL per-thread error queue: 0 means "no error".
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
		const unsigned long Err = bConsume ? ERR_get_error() : ERR_peek_last_error();
#else
		// On very old OpenSSL, fallback to popping (closest to original behavior).
		const unsigned long Err = ERR_get_error();
#endif

		if (Err == 0)
		{
			return TEXT("No OpenSSL error (queue empty)");
		}

		char Buf[256] = {};
		ERR_error_string_n(Err, Buf, sizeof(Buf));

		const char* LibStr    = ERR_lib_error_string(Err);
		const char* ReasonStr = ERR_reason_error_string(Err);

		if (LibStr || ReasonStr)
		{
			return FString::Printf(
				TEXT("%s | lib=%s | reason=%s"),
				UTF8_TO_TCHAR(Buf),
				LibStr ? UTF8_TO_TCHAR(LibStr) : TEXT("?"),
				ReasonStr ? UTF8_TO_TCHAR(ReasonStr) : TEXT("?"));
		}

		return UTF8_TO_TCHAR(Buf);
	}

	int32 FMqttifySecureSocket::SslCertVerify(int32 PreverifyOk, X509_STORE_CTX* Context)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("SslCertVerify: PreverifyOk %d"), PreverifyOk);
		const SSL* Handle = static_cast<SSL*>(X509_STORE_CTX_get_ex_data(
			Context,
			SSL_get_ex_data_X509_STORE_CTX_idx()));
		check(Handle);
		const FMqttifySecureSocket* Socket = static_cast<FMqttifySecureSocket*>(SSL_get_app_data(Handle));
		check(Socket);

		if (!Socket->ConnectionSettings->ShouldVerifyServerCertificate())
		{
			return 1;
		}

		if (PreverifyOk == 1)
		{
			if (!FSslModule::Get().GetCertificateManager().VerifySslCertificates(
				Context,
				Socket->ConnectionSettings->GetHost()))
			{
				LOG_MQTTIFY(Verbose, TEXT("Certificate verification failed"));
				PreverifyOk = 0;
			}
		}
		return PreverifyOk;
	}


	void FMqttifySecureSocket::AppendAndProcess(const uint8* Data, int32 Len)
	{
		DataBuffer.Append(Data, Len);
		ReadPacketsFromBuffer();
	}

	BIO_METHOD* FMqttifySecureSocket::GetSocketBioMethod()
	{
		static BIO_METHOD* Method = nullptr;
		if (nullptr == Method)
		{
			const int32 BioId = BIO_get_new_index() | BIO_TYPE_SOURCE_SINK;
			Method            = BIO_meth_new(BioId, "Mqttify Socket BIO");
			BIO_meth_set_write(Method, SocketBioWrite);
			BIO_meth_set_read(Method, SocketBioRead);
			BIO_meth_set_ctrl(Method, SocketBioCtrl);
			BIO_meth_set_create(Method, SocketBioCreate);
			BIO_meth_set_destroy(Method, SocketBioDestroy);
		}
		return Method;
	}

	int FMqttifySecureSocket::SocketBioWrite(BIO* Bio, const char* InBuffer, const int32 BufferSize)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("Writing: %d bytes"), BufferSize);
		BIO_clear_retry_flags(Bio);

		const FMqttifySecureSocket* Self = static_cast<const FMqttifySecureSocket*>(BIO_get_data(Bio));
		if (nullptr == Self || !Self->Socket.IsValid() || BufferSize <= 0)
		{
			return 0;
		}

		int32 BytesSent = 0;
		const bool bOk  = Self->Socket->Send(reinterpret_cast<const uint8*>(InBuffer), BufferSize, BytesSent);

		if (bOk && BytesSent > 0)
		{
			LOG_MQTTIFY(VeryVerbose, TEXT("SocketBioWrite: %d bytes"), BytesSent);
			return BytesSent;
		}

		// Map would-block to retry
		const ESocketErrors Err = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();
		if (Err == SE_EWOULDBLOCK)
		{
			BIO_set_retry_write(Bio);
			return -1;
		}

		// Hard error
		return -1;
	}

	int FMqttifySecureSocket::SocketBioRead(BIO* Bio, char* OutBuffer, const int32 BufferSize)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("Reading, buffer size: %d"), BufferSize);
		BIO_clear_retry_flags(Bio);

		if (nullptr == OutBuffer)
		{
			LOG_MQTTIFY(Error, TEXT("SocketBioRead: OutputBuffer is nullptr"));
			return -1;
		}

		const auto* Self = static_cast<const FMqttifySecureSocket*>(BIO_get_data(Bio));
		if (!Self || !Self->Socket.IsValid())
		{
			LOG_MQTTIFY(Error, TEXT("SocketBioRead: Self or Socket is nullptr"));
			return -1;
		}

		uint32 Size = BufferSize;
		if (!Self->Socket->HasPendingData(Size))
		{
			LOG_MQTTIFY(Verbose, TEXT("SocketBioRead: Socket has no pending data"));
			BIO_set_retry_read(Bio);
			return 0;
		}

		int32 BytesRead = 0;
		const bool bOk  = Self->Socket->Recv(reinterpret_cast<uint8*>(OutBuffer), BufferSize, BytesRead);
		if (bOk && BytesRead > 0)
		{
			LOG_MQTTIFY(VeryVerbose, TEXT("SocketBioRead: %d bytes read"), BytesRead);
			return BytesRead;
		}

		const ESocketErrors Err = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();

		if ((bOk && BytesRead == 0) || Err == SE_EWOULDBLOCK)
		{
			BIO_set_retry_read(Bio);
			LOG_MQTTIFY(Verbose, TEXT("SocketBioRead: Socket read returned 0 bytes"));
			return -1;
		}

		if (!bOk)
		{
			LOG_MQTTIFY(Error, TEXT("SocketBioRead: Socket read failed"));
			return 0;
		}

		LOG_MQTTIFY(Verbose, TEXT("Socket read failed for unknown reason, retrying"));
		return -1;
	}

	long FMqttifySecureSocket::SocketBioCtrl(BIO* Bio, int Cmd, long Num, void* Ptr)
	{
		switch (Cmd)
		{
			case BIO_CTRL_FLUSH:
				return 1;
			default:
				return 0;
		}
	}

	int FMqttifySecureSocket::SocketBioCreate(BIO* Bio)
	{
		BIO_set_shutdown(Bio, 0);
		BIO_set_init(Bio, 1);
		BIO_set_data(Bio, nullptr);
		return 1;
	}

	int FMqttifySecureSocket::SocketBioDestroy(BIO* Bio)
	{
		if (nullptr == Bio)
		{
			LOG_MQTTIFY(Verbose, TEXT("SocketBioDestroy: bio is nullptr"));
			return 0;
		}

		BIO_set_init(Bio, 0);
		BIO_set_data(Bio, nullptr);
		return 1;
	}

	bool FMqttifySecureSocket::ReadAvailableData(
		const TUniqueFunction<bool(const int32 Want, TArray<uint8>& Tmp, size_t& OutBytesRead)>&& Reader)
	{
		uint32 PendingData = 0;
		Socket->HasPendingData(PendingData);

#if WITH_SSL
		if (bUseSSL && nullptr != Ssl)
		{
			PendingData = FMath::Max<int32>(PendingData, SSL_pending(Ssl));
		}
#endif // WITH_SSL

		if (PendingData == 0)
		{
			return true;
		}

		const int32 Want = FMath::Min<int32>(kMaxChunkSize, static_cast<int32>(PendingData));

		TArray<uint8> Tmp;
		Tmp.SetNumUninitialized(Want);

		size_t BytesRead = 0;
		if (!Reader(Want, Tmp, BytesRead))
		{
			return false; // Reader indicated a hard failure.
		}

		if (BytesRead > 0)
		{
			AppendAndProcess(Tmp.GetData(), BytesRead);
		}

		return true;
	}
#endif // WITH_SSL
} // namespace Mqttify