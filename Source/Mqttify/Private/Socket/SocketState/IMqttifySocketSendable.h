#pragma once

namespace Mqttify
{
	class IMqttifySocketSendable
	{
	public:
		virtual ~IMqttifySocketSendable() = default;
		virtual void Send(const uint8* Data, uint32 Size) = 0;
	};
} // namespace Mqttify
