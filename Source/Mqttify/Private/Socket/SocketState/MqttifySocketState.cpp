#include "Socket/SocketState/MqttifySocketState.h"

#include "Sockets.h"
#include "Socket/MqttifySocket.h"

namespace Mqttify
{
	void FMqttifySocketState::TransitionTo(TUniquePtr<FMqttifySocketState>&& InNewState) const
	{
		MqttifySocket->TransitionTo(MoveTemp(InNewState));
	}

	FMqttifySocketState::FMqttifySocketState(FUniqueSocket&& InSocket, FMqttifySocket* InMqttifySocket, SSL* InSsl)
		: Ssl{ InSsl }
		, Socket(MoveTemp(InSocket))
		, MqttifySocket{ InMqttifySocket } {}

	bool FMqttifySocketState::IsConnected() const
	{
		if (!Socket.IsValid())
		{
			return false;
		}

		if (Socket->GetConnectionState() == SCS_ConnectionError)
		{
			return false;
		}

		int32 BytesRead;
		uint8 Dummy;
		if (!Socket->Recv(&Dummy, 1, BytesRead, ESocketReceiveFlags::Peek))
		{
			return false;
		}

		const bool bConnection = Socket->GetConnectionState() == SCS_Connected;
		return bConnection;
	}
} // namespace Mqttify
