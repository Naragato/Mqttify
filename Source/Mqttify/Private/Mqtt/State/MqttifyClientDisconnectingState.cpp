#include "Mqtt/State/MqttifyClientDisconnectingState.h"

#include "MqttifyConstants.h"
#include "Mqtt/State/MqttifyClientDisconnectedState.h"
#include "Packets/MqttifyDisconnectPacket.h"
#include "Serialization/MemoryWriter.h"

namespace Mqttify
{
	FMqttifyClientDisconnectingState::FMqttifyClientDisconnectingState(
		const FOnStateChangedDelegate& InOnStateChanged,
		const TSharedRef<FMqttifyClientContext>& InContext,
		const FMqttifySocketRef& InSocket
		)
		: FMqttifyClientState{InOnStateChanged, InContext}
		, Socket{InSocket}
	{
		if (Socket->IsConnected())
		{
			return;
		}

		TransitionTo(MakeShared<FMqttifyClientDisconnectedState>(OnStateChanged, Context, Socket));
	}

	FMqttifyClientDisconnectingState::~FMqttifyClientDisconnectingState()
	{
		Context->CompleteDisconnect();
	}

	TFuture<TMqttifyResult<void>> FMqttifyClientDisconnectingState::DisconnectAsync()
	{
		return Context->GetDisconnectPromise()->GetFuture();
	}

	void FMqttifyClientDisconnectingState::OnSocketDisconnect()
	{
		TransitionTo(MakeShared<FMqttifyClientDisconnectedState>(OnStateChanged, Context, Socket));
	}

	void FMqttifyClientDisconnectingState::Tick()
	{
		if (Socket->IsConnected())
		{
			const auto DisconnectPacketRef = MakeShared<TMqttifyDisconnectPacket<GMqttifyProtocol>>();
			Socket->Send(DisconnectPacketRef);
			Socket->Disconnect();
		}
		else
		{
			TransitionTo(MakeShared<FMqttifyClientDisconnectedState>(OnStateChanged, Context, Socket));
		}
	}
} // namespace Mqttify
