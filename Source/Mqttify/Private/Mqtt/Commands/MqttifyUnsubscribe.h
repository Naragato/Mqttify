#pragma once

#include "Mqtt/Commands/MqttifyAcknowledgeable.h"
#include "Packets/MqttifyUnsubscribePacket.h"

struct FMqttifyUnsubscribeResult;

namespace Mqttify
{
	/// @brief Unsubscribe command
	class FMqttifyUnsubscribe final : public TMqttifyAcknowledgeable<TArray<FMqttifyUnsubscribeResult>>
	{
	public:
		/**
		 * @brief Construct a new FMqttifyUnsubscribe object
		 * @param InTopicFilters The topic filters to unsubscribe from
		 * @param InPacketId The packet id
		 * @param InSocket The socket to send the packet over
		 * @param InConnectionSettings The connection settings
		 */
		explicit FMqttifyUnsubscribe(const TArray<FMqttifyTopicFilter>& InTopicFilters,
		                             const uint16 InPacketId,
		                             const TWeakPtr<FMqttifySocketBase>& InSocket,
		                             const FMqttifyConnectionSettingsRef& InConnectionSettings);
		virtual bool Acknowledge(const FMqttifyPacketPtr& InPacket) override;
		virtual void Abandon() override;

	private:
		bool bIsDone;
		TArray<FMqttifyTopicFilter> TopicFilters;
		virtual bool IsDone() const override;
		virtual bool NextImpl() override;
	};
}
