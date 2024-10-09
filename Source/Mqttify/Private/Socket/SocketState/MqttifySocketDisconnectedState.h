#pragma once

#include "Socket/SocketState/IMqttifySocketConnectable.h"
#include "Socket/SocketState/MqttifySocketState.h"

namespace Mqttify
{
	class FMqttifySocketDisconnectedState final : public FMqttifySocketState, public IMqttifySocketConnectable
	{
	public:
		explicit FMqttifySocketDisconnectedState(FMqttifySocket* InMqttifySocket)
			: FMqttifySocketState{ nullptr, InMqttifySocket, nullptr } {}

		virtual EMqttifySocketState GetState() const override { return EMqttifySocketState::Disconnected; }
		virtual void Connect() override;
		virtual IMqttifySocketConnectable* AsConnectable() override { return this; }
	};
} // namespace Mqttify
