#pragma once

#include "MqttifyEnumToString.h"

namespace Mqttify
{
	/**
 * @enum EMqttifyPropertyIdentifier
 * @brief Enum representing MQTT property identifiers.
 * https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901027
 */
	enum class EMqttifyPropertyIdentifier : uint8
	{
		/// @brief Represents an unknown or unspecified MQTT property.
		Unknown = 0,
		/// @brief Indicates the format of the MQTT payload.
		PayloadFormatIndicator = 1,
		/// @brief Specifies the time after which the message should expire.
		MessageExpiryInterval = 2,
		/// @brief Describes the type of content in the payload.
		ContentType = 3,
		/// @brief Topic where response messages should be sent.
		ResponseTopic = 8,
		/// @brief Data used to correlate response messages with request messages.
		CorrelationData = 9,
		/// @brief Identifier that specifies the subscription.
		SubscriptionIdentifier = 11,
		/// @brief Time after which the MQTT session should expire.
		SessionExpiryInterval = 17,
		/// @brief Client identifier assigned by the server.
		AssignedClientIdentifier = 18,
		/// @brief Keep-alive time specified by the server.
		ServerKeepAlive = 19,
		/// @brief Method used for authentication.
		AuthenticationMethod = 21,
		/// @brief Data used for authentication.
		AuthenticationData = 22,
		/// @brief Flag to request problem information from the server.
		RequestProblemInformation = 23,
		/// @brief Interval before the Will message is sent.
		WillDelayInterval = 24,
		/// @brief Flag to request response information from the server.
		RequestResponseInformation = 25,
		/// @brief Response information from the server.
		ResponseInformation = 26,
		/// @brief Reference to another server where the client can connect.
		ServerReference = 28,
		/// @brief Reason string to provide more information about the operation.
		ReasonString = 31,
		/// @brief Specifies the maximum number of messages that can be received.
		ReceiveMaximum = 33,
		/// @brief Maximum number of topic aliases that can be used.
		TopicAliasMaximum = 34,
		/// @brief Topic alias to be used instead of the topic name.
		TopicAlias = 35,
		/// @brief The maximum QoS level the server supports.
		MaximumQoS = 36,
		/// @brief Indicates if the server supports retained messages.
		RetainAvailable = 37,
		/// @brief General-purpose user property for the message.
		UserProperty = 38,
		/// @brief Specifies the maximum packet size.
		MaximumPacketSize = 39,
		/// @brief Indicates if wildcard subscriptions are available.
		WildcardSubscriptionAvailable = 40,
		/// @brief Indicates if the subscription identifier is available.
		SubscriptionIdentifierAvailable = 41,
		/// @brief Indicates if shared subscriptions are available.
		SharedSubscriptionAvailable = 42,
		/// @brief Max enum size, used for validation.
		Max = 43
	};

	namespace MqttifyPropertyIdentifier
	{
		inline bool IsValid(const uint8 Value)
		{
			switch (Value)
			{
				case static_cast<uint8>(EMqttifyPropertyIdentifier::PayloadFormatIndicator):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::MessageExpiryInterval):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::ContentType):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::ResponseTopic):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::CorrelationData):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::SubscriptionIdentifier):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::SessionExpiryInterval):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::AssignedClientIdentifier):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::ServerKeepAlive):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::AuthenticationMethod):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::AuthenticationData):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::RequestProblemInformation):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::WillDelayInterval):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::RequestResponseInformation):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::ResponseInformation):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::ServerReference):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::ReasonString):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::ReceiveMaximum):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::TopicAliasMaximum):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::TopicAlias):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::MaximumQoS):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::RetainAvailable):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::UserProperty):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::MaximumPacketSize):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::WildcardSubscriptionAvailable):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::SubscriptionIdentifierAvailable):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::SharedSubscriptionAvailable):
					return true;
				case static_cast<uint8>(EMqttifyPropertyIdentifier::Unknown):
				case static_cast<uint8>(EMqttifyPropertyIdentifier::Max):
				default:
					return false;
			}
		}
	} // MqttifyPropertyIdentifier

	template <>
	FORCEINLINE const TCHAR* EnumToTCharString<EMqttifyPropertyIdentifier>(const EMqttifyPropertyIdentifier InValue)
	{
		switch (InValue)
		{
			case EMqttifyPropertyIdentifier::Unknown:
				return TEXT("Unknown");
			case EMqttifyPropertyIdentifier::PayloadFormatIndicator:
				return TEXT("PayloadFormatIndicator");
			case EMqttifyPropertyIdentifier::MessageExpiryInterval:
				return TEXT("MessageExpiryInterval");
			case EMqttifyPropertyIdentifier::ContentType:
				return TEXT("ContentType");
			case EMqttifyPropertyIdentifier::ResponseTopic:
				return TEXT("ResponseTopic");
			case EMqttifyPropertyIdentifier::CorrelationData:
				return TEXT("CorrelationData");
			case EMqttifyPropertyIdentifier::SubscriptionIdentifier:
				return TEXT("SubscriptionIdentifier");
			case EMqttifyPropertyIdentifier::SessionExpiryInterval:
				return TEXT("SessionExpiryInterval");
			case EMqttifyPropertyIdentifier::AssignedClientIdentifier:
				return TEXT("AssignedClientIdentifier");
			case EMqttifyPropertyIdentifier::ServerKeepAlive:
				return TEXT("ServerKeepAlive");
			case EMqttifyPropertyIdentifier::AuthenticationMethod:
				return TEXT("AuthenticationMethod");
			case EMqttifyPropertyIdentifier::AuthenticationData:
				return TEXT("AuthenticationData");
			case EMqttifyPropertyIdentifier::RequestProblemInformation:
				return TEXT("RequestProblemInformation");
			case EMqttifyPropertyIdentifier::WillDelayInterval:
				return TEXT("WillDelayInterval");
			case EMqttifyPropertyIdentifier::RequestResponseInformation:
				return TEXT("RequestResponseInformation");
			case EMqttifyPropertyIdentifier::ResponseInformation:
				return TEXT("ResponseInformation");
			case EMqttifyPropertyIdentifier::ServerReference:
				return TEXT("ServerReference");
			case EMqttifyPropertyIdentifier::ReasonString:
				return TEXT("ReasonString");
			case EMqttifyPropertyIdentifier::ReceiveMaximum:
				return TEXT("ReceiveMaximum");
			case EMqttifyPropertyIdentifier::TopicAliasMaximum:
				return TEXT("TopicAliasMaximum");
			case EMqttifyPropertyIdentifier::TopicAlias:
				return TEXT("TopicAlias");
			case EMqttifyPropertyIdentifier::MaximumQoS:
				return TEXT("MaximumQoS");
			case EMqttifyPropertyIdentifier::RetainAvailable:
				return TEXT("RetainAvailable");
			case EMqttifyPropertyIdentifier::UserProperty:
				return TEXT("UserProperty");
			case EMqttifyPropertyIdentifier::MaximumPacketSize:
				return TEXT("MaximumPacketSize");
			case EMqttifyPropertyIdentifier::WildcardSubscriptionAvailable:
				return TEXT("WildcardSubscriptionAvailable");
			case EMqttifyPropertyIdentifier::SubscriptionIdentifierAvailable:
				return TEXT("SubscriptionIdAvailable");
			case EMqttifyPropertyIdentifier::SharedSubscriptionAvailable:
				return TEXT("SharedSubscriptionAvailable");
			case EMqttifyPropertyIdentifier::Max:
			default:
				return TEXT("Invalid");
		}
	}

} // namespace Mqttify