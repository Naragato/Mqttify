#pragma once

#include "CoreMinimal.h"
#include "Mqtt/MqttifyConnectionSettings.h"
#include "Socket/Interface/IMqttifySocket.h"
#include "SocketState/MqttifySocketState.h"

namespace Mqttify
{
	/// @brief Raw Socket Implementation of FMqttifySocket
	class FMqttifySocket final : public IMqttifySocket
	{
	private:
		const FMqttifyConnectionSettingsRef ConnectionSettings;
		TUniquePtr<FMqttifySocketState> CurrentState;
		mutable FCriticalSection SocketAccessLock;

		void TransitionTo(TUniquePtr<FMqttifySocketState>&& InNewState);

	public:
		explicit FMqttifySocket(const FMqttifyConnectionSettingsRef& InConnectionSettings);

		~FMqttifySocket() override = default;
		void Connect() override;
		void Disconnect() override;
		void Send(const uint8* Data, uint32 Size) override;
		void Tick() override;
		bool IsConnected() const override;

		FMqttifyConnectionSettingsRef GetConnectionSettings() const { return ConnectionSettings; }

	private:
		friend class FMqttifySocketState;
	};
} // namespace Mqttify
