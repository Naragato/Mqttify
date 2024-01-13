#include "Mqtt/State/MqttifyClientDisconnectingState.h"

#include "Mqtt/State/MqttifyClientDisconnectedState.h"
#include "Packets/MqttifyDisconnectPacket.h"
#include "Serialization/MemoryWriter.h"

namespace Mqttify
{
	FMqttifyClientDisconnectingState::FMqttifyClientDisconnectingState(
		const FOnStateChangedDelegate& InOnStateChanged,
		const TSharedRef<FMqttifyClientContext>& InContext,
		const FMqttifySocketPtr& InSocket)
		: FMqttifyClientState{ InOnStateChanged, InContext }
		, Socket{ InSocket }
	{
		Socket->GetOnDisconnectDelegate().AddRaw(this, &FMqttifyClientDisconnectingState::OnSocketDisconnect);

		if (Socket->IsConnected())
		{
			TMqttifyDisconnectPacket<GMqttifyProtocol> DisconnectPacket;
			TArray<uint8> ActualBytes;
			FMemoryWriter Writer(ActualBytes);
			DisconnectPacket.Encode(Writer);
			Socket->Send(ActualBytes.GetData(), ActualBytes.Num());
			Socket->Disconnect();
		}

		InContext->CompleteDisconnect();
		TransitionTo(MakeUnique<FMqttifyDisconnectedState>(OnStateChanged, Context));
	}

	TFuture<TMqttifyResult<void>> FMqttifyClientDisconnectingState::DisconnectAsync()
	{
		Context->CompleteDisconnect();
		return Context->GetDisconnectPromise()->GetFuture();
	}

	void FMqttifyClientDisconnectingState::OnSocketDisconnect()
	{
		TransitionTo(MakeUnique<FMqttifyDisconnectedState>(OnStateChanged, Context));
	}
} // namespace Mqttify
