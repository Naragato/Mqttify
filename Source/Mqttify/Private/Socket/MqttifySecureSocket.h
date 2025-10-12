#pragma once

#include "CoreMinimal.h"
#include "Mqtt/MqttifyConnectionSettings.h"
#include "Socket/Interface/MqttifySocketBase.h"
#include "SocketSubsystem.h"

#if WITH_SSL
#define UI UI_ST
#include <openssl/bio.h>
#include <openssl/err.h>
#undef UI
#endif

namespace Mqttify
{
	enum class EMqttifySocketState;

	class FMqttifySecureSocket final : public FMqttifySocketBase, public TSharedFromThis<FMqttifySecureSocket>
	{
	public:
		explicit FMqttifySecureSocket(const FMqttifyConnectionSettingsRef& InConnectionSettings);
		virtual ~FMqttifySecureSocket() override;
		// FMqttifySocketBase
		virtual void Connect() override;
		virtual void Disconnect() override;
		virtual void Close(int32 Code = 1000, const FString& Reason = {}) override;

		virtual void Tick() override;
		virtual bool IsConnected() const override;
		// ~FMqttifySocketBase
	private:
		virtual void Send(const TSharedRef<IMqttifyControlPacket>& InPacket) override;
		virtual void Send(const uint8* InData, uint32 InSize) override;
		static constexpr uint32 kBufferSize  = 2 * 1024 * 1024;
		static constexpr uint32 kMaxChunkSize = 16 * 1024;
		FUniqueSocket Socket;
		mutable FCriticalSection SocketAccessLock;

		std::atomic<EMqttifySocketState> CurrentState;
		bool bUseSSL;

#if WITH_SSL
		SSL_CTX* SslCtx;
		SSL* Ssl;
		BIO* Bio;

#endif // WITH_SSL

		void Disconnect_Internal();
		void InitializeSocket();
		bool IsSocketReadyForWrite() const;
		// Plain socket receive.
		bool ReceiveFromSocket(int32 Want, TArray<uint8>& Tmp, size_t& OutBytesRead) const;
		// Reads pending data (if any) using the provided reader lambda.
		// The reader must have signature: bool(int32 Want, TArray<uint8>& Tmp, int32& BytesRead).
		bool ReadAvailableData(const TUniqueFunction<bool(const int32 Want, TArray<uint8>& Tmp, size_t& OutBytesRead)>&& Reader);
#if WITH_SSL
		bool InitializeSSL();
		void CleanupSSL();
		bool PerformSSLHandshake();
		bool ReceiveFromSSL(int32 Want, TArray<uint8>& Tmp, size_t& BytesRead) const;
		void AppendAndProcess(const uint8* Data, int32 Len);
		static FString GetLastSslErrorString(bool bConsume /*= false*/) noexcept;
		static int32 SslCertVerify(int32 PreverifyOk, X509_STORE_CTX* Context);
		static BIO_METHOD* GetSocketBioMethod();
		static int SocketBioWrite(BIO* Bio, const char* InBuffer, int32 BufferSize);
		static int SocketBioRead(BIO* Bio, char* OutBuffer, int32 BufferSize);
		static long SocketBioCtrl(BIO* Bio, int Cmd, long Num, void* Ptr);
		static int SocketBioCreate(BIO* Bio);
		static int SocketBioDestroy(BIO* Bio);
		
#if !UE_BUILD_SHIPPING
		void DebugSslBioState(EMqttifySocketState State);
		FString LastSslBioDebugLog{};
#endif // !UE_BUILD_SHIPPING
		
#endif // WITH_SSL
	};
} // namespace Mqttify