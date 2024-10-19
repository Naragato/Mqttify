#pragma once

namespace Mqttify
{
	class IMqttifySocketTickable
	{
	public:
		virtual ~IMqttifySocketTickable() = default;
		virtual void Tick() = 0;
	};
} // namespace Mqttify
