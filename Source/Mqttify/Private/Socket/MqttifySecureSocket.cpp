#include "Socket/MqttifySecureSocket.h"

#include "LogMqttify.h"
#include "MqttifyConstants.h"
#include "MqttifySocketState.h"
#include "Sockets.h"
#include "SslModule.h"
#include "Async/Async.h"
#include "Interfaces/ISslCertificateManager.h"

#if WITH_SSL
#define UI UI_ST
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
		, WriteBio{nullptr}
		, ReadBio{nullptr}
#endif // WITH_SSL
	{
		ReadBuffer.SetNumZeroed(kBufferSize);
		LOG_MQTTIFY(
			VeryVerbose,
			TEXT("ReadBuffer %p of size %d"),
			ReadBuffer.GetData(),
			ReadBuffer.Max());
	}

	FMqttifySecureSocket::~FMqttifySecureSocket()
	{
		Disconnect_Internal();
	}

	void FMqttifySecureSocket::Connect()
	{
		TSharedRef<FMqttifySecureSocket> Self = AsShared();
		EMqttifySocketState Expected = EMqttifySocketState::Disconnected;
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
				ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
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

						const bool bConnected = Socket->Connect(*Addr);
						if (bConnected)
						{
#if WITH_SSL
							if (bUseSSL)
							{
								if (InitializeSSL() && PerformSSLHandshake())
								{
									CurrentState.store(EMqttifySocketState::Connected, std::memory_order_release);
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
		else
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

		EMqttifySocketState Expected = EMqttifySocketState::Connected;
		if (CurrentState.compare_exchange_strong(Expected, EMqttifySocketState::Disconnected))
		{
			Disconnect_Internal();
			OnDisconnectDelegate.Broadcast();
		}
	}

	void FMqttifySecureSocket::Send(const uint8* InData, uint32 InSize)
	{
		bool bShouldDisconnect = false;
		{
			FScopeLock Lock{&SocketAccessLock};
			if (!IsConnected())
			{
				LOG_MQTTIFY(Warning, TEXT("Socket is not connected"));
				return;
			}

			if (!IsSocketReadyForWrite())
			{
				LOG_MQTTIFY(Error, TEXT("Socket is not available for writing"));
				bShouldDisconnect = true;
			}
			else
			{
				LOG_MQTTIFY_PACKET_DATA(
					VeryVerbose,
					InData,
					InSize,
					TEXT("Sending data to socket %s, ClientId %s"),
					*ConnectionSettings->ToString(),
					*ConnectionSettings->GetClientId());

#if WITH_SSL
				if (bUseSSL)
				{
					const int Ret = SSL_write(Ssl, InData, InSize);
					if (Ret <= 0)
					{
						const int Error = SSL_get_error(Ssl, Ret);
						LOG_MQTTIFY(Error, TEXT("SSL_write failed with error %d"), Error);
						bShouldDisconnect = true;
					}
				}
				else
#endif // WITH_SSL
				{
					int32 BytesSent = 0;
					if (!Socket->Send(InData, InSize, BytesSent))
					{
						LOG_MQTTIFY(Error, TEXT("Socket send failed"));
						bShouldDisconnect = true;
					}
				}
			}
		}

		if (bShouldDisconnect)
		{
			Disconnect();
		}
	}

	void FMqttifySecureSocket::Tick()
	{
		bool bShouldDisconnect = false;
		{
			FScopeLock Lock{&SocketAccessLock};
			if (!IsConnected())
			{
				return;
			}

			int32 BytesReceived = 0;
#if WITH_SSL
			if (bUseSSL)
			{
				const int Ret = SSL_read(Ssl, ReadBuffer.GetData(), ReadBuffer.Max());
				if (Ret < 0)
				{
					const int Error = SSL_get_error(Ssl, Ret);
					if (Error != SSL_ERROR_WANT_READ && Error != SSL_ERROR_WANT_WRITE)
					{
						LOG_MQTTIFY(Error, TEXT("SSL_read failed with error %d"), Error);
						bShouldDisconnect = true;
					}
				}
				else
				{
					BytesReceived = Ret;
				}
			}
			else
#endif // WITH_SSL
			{
				uint32 Size = 0;
				if (!Socket->HasPendingData(Size))
				{
					return;
				}
				LOG_MQTTIFY(VeryVerbose, TEXT("Socket has pending data: %d"), Size);
				if (!Socket->Recv(ReadBuffer.GetData(), ReadBuffer.Max(), BytesReceived))
				{
					LOG_MQTTIFY(Error, TEXT("Socket receive failed"));
					bShouldDisconnect = true;
				}
			}
			if (BytesReceived > 0)
			{
				LOG_MQTTIFY(
					VeryVerbose,
					TEXT("Socket Data Received (%s , %s): Length %d"),
					*ConnectionSettings->ToString(),
					*ConnectionSettings->GetClientId(),
					BytesReceived);

				DataBuffer.Append(ReadBuffer.GetData(), BytesReceived);
				ReadPacketsFromBuffer();
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
		return CurrentState.load(std::memory_order_acquire) == EMqttifySocketState::Connected
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
		Socket->SetNonBlocking(true);
		Socket->SetNoDelay(false);
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
		uint8 Dummy = 0;
		if (!Socket->Recv(&Dummy, 1, BytesRead, ESocketReceiveFlags::Peek))
		{
			LOG_MQTTIFY(Verbose, TEXT("Socket is not ready for writing"));
			return false;
		}
		return true;
	}

#if WITH_SSL
	bool FMqttifySecureSocket::InitializeSSL()
	{
#if (OPENSSL_VERSION_NUMBER < 0x10100000L)
		SSL_library_init();
		SSL_load_error_strings();
		OpenSSL_add_all_algorithms();
#else
		OPENSSL_init_ssl(0, nullptr);
#endif // (OPENSSL_VERSION_NUMBER < 0x10100000L)

		SslCtx = SSL_CTX_new(TLS_client_method());
		if (!SslCtx)
		{
			LOG_MQTTIFY(Error, TEXT("Failed to create SSL context"));
			return false;
		}

		SSL_CTX_set_default_verify_paths(SslCtx);
		FSslModule::Get().GetCertificateManager().AddCertificatesToSslContext(SslCtx);
		if (ConnectionSettings->ShouldVerifyServerCertificate())
		{
			SSL_CTX_set_verify(SslCtx, SSL_VERIFY_PEER, SslCertVerify);
		}
		else
		{
			SSL_CTX_set_verify(SslCtx, SSL_VERIFY_NONE, SslCertVerify);
		}

		SSL_CTX_set_session_cache_mode(
			SslCtx,
			SSL_SESS_CACHE_CLIENT
			| SSL_SESS_CACHE_NO_INTERNAL_STORE);

		Ssl = SSL_new(SslCtx);
		if (!Ssl)
		{
			LOG_MQTTIFY(Error, TEXT("Failed to create SSL object"));
			return false;
		}

		SSL_CTX_set_min_proto_version(SslCtx, TLS1_2_VERSION);
		SSL_set_min_proto_version(Ssl, TLS1_2_VERSION);
		SSL_set_tlsext_host_name(Ssl, TCHAR_TO_ANSI(*ConnectionSettings->GetHost()));

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

		WriteBio = BIO_new(GetSocketBioMethod());
		ReadBio = BIO_new(GetSocketBioMethod());
		if (nullptr == WriteBio)
		{
			LOG_MQTTIFY(Error, TEXT("Failed to create custom write BIO"));
			return false;
		}

		if (nullptr == ReadBio)
		{
			LOG_MQTTIFY(Error, TEXT("Failed to create custom read BIO"));
			return false;
		}

		SSL_CTX_set_options(
			SslCtx,
			SSL_OP_CIPHER_SERVER_PREFERENCE |
			SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION |
			SSL_OP_NO_RENEGOTIATION);
		SSL_set_options(
			Ssl,
			SSL_OP_CIPHER_SERVER_PREFERENCE |
			SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION |
			SSL_OP_NO_RENEGOTIATION);

		BIO_set_data(WriteBio, this);
		BIO_set_data(ReadBio, this);
		BIO_set_init(WriteBio, 1);
		BIO_set_init(ReadBio, 1);
		SSL_set0_rbio(Ssl, ReadBio);
		SSL_set0_wbio(Ssl, WriteBio);
		BIO_up_ref(WriteBio);
		BIO_up_ref(ReadBio);
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
		if (nullptr != WriteBio)
		{
			WriteBio = nullptr;
		}
		if (nullptr != ReadBio)
		{
			ReadBio = nullptr;
		}
		if (nullptr != SslCtx)
		{
			SSL_CTX_free(SslCtx);
			SslCtx = nullptr;
		}
	}

	bool FMqttifySecureSocket::PerformSSLHandshake() const
	{
		FScopeLock Lock(&SocketAccessLock);
		LOG_MQTTIFY(Verbose, TEXT("Performing SSL handshake"));

		int SslError;
		do
		{
			const int Ret = SSL_do_handshake(Ssl);
			SslError = SSL_get_error(Ssl, Ret);
			LOG_MQTTIFY(Verbose, TEXT("SSL_do_handshake returned %d, %d"), SslError, Ret);
			if (SslError == SSL_ERROR_NONE)
			{
				break;
			}

			if (SslError != SSL_ERROR_WANT_READ && SslError != SSL_ERROR_WANT_WRITE && SslError != SSL_ERROR_SYSCALL)
			{
				break;
			}

			FPlatformProcess::YieldThread();
		}
		while (true);

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

		if (SslError != SSL_ERROR_NONE)
		{
			LOG_MQTTIFY(Error, TEXT("SSL handshake failed with error %d"), SslError);
			return false;
		}

		return true;
	}

	// bool FMqttifySecureSocket::PerformSSLHandshake()
	// {
	// 	FScopeLock Lock(&SocketAccessLock);
	// 	LOG_MQTTIFY(Verbose, TEXT("Performing SSL handshake"));
	//
	// 	while (true)
	// 	{
	// 		if (SSL_is_init_finished(Ssl))
	// 		{
	// 			LOG_MQTTIFY(Verbose, TEXT("SSL handshake is finished"));
	// 			break;
	// 		}
	//
	// 		const int RC = SSL_do_handshake(Ssl);
	// 		LOG_MQTTIFY(Verbose, TEXT("SSL_do_handshake returned %d"), RC);
	// 		if (RC > 0)
	// 		{
	// 			break;
	// 		}
	//
	// 		const int SSLError = SSL_get_error(Ssl, RC);
	// 		switch (SSLError)
	// 		{
	// 		case SSL_ERROR_WANT_READ:
	// 		case SSL_ERROR_WANT_WRITE:
	// 			// The handshake needs to read/write more data
	// 			Tick();
	// 			FPlatformProcess::YieldThread();
	// 			continue;
	//
	// 		case SSL_ERROR_ZERO_RETURN:
	// 			// The TLS/SSL connection has been closed
	// 			LOG_MQTTIFY(Error, TEXT("SSL connection was closed during handshake"));
	// 			return false;
	//
	// 		case SSL_ERROR_SSL:
	// 			// A failure in the SSL library occurred, usually a protocol error
	// 			LOG_MQTTIFY(Error, TEXT("SSL protocol error during handshake"));
	// 			return false;
	//
	// 		case SSL_ERROR_SYSCALL:
	// 			// Some I/O error occurred
	// 			LOG_MQTTIFY(Error, TEXT("I/O error during SSL handshake"));
	// 			return false;
	//
	// 		default:
	// 			// Other errors
	// 			LOG_MQTTIFY(Error, TEXT("SSL handshake failed with error %d"), SSLError);
	// 			return false;
	// 		}
	// 	}
	//
	// 	if (ConnectionSettings->ShouldVerifyServerCertificate())
	// 	{
	// 		const long VerifyResult = SSL_get_verify_result(Ssl);
	// 		if (VerifyResult != X509_V_OK)
	// 		{
	// 			LOG_MQTTIFY(
	// 				Error,
	// 				TEXT("SSL certificate verification failed: %hs"),
	// 				X509_verify_cert_error_string(VerifyResult));
	// 			return false;
	// 		}
	// 	}
	//
	// 	LOG_MQTTIFY(Verbose, TEXT("SSL handshake completed successfully"));
	// 	return true;
	// }

	BIO_METHOD* FMqttifySecureSocket::GetSocketBioMethod()
	{
		static BIO_METHOD* Method = nullptr;
		if (nullptr == Method)
		{
			const int32 BioId = BIO_get_new_index() | BIO_TYPE_SOURCE_SINK;
			Method = BIO_meth_new(BioId, "Socket BIO");
			BIO_meth_set_write(Method, SocketBioWrite);
			BIO_meth_set_read(Method, SocketBioRead);
			BIO_meth_set_ctrl(Method, SocketBioCtrl);
			BIO_meth_set_create(Method, SocketBioCreate);
			BIO_meth_set_destroy(Method, SocketBioDestroy);
		}
		return Method;
	}

	int FMqttifySecureSocket::SocketBioWrite(BIO* Bio, const char* Buf, int BufferSize)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("Writing: %d bytes"), BufferSize);
		const FMqttifySecureSocket* Socket = static_cast<FMqttifySecureSocket*>(BIO_get_data(Bio));
		if (!Socket || !Socket->Socket.IsValid())
		{
			return -1;
		}

		BIO_clear_retry_flags(Bio);
		int32 BytesSent = 0;
		const bool bSuccess = Socket->Socket->Send(reinterpret_cast<const uint8*>(Buf), BufferSize, BytesSent);

		if (!bSuccess)
		{
			LOG_MQTTIFY(VeryVerbose, TEXT("Socket write failed"));
			// TODO consider using BIO_set_flags(Bio, BIO_FLAGS_WRITE | BIO_FLAGS_SHOULD_RETRY);
			return -1;
		}

		return BytesSent;
	}

	int FMqttifySecureSocket::SocketBioRead(BIO* Bio, char* Buf, int BufferSize)
	{
		const FMqttifySecureSocket* Socket = static_cast<FMqttifySecureSocket*>(BIO_get_data(Bio));
		BIO_clear_retry_flags(Bio);
		if (!Socket || !Socket->Socket.IsValid())
		{
			return -1;
		}

		uint32 Size = 0;
		if (!Socket->Socket->HasPendingData(Size))
		{
			return 0;
		}

		LOG_MQTTIFY(VeryVerbose, TEXT("Buffer address: %p, size: %d"), Buf, BufferSize);

		int32 BytesRead = 0;
		const bool bSuccess = Socket->Socket->Recv(
			reinterpret_cast<uint8*>(Buf),
			BufferSize,
			BytesRead,
			ESocketReceiveFlags::None);

		if (!bSuccess)
		{
			LOG_MQTTIFY(VeryVerbose, TEXT("Socket read failed"));
			return -1;
		}

		if (BytesRead == 0)
		{
			LOG_MQTTIFY(VeryVerbose, TEXT("BytesRead is 0"));
			return 0;
		}

		LOG_MQTTIFY_PACKET_DATA(VeryVerbose, reinterpret_cast<uint8*>(Buf), BytesRead, TEXT("SocketBioRead"));
		return BytesRead;
	}

	long FMqttifySecureSocket::SocketBioCtrl(BIO* Bio, int Cmd, long Num, void* Ptr)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("SocketBioCtrl: Cmd %d, Num %ld"), Cmd, Num);
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
		BIO_set_init(Bio, 1);
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

	int FMqttifySecureSocket::SslCertVerify(int PreverifyOk, X509_STORE_CTX* Context)
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
#endif // WITH_SSL
} // namespace Mqttify
