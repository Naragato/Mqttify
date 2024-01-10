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

		void Tick() override;
		EMqttifySocketState GetState() const override { return EMqttifySocketState::Disconnecting; }
		void Connect() override;
		IMqttifySocketConnectable* AsConnectable() override { return this; }
		IMqttifySocketTickable* AsTickable() override { return this; }
	};
} // namespace Mqttify
