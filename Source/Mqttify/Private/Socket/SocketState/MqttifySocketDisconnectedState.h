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

		EMqttifySocketState GetState() const override { return EMqttifySocketState::Disconnected; }
		void Connect() override;
		IMqttifySocketConnectable* AsConnectable() override { return this; }
	};
} // namespace Mqttify
