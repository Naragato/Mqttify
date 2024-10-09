#pragma once

#include "CoreMinimal.h"
#include "Mqtt/MqttifyConnectionSettings.h"
#include "Socket/Interface/MqttifySocketBase.h"
#include "SocketState/MqttifySocketState.h"

namespace Mqttify
{
	/// @brief Raw Socket Implementation of FMqttifySocket
	class FMqttifySocket final : public FMqttifySocketBase, public TSharedFromThis<FMqttifySocket> 
	{
	private:
		const FMqttifyConnectionSettingsRef ConnectionSettings;
		TUniquePtr<FMqttifySocketState> CurrentState;
		mutable FCriticalSection SocketAccessLock{};

		void TransitionTo(TUniquePtr<FMqttifySocketState>&& InNewState);

	public:
		explicit FMqttifySocket(const FMqttifyConnectionSettingsRef& InConnectionSettings);

		virtual ~FMqttifySocket() override = default;
		virtual void Connect() override;
		virtual void Disconnect() override;
		virtual void Send(const uint8* Data, uint32 Size) override;
		virtual void Tick() override;
		virtual bool IsConnected() const override;

		FMqttifyConnectionSettingsRef GetConnectionSettings() const { return ConnectionSettings; }

	private:
		friend class FMqttifySocketState;
	};
} // namespace Mqttify
