#pragma once
#include "MqttifyAcknowledgeable.h"
#include "Mqtt/MqttifyProtocolVersion.h"

namespace Mqttify
{
	class FMqttifyPingReq final : public TMqttifyAcknowledgeable<void>
	{
	private:
		bool bIsDone{ false };

	public:
		FMqttifyPingReq(const TWeakPtr<FMqttifySocketBase>& InSocket,
						const TSharedRef<FMqttifyConnectionSettings>& InConnectionSettings)
			: TMqttifyAcknowledgeable(0, InSocket, InConnectionSettings) {}

		bool Acknowledge(const FMqttifyPacketPtr& InPacket) override;

		bool NextImpl() override;

		void Abandon() override;

	protected:
		bool IsDone() const override;
	};
}
