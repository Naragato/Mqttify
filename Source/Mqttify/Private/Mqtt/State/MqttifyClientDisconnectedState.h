#pragma once
#include "Async/Future.h"
#include "Mqtt/MqttifyState.h"
#include "Mqtt/Interface/IMqttifyConnectableAsync.h"
#include "Mqtt/Interface/IMqttifyDisconnectableAsync.h"
#include "Mqtt/State/MqttifyClientState.h"

enum class EMqttifyProtocolVersion : uint8;

namespace Mqttify
{
	class IMqttifyControlPacket;

	class FMqttifyDisconnectedState final : public FMqttifyClientState,
											public IMqttifyConnectableAsync,
											public IMqttifyDisconnectableAsync
	{
	public:
		explicit FMqttifyDisconnectedState(const FOnStateChangedDelegate& InOnStateChanged,
											const TSharedRef<FMqttifyClientContext>& InContext);

		EMqttifyState GetState() override { return EMqttifyState::Disconnected; }
		TFuture<TMqttifyResult<void>> ConnectAsync(bool bCleanSession = false) override;
		IMqttifyConnectableAsync* AsConnectable() override { return this; }

		TFuture<TMqttifyResult<void>> DisconnectAsync() override
		{
			return MakeFulfilledPromise<TMqttifyResult<void>>(TMqttifyResult<void>{ true }).GetFuture();
		}

		IMqttifyDisconnectableAsync* AsDisconnectable() override { return this; }
	};
} // namespace Mqttify
