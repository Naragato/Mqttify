#pragma once

#include "CoreMinimal.h"
#include "Mqtt/MqttifyConnectionSettings.h"
#include "Socket/Interface/MqttifySocketBase.h"

#define UI UI_ST
#include "boost/asio/ssl.hpp"
#include "boost/asio/ip/tcp.hpp"
#undef UI

namespace Mqttify
{
	enum class EMqttifySocketState;

	using FTcpSocket = boost::asio::ip::tcp::socket;
	using FSslSocket = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

	// TSslContextHolder for non-SSL sockets (empty base class)
	template <typename TSocketType, typename Enable = void>
	class TSslContextHolder
	{
	protected:
		TSslContextHolder() = default;
	};

	template <typename TSocketType>
	class TSslContextHolder<TSocketType, std::enable_if_t<std::is_same<TSocketType, FSslSocket>::value>>
	{
	protected:
		TUniquePtr<boost::asio::ssl::context> SslContext;

		TSslContextHolder()
			: SslContext(MakeUnique<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv13_client))
		{
		}
	};

	template <typename TSocketType>
	class TMqttifySocket final : public FMqttifySocketBase,
	                             public TSharedFromThis<TMqttifySocket<TSocketType>>,
	                             private TSslContextHolder<TSocketType>
	{
	private:
		using FSocketPtr = TSharedPtr<TSocketType>;
		boost::asio::io_context IoContext{};
		boost::asio::ip::tcp::resolver Resolver;
		FSocketPtr Socket;
		TArray<uint8, TFixedAllocator<4096>> ReadBuffer;

		uint32 RemainingLength;
		EMqttifySocketState CurrentState;

	public:
		explicit TMqttifySocket(const FMqttifyConnectionSettingsRef& InConnectionSettings);

		virtual ~TMqttifySocket() override { Disconnect_Internal(); }
		virtual void Connect() override;
		void HandleResolve(
			const boost::system::error_code& InError,
			const boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp>& InEndpoints
			);
		void OnAsyncConnect(const boost::system::error_code& InError);

		// SSL handshake handling
		void OnSocketConnected(const boost::system::error_code& InError);

		void StartAsyncRead();
		void HandleRead(const boost::system::error_code& InError, std::size_t InBytesTransferred);
		// FMqttifySocketBase
		virtual void Disconnect() override;
		virtual void Send(const uint8* InData, uint32 InSize) override;
		virtual void Tick() override;
		virtual bool IsConnected() const override;
		// ~ FMqttifySocketBase
	private:
		void Disconnect_Internal();
	};
} // namespace Mqttify
