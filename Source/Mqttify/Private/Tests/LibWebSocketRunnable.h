#pragma once
#if WITH_AUTOMATION_TESTS

#include <atomic>

#include "CoreMinimal.h"
#include "LogMqttify.h"

#define UI UI_ST
THIRD_PARTY_INCLUDES_START
#pragma warning(push, 0)
#include <libwebsockets.h>
THIRD_PARTY_INCLUDES_END
#undef UI

namespace Mqttify
{
	enum class ELibWebSocketState : uint8
	{
		Disconnected,
		Connecting,
		Connected
	};

	class FLibWebSocketRunnable final : public FRunnable
	{
	public:
		TFunction<int(lws*, const lws_callback_reasons, void*, void*, const size_t)> Callback = nullptr;


		FORCEINLINE static TSharedPtr<FLibWebSocketRunnable> GetInstance()
		{
			if (!Instance)
			{
				Instance = TSharedPtr<FLibWebSocketRunnable>(new FLibWebSocketRunnable());
			}
			return Instance;
		}

		void Send(const uint8* Data, const uint32 Size)
		{
			if (nullptr == UserWebsocketInstance)
			{
				return;
			}

			Payloads.Enqueue(TArray(Data, Size));
		}

		void Stop() override
		{
			FRunnable::Stop();
			bIsStopping.store(true, std::memory_order_release);
			bConnectionState.store(ELibWebSocketState::Disconnected, std::memory_order_release);
		}

		~FLibWebSocketRunnable() override
		{
			LOG_MQTTIFY(Display, TEXT("Destroying websocket."));
			if (Context)
			{
				lws_context* ExistingContext = Context;
				Context                      = nullptr;
				lws_context_destroy(ExistingContext);

				lws_callback_on_writable_all_protocol(ExistingContext, Protocols);
				lws_cancel_service(ExistingContext); // interrupt lws_service loop
				// context destroy drives LWS_CALLBACK_WSI_DESTROY for all live connections
				lws_context_destroy(ExistingContext);
				// event loop
				while (ExistingContext)
				{
					lws_service(ExistingContext, 1000);
				}
			}
			Thread->Kill(true);
			delete Thread;
		}

		bool Open();

	private:
		FORCEINLINE static int HttpCallback(lws* Wsi,
											const lws_callback_reasons Reason,
											void* User,
											void* In,
											const size_t Len)
		{
			if (nullptr == Instance)
			{
				return 0;
			}
			if (nullptr != GetInstance())
			{
				return GetInstance()->HttpCallback_Internal(Wsi, Reason, User, In, Len);
			}
			return 0;
		}

		FORCEINLINE static int WebsocketCallback(lws* Wsi,
												const lws_callback_reasons Reason,
												void* User,
												void* In,
												const size_t Len)
		{
			if (nullptr == Instance)
			{
				return 0;
			}
			if (nullptr != GetInstance())
			{
				return GetInstance()->WebsocketCallback_Internal(Wsi, Reason, User, In, Len);
			}
			return 0;
		}

		FORCEINLINE int HttpCallback_Internal(lws* Lws, lws_callback_reasons Reason, void* User, void* In, size_t Len)
		{
			LOG_MQTTIFY(Display, TEXT("Http callback. Reason: %d"), static_cast<int>(Reason));
			return 0;
		}

		FORCEINLINE int WebsocketCallback_Internal(lws* Wsi,
													const lws_callback_reasons Reason,
													void* User,
													void* In,
													const size_t Len)
		{
			LOG_MQTTIFY(Display, TEXT("Websocket callback. Reason: %d"), static_cast<int>(Reason));

			switch (Reason)
			{
				case LWS_CALLBACK_PROTOCOL_INIT:
					LOG_MQTTIFY(Display, TEXT("Websocket protocol init."));
					bConnectionState.store(ELibWebSocketState::Connected, std::memory_order_release);
					break;
				case LWS_CALLBACK_ESTABLISHED:
				{
					// Websocket connection established
					LOG_MQTTIFY(Display, TEXT("Websocket connection established."));
					UserWebsocketInstance = Wsi;
					lws_set_timeout(Wsi, NO_PENDING_TIMEOUT, 0);
				}
				break;

				case LWS_CALLBACK_RECEIVE:
				{
					// Websocket data received
					LOG_MQTTIFY(Display, TEXT("Websocket data received."));
					Callback(Wsi, Reason, User, In, Len);
					lws_set_timeout(Wsi, NO_PENDING_TIMEOUT, 0);
				}
				break;

				case LWS_CALLBACK_SERVER_WRITEABLE:
				{
					// Websocket is ready to send data
					LOG_MQTTIFY(Display, TEXT("Websocket is ready to send data."));

					// Send data from buffer
					TArray<uint8> Payload;
					Payloads.Dequeue(Payload);
					if (Payload.Num() > 0)
					{
						unsigned char Buffer[LWS_SEND_BUFFER_PRE_PADDING + 512 + LWS_SEND_BUFFER_POST_PADDING];
						unsigned char* P = &Buffer[LWS_SEND_BUFFER_PRE_PADDING];
						memcpy(P, Payload.GetData(), Payload.Num());
						lws_write(Wsi, P, Payload.Num(), LWS_WRITE_BINARY);
					}

					lws_set_timeout(Wsi, NO_PENDING_TIMEOUT, 0);
				}
				break;
				case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
				{
					// Websocket connection error
					LOG_MQTTIFY(Error, TEXT("Websocket connection error."));
					return Callback(Wsi, Reason, User, In, Len);
				}
				case LWS_CALLBACK_CLOSED:
				{
					// Websocket connection closed
					LOG_MQTTIFY(Display, TEXT("Websocket connection closed."));
					return Callback(Wsi, Reason, User, In, Len);
				}
				case LWS_CALLBACK_WSI_DESTROY:
				{
					// Websocket instance destroyed
					LOG_MQTTIFY(Display, TEXT("Websocket instance destroyed."));
				}
				break;
				case LWS_CALLBACK_PROTOCOL_DESTROY:
					// Websocket instance destroyed
					LOG_MQTTIFY(Display, TEXT("Websocket instance destroyed."));
#if PLATFORM_WINDOWS
			{
				// Fix taken from WebsocketServer Plugin for Unreal Engine 5
				lws_sockfd_type Fd = lws_get_socket_fd(Wsi);
				if (Fd != INVALID_SOCKET)
				{
					closesocket(Fd);
				}
			}
#endif
					break;
				case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
				{
					// Websocket connection filtered
					LOG_MQTTIFY(Display, TEXT("Websocket connection filtered."));
					break;
				}
				default:
					break;
			}
			return 0;
		}

		explicit FLibWebSocketRunnable()
		{
			Thread = FRunnableThread::Create(this,
											TEXT("WebSocketRunnable"),
											0,
											TPri_Normal,
											FPlatformAffinity::GetPoolThreadMask());
		}


		uint32 Run() override;

	private:
		inline static TSharedPtr<FLibWebSocketRunnable> Instance = nullptr;
		std::atomic<bool> bIsStopping;
		std::atomic<ELibWebSocketState> bConnectionState;
		lws_context_creation_info Info;
		lws_context* Context       = nullptr;
		lws* UserWebsocketInstance = nullptr;

		FRunnableThread* Thread;

		TQueue<TArray<uint8>, EQueueMode::Mpsc> Payloads;

		inline static lws_protocols Protocols[] = {
			{
				"http",                               // Name of the protocol
				&FLibWebSocketRunnable::HttpCallback, // Callback function
				0,                                    // Per-session data size
				2 * 1024 * 1024,                      // Maximum frame size / rx buffer.
			},
			{
				"mqtt",                                    // Name of the protocol
				&FLibWebSocketRunnable::WebsocketCallback, // Callback function
				0,                                         // Per-session data size
				2 * 1024 * 1024,                           // Maximum frame size / rx buffer.
			},
			{ nullptr, nullptr, 0, 0 } // Terminator
		};
	};

	inline bool FLibWebSocketRunnable::Open()
	{
		if (bConnectionState.load(std::memory_order_acquire) == ELibWebSocketState::Connected)
		{
			return true;
		}

		ELibWebSocketState Expected = ELibWebSocketState::Disconnected;
		if (bConnectionState.compare_exchange_strong(Expected, ELibWebSocketState::Connecting))
		{
			LOG_MQTTIFY(Display, TEXT("Opening websocket connection."));
			Context = lws_create_context(&Info);
		}
		if (nullptr == Context)
		{
			LOG_MQTTIFY(Error, TEXT("Failed to create websocket context."));
			return false;
		}
		const UE::FTimeout Timeout{ FTimespan::FromSeconds(30) };
		// while connection in progress 5 second timeout and we're not stopping
		while (bConnectionState.load(std::memory_order_acquire) == ELibWebSocketState::Connecting &&
			!bIsStopping.load(std::memory_order_acquire) &&
			Timeout.GetRemainingTime() > FTimespan::Zero())
		{
			FPlatformProcess::YieldThread();
		}

		if (bConnectionState.load(std::memory_order_acquire) != ELibWebSocketState::Connected)
		{
			bConnectionState.store(ELibWebSocketState::Disconnected, std::memory_order_release);
			LOG_MQTTIFY(Error, TEXT("Failed to open to websocket."));
		}

		return bConnectionState.load(std::memory_order_acquire) == ELibWebSocketState::Connected;
	}

	inline uint32 FLibWebSocketRunnable::Run()
	{
		lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_DEBUG | LLL_INFO, nullptr);
		FMemory::Memzero(&Info, sizeof(lws_context_creation_info));
		Info.port      = 9001;
		Info.iface     = "127.0.0.1";
		Info.protocols = Protocols;
		Info.gid       = -1;
		Info.uid       = -1;
		Info.options   = LWS_SERVER_OPTION_ALLOW_LISTEN_SHARE;
		Info.user      = this;
		Info.options |= LWS_SERVER_OPTION_DISABLE_IPV6;

		while (!bIsStopping.load(std::memory_order_acquire))
		{
			if (nullptr != Context)
			{
				lws_service(Context, /* timeout_ms = */ 50);
				lws_callback_on_writable_all_protocol(Context, &Protocols[0]);
			}

			if (nullptr != UserWebsocketInstance)
			{
				lws_callback_on_writable(UserWebsocketInstance);
			}

			FPlatformProcess::YieldThread();
		}

		return 0;
	}
} // namespace Mqttify
#endif // WITH_AUTOMATION_TESTS
