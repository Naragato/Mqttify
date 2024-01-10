#pragma once

#include "CoreMinimal.h"
#include "MqttifyEnumToString.h"
#include "MqttifyReasonCode.generated.h"

/**
 * @brief EMqttReasonCode
 * Reason Codes as per MQTT v5.0 specification
 * Reference: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901031
 */
UENUM(BlueprintType)
enum class EMqttifyReasonCode : uint8
{
	/**
	 * @brief 0x00 Success
	 * (CONNACK, PUBACK, PUBREC, PUBREL, PUBCOMP, UNSUBACK,
	 * AUTH, DISCONNECT, AUTH, SUBACK, UNSUBACK, PUBACK, PUBREC,
	 * PUBREL, PUBCOMP, DISCONNECT, AUTH, DISCO)
	 */
	Success = 0,

	/// @brief 0x00 Normal disconnection (DISCONNECT)
	NormalDisconnection = 0,

	/// @brief 0x00 Granted Quality Of Service 0 (SUBACK)
	GrantedQualityOfService0 = 0,

	/// @brief 0x01 Granted Quality Of Service 1 (SUBACK)
	GrantedQualityOfService1 = 1,

	/// @brief 0x02 Granted Quality Of Service 2 (SUBACK)
	GrantedQualityOfService2 = 2,

	/// @brief 0x00 Disconnect with Will message (DISCONNECT)
	DisconnectWithWillMessage = 4,

	/// @brief 0x00 No matching subscribers (PUBACK, PUBREC)
	NoMatchingSubscribers = 16,

	/// @brief 0x10 No subscription existed (UNSUBACK)
	NoSubscriptionExisted = 17,

	/// @brief 0x11 Continue authentication (AUTH)
	ContinueAuthentication = 24,

	/// @brief 0x18 Re-authenticate (AUTH)
	ReAuthenticate = 25,

	/**
	 * @brief 0x80 Unspecified error
	 * (CONNACK, PUBACK, PUBREC, SUBACK, UNSUBACK, DISCONNECT)
	 */
	UnspecifiedError = 128,

	/// @brief 0x81 Malformed Packet (CONNACK, DISCONNECT)
	MalformedPacket = 129,

	/// @brief 0x82 Protocol Error (CONNACK, DISCONNECT)
	ProtocolError = 130,

	/**
	 * @brief 0x83 Implementation specific error
	 * (CONNACK, PUBACK, PUBREC, SUBACK, UNSUBACK, DISCONNECT)
	 */
	ImplementationSpecificError = 131,

	/// @brief 0x84 Unsupported protocol version (CONNACK)
	UnsupportedProtocolVersion = 132,

	/// @brief 0x85 Client identifier not valid (CONNACK)
	ClientIdentifierNotValid = 133,

	/// @brief 0x86 Bad user name or password (CONNACK)
	BadUsernameOrPassword = 134,

	/**
	 * @brief 0x87 Not authorized
	 * (CONNACK, PUBACK, PUBREC, SUBACK, UNSUBACK, DISCONNECT)
	 */
	NotAuthorized = 135,

	/// @brief 0x88 Server unavailable (CONNACK)
	ServerUnavailable = 136,

	/// @brief 0x89 Server busy (CONNACK, DISCONNECT)
	ServerBusy = 137,

	/// @brief 0x8A Banned (CONNACK)
	Banned = 138,

	/// @brief 0x8B Server shutting down (DISCONNECT)
	ServerShuttingDown = 139,

	/// @brief 0x8C Bad authentication method (CONNACK, DISCONNECT)
	BadAuthenticationMethod = 140,

	/// @brief 0x8D Keep alive timeout (DISCONNECT)
	KeepAliveTimeout = 141,

	/// @brief 0x8E Session taken over (DISCONNECT)
	SessionTakenOver = 142,

	/**
	 * @brief 0x8F Topic filter invalid
	 * (SUBACK, UNSUBACK, DISCONNECT)
	 */
	TopicFilterInvalid = 143,

	/**
	 * @brief 0x90 Topic name invalid
	 * (CONNACK, PUBACK, PUBREC, DISCONNECT)
	 */
	TopicNameInvalid = 144,

	/// @brief 0x91 Packet identifier in use (PUBACK, PUBREC, SUBACK, UNSUBACK)
	PacketIdentifierInUse = 145,

	/// @brief 0x92 Packet identifier not found (PUBREL, PUBCOMP)
	PacketIdentifierNotFound = 146,

	/// @brief 0x93 Receive maximum exceeded (DISCONNECT)
	ReceiveMaximumExceeded = 147,

	/// @brief 0x94 Topic alias invalid (DISCONNECT)
	TopicAliasInvalid = 148,

	/// @brief 0x95 Packet too large (CONNACK, DISCONNECT)
	PacketTooLarge = 149,

	/// @brief 0x96 Message rate too high (DISCONNECT)
	MessageRateTooHigh = 150,

	/// @brief 0x97 Quota exceeded (CONNACK, PUBACK, PUBREC, SUBACK, DISCONNECT)
	QuotaExceeded = 151,

	/// @brief 0x98 Administrative action (DISCONNECT)
	AdministrativeAction = 152,

	/// @brief 0x99 Payload format invalid (CONNACK, PUBACK, PUBREC, DISCONNECT)
	PayloadFormatInvalid = 153,

	/// @brief 0x9A Retain not supported (CONNACK, DISCONNECT)
	RetainNotSupported = 154,

	/// @brief 0x9B QoS not supported (CONNACK, DISCONNECT)
	QoSNotSupported = 155,

	/// @brief 0x9C Use another server (CONNACK, DISCONNECT)
	UseAnotherServer = 156,

	/// @brief 0x9D Server moved (CONNACK, DISCONNECT)
	ServerMoved = 157,

	/// @brief 0x9E Shared subscription not supported (SUBACK, DISCONNECT
	SharedSubscriptionsNotSupported = 158,

	/// @brief 0x9F Connection rate exceeded (CONNACK, DISCONNECT)
	ConnectionRateExceeded = 159,

	/// @brief 0xA0 Maximum connect time (DISCONNECT)
	MaximumConnectTime = 160,

	/// @brief 0xA1 Subscription identifiers not supported (SUBACK, DISCONNECT)
	SubscriptionIdentifiersNotSupported = 161,

	/// @brief 0xA2 Wildcard subscription not supported (SUBACK, DISCONNECT)
	WildcardSubscriptionsNotSupported = 162
};

namespace MqttifyReasonCode
{
	constexpr TCHAR InvalidReasonCode[] = TEXT("Invalid Reason code.");
}

namespace Mqttify
{
	template <>
	FORCEINLINE const TCHAR* EnumToTCharString<EMqttifyReasonCode>(const EMqttifyReasonCode InValue)
	{
		switch (InValue)
		{
			case EMqttifyReasonCode::Success:
				return TEXT("Success|Normal Disconnection|Granted Quality Of Service 0");
			case EMqttifyReasonCode::GrantedQualityOfService1:
				return TEXT("Granted Quality Of Service 1");
			case EMqttifyReasonCode::GrantedQualityOfService2:
				return TEXT("Granted Quality Of Service 2");
			case EMqttifyReasonCode::DisconnectWithWillMessage:
				return TEXT("Disconnect with Will message");
			case EMqttifyReasonCode::NoMatchingSubscribers:
				return TEXT("No matching subscribers");
			case EMqttifyReasonCode::NoSubscriptionExisted:
				return TEXT("No subscription existed");
			case EMqttifyReasonCode::ContinueAuthentication:
				return TEXT("Continue authentication");
			case EMqttifyReasonCode::ReAuthenticate:
				return TEXT("Re-authenticate");
			case EMqttifyReasonCode::UnspecifiedError:
				return TEXT("Unspecified error");
			case EMqttifyReasonCode::MalformedPacket:
				return TEXT("Malformed Packet");
			case EMqttifyReasonCode::ProtocolError:
				return TEXT("Protocol Error");
			case EMqttifyReasonCode::ImplementationSpecificError:
				return TEXT("Implementation specific error");
			case EMqttifyReasonCode::UnsupportedProtocolVersion:
				return TEXT("Unsupported protocol version");
			case EMqttifyReasonCode::ClientIdentifierNotValid:
				return TEXT("Client identifier not valid");
			case EMqttifyReasonCode::BadUsernameOrPassword:
				return TEXT("Bad user name or password");
			case EMqttifyReasonCode::NotAuthorized:
				return TEXT("Not authorized");
			case EMqttifyReasonCode::ServerUnavailable:
				return TEXT("Server unavailable");
			case EMqttifyReasonCode::ServerBusy:
				return TEXT("Server busy");
			case EMqttifyReasonCode::Banned:
				return TEXT("Banned");
			case EMqttifyReasonCode::ServerShuttingDown:
				return TEXT("Server shutting down");
			case EMqttifyReasonCode::BadAuthenticationMethod:
				return TEXT("Bad authentication method");
			case EMqttifyReasonCode::KeepAliveTimeout:
				return TEXT("Keep alive timeout");
			case EMqttifyReasonCode::SessionTakenOver:
				return TEXT("Session taken over");
			case EMqttifyReasonCode::TopicFilterInvalid:
				return TEXT("Topic filter invalid");
			case EMqttifyReasonCode::TopicNameInvalid:
				return TEXT("Topic name invalid");
			case EMqttifyReasonCode::PacketIdentifierInUse:
				return TEXT("Packet identifier in use");
			case EMqttifyReasonCode::PacketIdentifierNotFound:
				return TEXT("Packet identifier not found");
			case EMqttifyReasonCode::ReceiveMaximumExceeded:
				return TEXT("Receive maximum exceeded");
			case EMqttifyReasonCode::TopicAliasInvalid:
				return TEXT("Topic alias invalid");
			case EMqttifyReasonCode::PacketTooLarge:
				return TEXT("Packet too large");
			case EMqttifyReasonCode::MessageRateTooHigh:
				return TEXT("Message rate too high");
			case EMqttifyReasonCode::QuotaExceeded:
				return TEXT("Quota exceeded");
			case EMqttifyReasonCode::AdministrativeAction:
				return TEXT("Administrative action");
			case EMqttifyReasonCode::PayloadFormatInvalid:
				return TEXT("Payload format invalid");
			case EMqttifyReasonCode::RetainNotSupported:
				return TEXT("Retain not supported");
			case EMqttifyReasonCode::QoSNotSupported:
				return TEXT("QoS not supported");
			case EMqttifyReasonCode::UseAnotherServer:
				return TEXT("Use another server");
			case EMqttifyReasonCode::ServerMoved:
				return TEXT("Server moved");
			case EMqttifyReasonCode::SharedSubscriptionsNotSupported:
				return TEXT("Shared subscription not supported");
			case EMqttifyReasonCode::ConnectionRateExceeded:
				return TEXT("Connection rate exceeded");
			case EMqttifyReasonCode::MaximumConnectTime:
				return TEXT("Maximum connect time");
			case EMqttifyReasonCode::SubscriptionIdentifiersNotSupported:
				return TEXT("Subscription identifiers not supported");
			case EMqttifyReasonCode::WildcardSubscriptionsNotSupported:
				return TEXT("Wildcard subscription not supported");
			default:
				return TEXT("Unknown");
		}
	}
} // namespace Mqttify
