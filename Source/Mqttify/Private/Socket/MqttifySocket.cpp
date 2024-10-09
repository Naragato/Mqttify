#include "Socket/MqttifySocket.h"

#include "LogMqttify.h"
#include "SocketState/IMqttifySocketDisconnectable.h"
#include "SocketState/IMqttifySocketSendable.h"
#include "SocketState/IMqttifySocketTickable.h"
#include "SocketState/MqttifySocketDisconnectedState.h"

namespace Mqttify
{
	void FMqttifySocket::TransitionTo(TUniquePtr<FMqttifySocketState>&& InNewState)
	{
		FScopeLock Lock{ &SocketAccessLock };
		if (nullptr == InNewState)
		{
			LOG_MQTTIFY(Error, TEXT("InNewState is null"));
			return;
		}
		LOG_MQTTIFY(
			VeryVerbose,
			TEXT("Transitioning from %s to %s"),
			EnumToTCharString(CurrentState->GetState()),
			EnumToTCharString(InNewState->GetState()));
		CurrentState = MoveTemp(InNewState);
	}

	FMqttifySocket::FMqttifySocket(const FMqttifyConnectionSettingsRef& InConnectionSettings)
		: ConnectionSettings{ InConnectionSettings }
		, CurrentState{ MakeUnique<FMqttifySocketDisconnectedState>(this) } {}

	void FMqttifySocket::Connect()
	{
		FScopeLock Lock{ &SocketAccessLock };
		if (nullptr == CurrentState)
		{
			LOG_MQTTIFY(Error, TEXT("CurrentState is null"));
			return;
		}

		if (IMqttifySocketConnectable* ConnectableState = CurrentState->AsConnectable())
		{
			ConnectableState->Connect();
		}
	}

	void FMqttifySocket::Disconnect()
	{
		FScopeLock Lock{ &SocketAccessLock };
		if (nullptr == CurrentState)
		{
			LOG_MQTTIFY(Error, TEXT("CurrentState is null"));
			return;
		}

		if (IMqttifySocketDisconnectable* DisconnectableState = CurrentState->AsDisconnectable())
		{
			DisconnectableState->Disconnect();
		}
	}

	void FMqttifySocket::Send(const uint8* Data, const uint32 Size)
	{
		FScopeLock Lock{ &SocketAccessLock };
		if (nullptr == CurrentState)
		{
			LOG_MQTTIFY(Error, TEXT("CurrentState is null"));
			return;
		}

		if (IMqttifySocketSendable* SendableState = CurrentState->AsSendable())
		{
			SendableState->Send(Data, Size);
		}
	}

	void FMqttifySocket::Tick()
	{
		FScopeLock Lock{ &SocketAccessLock };
		if (nullptr == CurrentState)
		{
			LOG_MQTTIFY(Error, TEXT("CurrentState is null"));
			return;
		}

		if (IMqttifySocketTickable* TickableState = CurrentState->AsTickable())
		{
			TickableState->Tick();
		}
	}

	bool FMqttifySocket::IsConnected() const
	{
		FScopeLock Lock{ &SocketAccessLock };
		return CurrentState.IsValid() && CurrentState->GetState() == EMqttifySocketState::Connected;
	}
} // namespace Mqttify
