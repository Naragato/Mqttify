#include "Socket/SocketState/MqttifySocketConnectingState.h"

#include "LogMqttify.h"
#include "MqttifySocketConnectedState.h"
#include "MqttifySocketDisconnectingState.h"
#include "Sockets.h"
#include "Socket/MqttifySocket.h"

namespace Mqttify
{
	void FMqttifySocketConnectingState::Tick()
	{
		const FMqttifyConnectionSettingsRef ConnectionSettings = MqttifySocket->GetConnectionSettings();
		double Expected = 0;
		const double Desired = FPlatformTime::Seconds() + ConnectionSettings->GetSocketConnectionTimoutSeconds();
		TimeoutTime.compare_exchange_strong(Expected, Desired);

		if (!Socket.IsValid())
		{
			ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
			Socket = SocketSubsystem->CreateUniqueSocket(NAME_Stream, TEXT("MQTT Connection"));

			if (!Socket.IsValid())
			{
				LOG_MQTTIFY(Error, TEXT("Error creating socket."));
				TransitionTo(MakeUnique<FMqttifySocketDisconnectingState>(MoveTemp(Socket), MqttifySocket));
				MqttifySocket->GetOnConnectDelegate().Broadcast(false);
				return;
			}

			if (ConnectionSettings->GetTransportProtocol() == EMqttifyConnectionProtocol::Mqtts)
			{
				// Can we get the raw Socket ?
				// We may have to use asio instead of FSocket.
				// auto* BSDSocket = static_cast<FSocketBSD*>(Socket.Get());
			}

			Socket->SetReuseAddr(true);
			Socket->SetNonBlocking(true);
			Socket->SetRecvErr(true);
			int32 BufferSize = 0;
			Socket->SetReceiveBufferSize(kBufferSize, BufferSize);
			LOG_MQTTIFY(Verbose, TEXT("Socket receive buffer size: %d."), BufferSize);
			Socket->SetSendBufferSize(kBufferSize, BufferSize);
			LOG_MQTTIFY(Verbose, TEXT("Socket send buffer size: %d."), BufferSize);

			FAddressInfoResult AddressInfoResult = SocketSubsystem->GetAddressInfo(
				*ConnectionSettings->GetHost(),
				nullptr,
				EAddressInfoFlags::Default,
				NAME_None,
				SOCKTYPE_Streaming);

			if (AddressInfoResult.ReturnCode != SE_NO_ERROR)
			{
				LOG_MQTTIFY(Error, TEXT("Invalid address. AddressInfo return code: %d."), AddressInfoResult.ReturnCode);
				TransitionTo(MakeUnique<FMqttifySocketDisconnectingState>(MoveTemp(Socket), MqttifySocket));
				MqttifySocket->GetOnConnectDelegate().Broadcast(false);
				return;
			}

			if (AddressInfoResult.Results.Num() <= 0)
			{
				LOG_MQTTIFY(Error, TEXT("Not results for provided address: %s."), *ConnectionSettings->ToString());
				TransitionTo(MakeUnique<FMqttifySocketDisconnectingState>(MoveTemp(Socket), MqttifySocket));
				MqttifySocket->GetOnConnectDelegate().Broadcast(false);
				return;
			}

			const TSharedRef<FInternetAddr> Addr = AddressInfoResult.Results[0].Address;

			if (!Addr->IsValid())
			{
				LOG_MQTTIFY(Error, TEXT("Invalid Address %s."), *ConnectionSettings->ToString());
				TransitionTo(MakeUnique<FMqttifySocketDisconnectingState>(MoveTemp(Socket), MqttifySocket));
				MqttifySocket->GetOnConnectDelegate().Broadcast(false);
				return;
			}

			Addr->SetPort(ConnectionSettings->GetPort());
			RemoteAddress = Addr.ToSharedPtr();
		}

		if (Socket.IsValid() && RemoteAddress.IsValid())
		{
			if (Socket->Connect(*RemoteAddress) && IsConnected())
			{
				LOG_MQTTIFY(Verbose, TEXT("Socket connected. %s"), *MqttifySocket->GetConnectionSettings()->ToString());
				TransitionTo(MakeUnique<FMqttifySocketConnectedState>(MoveTemp(Socket), MqttifySocket));
				MqttifySocket->GetOnConnectDelegate().Broadcast(true);
				return;
			}
		}

		if (TimeoutTime.load(std::memory_order_acquire) < FPlatformTime::Seconds())
		{
			LOG_MQTTIFY(Error,
						TEXT("Connection timed out: %f, for %s"),
						TimeoutTime.load(),
						*ConnectionSettings->ToString());
			TransitionTo(MakeUnique<FMqttifySocketDisconnectingState>(MoveTemp(Socket), MqttifySocket));
			MqttifySocket->GetOnConnectDelegate().Broadcast(false);
		}
	}

	void FMqttifySocketConnectingState::Disconnect()
	{
		TransitionTo(MakeUnique<FMqttifySocketDisconnectingState>(MoveTemp(Socket), MqttifySocket));
	}
} // namespace Mqttify
