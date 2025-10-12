#pragma once

#include "MqttifyEnumToString.h"
#include "HAL/Platform.h"

namespace Mqttify
{
	/**
	 * @brief Enum representing MQTT Control Packet types.
	 */
	enum class EMqttifyPacketType : uint8
	{
		/// @brief Non-spec entry.
		None = 0,

		/// @brief CLIENT requests a connection to a Server.
		Connect = 1,

		/// @brief CONNACK - Acknowledge connection request.
		ConnAck = 2,

		/// @brief PUBLISH - Publish message.
		Publish = 3,

		/// @brief PUBACK - Publish acknowledgement for QoS 1.
		PubAck = 4,

		/// @brief PUBREC - Publish received for QoS 2.
		PubRec = 5,

		/// @brief PUBREL - Publish release for QoS 2.
		PubRel = 6,

		/// @brief PUBCOMP - Publish complete for QoS 2.
		PubComp = 7,

		/// @brief SUBSCRIBE - Subscribe request.
		Subscribe = 8,

		/// @brief SUBACK - Subscribe acknowledgement.
		SubAck = 9,

		/// @brief UNSUBSCRIBE - Unsubscribe request.
		Unsubscribe = 10,

		/// @brief UNSUBACK - Unsubscribe acknowledgement.
		UnsubAck = 11,

		/// @brief PINGREQ - Ping request.
		PingReq = 12,

		/// @brief PINGRESP - Ping response.
		PingResp = 13,

		/// @brief DISCONNECT - Disconnect notification.
		Disconnect = 14,

		/// @brief AUTH - Authentication for MQTT 5.0.
		Auth = 15,

		/// @brief Max enum size, used for validation.
		Max
	};

	template <>
	FORCEINLINE const TCHAR* EnumToTCharString<EMqttifyPacketType>(const EMqttifyPacketType InValue)
	{
		switch (InValue)
		{
			case EMqttifyPacketType::Connect:
				return TEXT("Connect");
			case EMqttifyPacketType::ConnAck:
				return TEXT("ConnAck");
			case EMqttifyPacketType::Publish:
				return TEXT("Publish");
			case EMqttifyPacketType::PubAck:
				return TEXT("PubAck");
			case EMqttifyPacketType::PubRec:
				return TEXT("PubRec");
			case EMqttifyPacketType::PubRel:
				return TEXT("PubRel");
			case EMqttifyPacketType::PubComp:
				return TEXT("PubComp");
			case EMqttifyPacketType::Subscribe:
				return TEXT("Subscribe");
			case EMqttifyPacketType::SubAck:
				return TEXT("SubAck");
			case EMqttifyPacketType::Unsubscribe:
				return TEXT("Unsubscribe");
			case EMqttifyPacketType::UnsubAck:
				return TEXT("UnsubAck");
			case EMqttifyPacketType::PingReq:
				return TEXT("PingReq");
			case EMqttifyPacketType::PingResp:
				return TEXT("PingResp");
			case EMqttifyPacketType::Disconnect:
				return TEXT("Disconnect");
			case EMqttifyPacketType::Auth:
				return TEXT("Auth");
			case EMqttifyPacketType::None:
			case EMqttifyPacketType::Max:
			default:
				return TEXT("Unknown");
		}
	}

	namespace MqttifyPacketType
	{
		constexpr TCHAR InvalidPacketType[] = TEXT("Invalid packet type");
	}
} // namespace Mqttify