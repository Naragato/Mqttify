#pragma once

#include "Packets/MqttifyPacketType.h"
#include "Packets/Interface/IMqttifyControlPacket.h"

namespace Mqttify
{
	/**
	 * @brief This is a base class for MQTT control packets.  "The MQTT protocol works by exchanging a series of MQTT Control Packets in a defined way."
	 * https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901019
	 * http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_MQTT_Control_Packets
	 *
	 */
	template <EMqttifyPacketType TPacketType>
	class TMqttifyControlPacket
		: public IMqttifyControlPacket, public TSharedFromThis<TMqttifyControlPacket<TPacketType>>
	{
	public:
		static constexpr TCHAR InvalidPacket[] = TEXT("Invalid packet.");

		explicit TMqttifyControlPacket() {}

		explicit TMqttifyControlPacket(const FMqttifyFixedHeader& InFixedHeader)
			: FixedHeader{ InFixedHeader } {}

		virtual ~TMqttifyControlPacket() override = default;

		/**
		 * @brief Encode the packet.
		 * @return The packet type.
		 */
		virtual EMqttifyPacketType GetPacketType() const override
		{
			return TPacketType;
		}

		/**
		 * @brief Is this packet valid?
		 */
		virtual bool IsValid() const override
		{
			return bIsValid;
		}

	protected:
		FMqttifyFixedHeader FixedHeader;
		bool bIsValid                                = true;
		static constexpr uint8 StringLengthFieldSize = sizeof(uint16);
	};
} // namespace Mqttify
