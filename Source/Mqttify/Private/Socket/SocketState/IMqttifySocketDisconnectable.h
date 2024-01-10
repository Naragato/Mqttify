#pragma once

namespace Mqttify
{
	/// @brief Interface for states where we can disconnect from a socket
	class IMqttifySocketDisconnectable
	{
	public:
		virtual ~IMqttifySocketDisconnectable() = default;
		/// @brief Disconnect from the socket
		virtual void Disconnect() = 0;
	};
} // namespace Mqttify
