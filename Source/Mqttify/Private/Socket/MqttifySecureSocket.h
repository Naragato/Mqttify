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
		static constexpr uint32 kBufferSize = 1024 * 1024 + 1;
		FUniqueSocket Socket;
		TArray<uint8> ReadBuffer;
		mutable FCriticalSection SocketAccessLock;

		std::atomic<EMqttifySocketState> CurrentState;
		bool bUseSSL;

#if WITH_SSL
		SSL_CTX* SslCtx;
		SSL* Ssl;
		BIO* WriteBio;
		BIO* ReadBio;
#endif // WITH_SSL

	private:
		void Disconnect_Internal();
		void InitializeSocket();
		bool IsSocketReadyForWrite() const;
#if WITH_SSL
		bool InitializeSSL();
		void CleanupSSL();
		bool PerformSSLHandshake() const;
		static BIO_METHOD* GetSocketBioMethod();
		static int SocketBioWrite(BIO* Bio, const char* Buf, int BufferSize);
		static int SocketBioRead(BIO* Bio, char* Buf, int BufferSize);
		static long SocketBioCtrl(BIO* Bio, int Cmd, long Num, void* Ptr);
		static int SocketBioCreate(BIO* Bio);
		static int SocketBioDestroy(BIO* Bio);
		static int SslCertVerify(int PreverifyOk, X509_STORE_CTX* Context);

	private:
#endif // WITH_SSL
	};
} // namespace Mqttify
