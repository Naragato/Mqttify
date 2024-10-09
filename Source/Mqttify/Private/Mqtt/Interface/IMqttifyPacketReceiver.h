#pragma once

class FArrayReader;

namespace Mqttify
{
	/**
	 * @brief Interface for a state that can receive MQTT packets
	 */
	class MQTTIFY_API IMqttifyPacketReceiver
	{
	public:
		virtual ~IMqttifyPacketReceiver() = default;

		/**
         * @brief Receive a packet
         * @param InPacket The packet to receive
         * @return A future that contains the result of the receive, which can be checked for success.
         */
        virtual void OnReceivePacket(const TSharedPtr<FArrayReader>& InPacket) = 0;
	};
} // namespace Mqttify
