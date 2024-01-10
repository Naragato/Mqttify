#pragma once

#include "Socket/SocketState/IMqttifySocketDisconnectable.h"
#include "Socket/SocketState/IMqttifySocketTickable.h"
#include "Socket/SocketState/MqttifySocketState.h"

namespace Mqttify
{
	class FMqttifySocketConnectingState final : public FMqttifySocketState,
												public IMqttifySocketDisconnectable,
												public IMqttifySocketTickable
	{
	public:
		explicit FMqttifySocketConnectingState(FMqttifySocket* InMqttifySocket)
			: FMqttifySocketState{ nullptr, InMqttifySocket }
			, RemoteAddress{ nullptr }
			, TimeoutTime{ 0 } {}

		void Tick() override;
		void Disconnect() override;
		EMqttifySocketState GetState() const override { return EMqttifySocketState::Connecting; }

		IMqttifySocketDisconnectable* AsDisconnectable() override { return this; }
		IMqttifySocketTickable* AsTickable() override { return this; }

	private:
		// 1Mb
		static constexpr uint32 kBufferSize = 1024 * 1024 + 1;
		TSharedPtr<FInternetAddr> RemoteAddress;
		std::atomic<double> TimeoutTime;
	};
} // namespace Mqttify
