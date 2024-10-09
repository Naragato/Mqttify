#pragma once

#include "Socket/SocketState/IMqttifySocketConnectable.h"
#include "Socket/SocketState/IMqttifySocketTickable.h"
#include "Socket/SocketState/MqttifySocketState.h"

namespace Mqttify
{
	class FMqttifySocketDisconnectingState final : public FMqttifySocketState,
													public IMqttifySocketConnectable,
													public IMqttifySocketTickable
	{
	public:
		explicit FMqttifySocketDisconnectingState(FUniqueSocket&& InSocket,
												FMqttifySocket* InMqttifySocket,
												SSL* InSsl = nullptr)
			: FMqttifySocketState{ MoveTemp(InSocket), InMqttifySocket, InSsl } {}

		virtual void Tick() override;
		virtual EMqttifySocketState GetState() const override { return EMqttifySocketState::Disconnecting; }
		virtual void Connect() override;
		virtual IMqttifySocketConnectable* AsConnectable() override { return this; }
		virtual IMqttifySocketTickable* AsTickable() override { return this; }
	};
} // namespace Mqttify
