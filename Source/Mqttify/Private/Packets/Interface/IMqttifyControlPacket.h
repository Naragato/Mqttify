#pragma once

#include "Packets/MqttifyPacketType.h"
#include "Serialization/Interface/ISerializable.h"
#include "Templates/UniquePtr.h"

namespace Mqttify
{
	class IMqttifyControlPacket : public ISerializable
	{
	public:
		IMqttifyControlPacket()           = default;
		virtual ~IMqttifyControlPacket() override = default;

		/**
		 * @brief Gets the size of the serialized packet.
		 *
		 * Implement this method to return the size of the serialized form of the
		 * control packet. This size includes all headers, payload, and any other
		 * applicable fields.
		 *
		 * @return The size of the serialized packet in bytes.
		 */
		virtual uint32 GetLength() const = 0;

		/**
		 * @brief Gets the type of the MQTT control packet.
		 *
		 * Implement this method to return the specific type of control packet
		 * represented by this instance.
		 *
		 * @return The type of the control packet as defined in the EPacketType enum.
		 */
		virtual EMqttifyPacketType GetPacketType() const = 0;

		/**
		 * @brief Gets the packet id of the control packet.
		 * @return The packet id of the control packet. Defaults to 0.
		 * if the packet is doesn't have a packet id.
		 */
		virtual uint16 GetPacketId() const { return 0; }

		virtual bool IsValid() const = 0;
	};

	using FMqttifyPacketPtr = TUniquePtr<IMqttifyControlPacket>;
} // namespace Mqttify
