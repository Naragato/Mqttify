#pragma once
#include "MqttifyAcknowledgeable.h"

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

		virtual bool Acknowledge(const FMqttifyPacketPtr& InPacket) override;

		virtual bool NextImpl() override;

		virtual void Abandon() override;

	protected:
		virtual bool IsDone() const override;
	};
}
