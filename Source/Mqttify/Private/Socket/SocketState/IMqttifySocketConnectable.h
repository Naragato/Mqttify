#pragma once

namespace Mqttify
{
	/// @brief Interface for states where we can connect to a socket
	class IMqttifySocketConnectable
	{
	public:
		virtual ~IMqttifySocketConnectable() = default;
		/// @brief Connect to the socket
		virtual void Connect() = 0;
	};
} // namespace Mqttify
