#pragma once

#include "Mqtt/Commands/MqttifyAcknowledgeable.h"
#include "Mqtt/Delegates/OnMessage.h"
#include "Packets/MqttifySubscribePacket.h"

struct FMqttifySubscribeResult;

namespace Mqttify
{
	class FMqttifySubscribe final : public TMqttifyAcknowledgeable<TArray<FMqttifySubscribeResult>>
	{
	public:
		explicit FMqttifySubscribe(
			const TArray<TTuple<FMqttifyTopicFilter, TSharedRef<FOnMessage>>>& InTopicFilters,
			uint16 InPacketId,
			const TWeakPtr<FMqttifySocketBase>& InSocket,
			const FMqttifyConnectionSettingsRef& InConnectionSettings);

		virtual void Abandon() override;
		virtual bool Acknowledge(const FMqttifyPacketPtr& InPacket) override;

	protected:
		virtual bool IsDone() const override;
		virtual bool NextImpl() override;
		bool bIsDone;
		TArray<TTuple<FMqttifyTopicFilter, TSharedRef<FOnMessage>>> TopicFilters;
	};
}
