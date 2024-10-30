#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "LogMqttify.h"
#include "Containers/Queue.h"
#include "HAL/PlatformProcess.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "Misc/Timeout.h"
#include "Templates/SharedPointer.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif // PLATFORM_WINDOWS
#define UI UI_ST
THIRD_PARTY_INCLUDES_START
#pragma warning(push, 0)
#include "libwebsockets.h"
THIRD_PARTY_INCLUDES_END
#undef UI

#if PLATFORM_WINDOWS
#define INVALID_SOCKET  (SOCKET)(~0)
#else
#define INVALID_SOCKET -1
#endif // PLATFORM_WINDOWS

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

		explicit FLibWebSocketRunnable()
		{
			lws_context_creation_info ContextInfo;
			FMemory::Memzero(&ContextInfo, sizeof(ContextInfo));
			ContextInfo.port = 9001;
			ContextInfo.iface = "127.0.0.1";
			ContextInfo.protocols = Protocols;
			ContextInfo.gid = -1;
			ContextInfo.uid = -1;
			ContextInfo.options = LWS_SERVER_OPTION_ALLOW_LISTEN_SHARE | LWS_SERVER_OPTION_DISABLE_IPV6;
			ContextInfo.user = this;
			Context = lws_create_context(&ContextInfo);
			bConnectionState.store(ELibWebSocketState::Connected, std::memory_order_release);
			Thread = FRunnableThread::Create(
				this,
				TEXT("WebSocketRunnable"),
				0,
				TPri_Normal,
				FPlatformAffinity::GetPoolThreadMask());
		}

		void Send(const uint8* Data, const uint32 Size)
		{
			if (nullptr == UserWebsocketInstance)
			{
				return;
			}

			Payloads.Enqueue(TArray<uint8>(Data, Size));
		}

		virtual void Stop() override
		{
			FRunnable::Stop();
			bIsStopping.store(true, std::memory_order_release);
			bConnectionState.store(ELibWebSocketState::Disconnected, std::memory_order_release);
		}

		bool IsConnected() const
		{
			return bConnectionState.load(std::memory_order_acquire) == ELibWebSocketState::Connected;
		}

		virtual ~FLibWebSocketRunnable() override
		{
			LOG_MQTTIFY(Display, TEXT("Destroying websocket"));
			if (Thread)
			{
				bIsStopping.store(true, std::memory_order_release);
				Thread->WaitForCompletion();
				Thread->Kill(true);
				delete Thread;
				Thread = nullptr;
			}

			if (nullptr != Context)
			{
				lws_context* ExistingContext = Context;
				Context = nullptr;
				lws_context_destroy(ExistingContext);
			}
		}

		FORCEINLINE bool Open()
		{
			if (bConnectionState.load(std::memory_order_acquire) == ELibWebSocketState::Connected)
			{
				LOG_MQTTIFY(Display, TEXT("WebSocket already connected."));
				return true;
			}

			ELibWebSocketState Expected = ELibWebSocketState::Disconnected;
			if (bConnectionState.compare_exchange_strong(Expected, ELibWebSocketState::Connecting))
			{
				LOG_MQTTIFY(Display, TEXT("Transitioned from Disconnected to Connecting."));
			}
			else
			{
				LOG_MQTTIFY(
					Display,
					TEXT("Failed to transition to Connecting, current state: %d"),
					static_cast<int>(bConnectionState.load(std::memory_order_acquire)));
				return false;
			}

			if (nullptr == Context)
			{
				LOG_MQTTIFY(Error, TEXT("Failed to create websocket context."));
				bConnectionState.store(ELibWebSocketState::Disconnected, std::memory_order_release);
				return false;
			}

			const UE::FTimeout Timeout{FTimespan::FromSeconds(30)};
			while (bConnectionState.load(std::memory_order_acquire) == ELibWebSocketState::Connecting && !bIsStopping.
				load(std::memory_order_acquire) && Timeout.GetRemainingTime() > FTimespan::Zero())
			{
				LOG_MQTTIFY(
					Display,
					TEXT("Waiting for connection. Remaining time: %f seconds"),
					Timeout.GetRemainingTime().GetTotalSeconds());

				if (UserWebsocketInstance)
				{
					const lws_sockfd_type SocketFd = lws_get_socket_fd(UserWebsocketInstance);
					if (INVALID_SOCKET != SocketFd)
					{
						LOG_MQTTIFY(Display, TEXT("WebSocket has a valid socket FD, likely connected."));
					}
					else
					{
						LOG_MQTTIFY(
							Display,
							TEXT("WebSocket does not have a valid socket FD, connection may not be established yet."));
					}
				}
				FPlatformProcess::YieldThread();
			}

			ELibWebSocketState CurrentState = bConnectionState.load(std::memory_order_acquire);
			if (CurrentState != ELibWebSocketState::Connected)
			{
				LOG_MQTTIFY(Error, TEXT("Failed to open websocket. Final state: %d"), static_cast<int>(CurrentState));
				bConnectionState.store(ELibWebSocketState::Disconnected, std::memory_order_release);
				return false;
			}

			LOG_MQTTIFY(Display, TEXT("WebSocket connected successfully."));
			return true;
		}

	private:
		FORCEINLINE static int WebsocketCallback(
			lws* Wsi,
			const lws_callback_reasons Reason,
			void* User,
			void* In,
			const size_t Len
			)
		{
			switch (Reason)
			{
			case LWS_CALLBACK_PROTOCOL_INIT:
				{
					LOG_MQTTIFY(Display, TEXT("Websocket protocol init."));
				}
				break;
			case LWS_CALLBACK_PROTOCOL_DESTROY:
				{
					LOG_MQTTIFY(Display, TEXT("Websocket protocol destroy."));
#if PLATFORM_WINDOWS
					{
						const lws_sockfd_type Fd = lws_get_socket_fd(Wsi);
						if (Fd != INVALID_SOCKET)
						{
							closesocket(Fd);
						}
					}
#endif
				}
				break;
			default:
				break;
			}

			lws_context* Context = lws_get_context(Wsi);
			if (Context == nullptr)
			{
				LOG_MQTTIFY(Error, TEXT("Failed to get lws_context from Wsi."));
				return -1;
			}

			void* UserData = lws_context_user(Context);
			if (UserData != nullptr)
			{
				auto* Instance = static_cast<FLibWebSocketRunnable*>(UserData);
				return Instance->WebsocketCallback_Internal(Wsi, Reason, User, In, Len);
			}
			else
			{
				switch (Reason)
				{
				case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
					{
						LOG_MQTTIFY(Error, TEXT("Websocket connection error, but no instance available."));
						return -1;
					}
				case LWS_CALLBACK_CLOSED:
					{
						LOG_MQTTIFY(Display, TEXT("Websocket connection closed, but no instance available."));
						return -1;
					}
				default:
					break;
				}
			}

			return -1;
		}

		FORCEINLINE int WebsocketCallback_Internal(
			lws* Wsi,
			const lws_callback_reasons Reason,
			void* User,
			void* In,
			const size_t Len
			)
		{
			LOG_MQTTIFY(Display, TEXT("Websocket callback. Reason: %d"), static_cast<int>(Reason));

			switch (Reason)
			{
			case LWS_CALLBACK_ESTABLISHED:
				{
					LOG_MQTTIFY(Display, TEXT("Websocket connection established."));
					UserWebsocketInstance = Wsi;
					bConnectionState.store(ELibWebSocketState::Connected, std::memory_order_release);
					lws_set_timeout(Wsi, NO_PENDING_TIMEOUT, 0);
				}
				break;

			case LWS_CALLBACK_RECEIVE:
				{
					LOG_MQTTIFY(Display, TEXT("Websocket data received."));
					if (Callback)
					{
						Callback(Wsi, Reason, User, In, Len);
					}
					lws_set_timeout(Wsi, NO_PENDING_TIMEOUT, 0);
				}
				break;

			case LWS_CALLBACK_SERVER_WRITEABLE:
				{
					LOG_MQTTIFY(Display, TEXT("Websocket is ready to send data."));
					TArray<uint8> Payload;
					if (Payloads.Dequeue(Payload) && Payload.Num() > 0)
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
					LOG_MQTTIFY(Error, TEXT("Websocket connection error."));
					bConnectionState.store(ELibWebSocketState::Disconnected, std::memory_order_release);
					if (Callback)
					{
						Callback(Wsi, Reason, User, In, Len);
					}
				}
				break;
			case LWS_CALLBACK_CLOSED:
				{
					LOG_MQTTIFY(Display, TEXT("Websocket connection closed."));
					bConnectionState.store(ELibWebSocketState::Disconnected, std::memory_order_release);
					if (Callback)
					{
						Callback(Wsi, Reason, User, In, Len);
					}
				}
				break;
			case LWS_CALLBACK_WSI_DESTROY:
				{
					LOG_MQTTIFY(Display, TEXT("Websocket instance destroyed."));
				}
				break;
			default:
				break;
			}
			return 0;
		}

		virtual uint32 Run() override
		{
			lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_DEBUG | LLL_INFO, nullptr);
			while (!bIsStopping.load(std::memory_order_acquire))
			{
				if (nullptr != Context)
				{
					lws_service(Context, /* timeout_ms = */ 50);
					lws_callback_on_writable_all_protocol(Context, Protocols);
				}

				if (nullptr != UserWebsocketInstance)
				{
					lws_callback_on_writable(UserWebsocketInstance);
				}

				FPlatformProcess::YieldThread();
			}

			return 0;
		}

	private:
		std::atomic<bool> bIsStopping{false};
		std::atomic<ELibWebSocketState> bConnectionState{ELibWebSocketState::Disconnected};
		lws_context* Context = nullptr;
		lws* UserWebsocketInstance = nullptr;

		FRunnableThread* Thread = nullptr;

		TQueue<TArray<uint8>, EQueueMode::Mpsc> Payloads;
		static constexpr size_t kServBufferSize{4096};
		static constexpr size_t txBuffSize{kServBufferSize * 2};

		inline static lws_protocols Protocols[] = {
			{"mqtt", &FLibWebSocketRunnable::WebsocketCallback, 0, 0, 1, nullptr, txBuffSize},
			{"mqttv3.1", &FLibWebSocketRunnable::WebsocketCallback, 0, 0, 2, nullptr, txBuffSize},
			{nullptr, nullptr, 0, 0} // Terminator
		};
	};
} // namespace Mqttify
#endif // WITH_DEV_AUTOMATION_TESTS
