#pragma once

#include "Mqtt/MqttifyConnectionSettings.h"

class MQTTIFY_API FMqttifyConnectionSettingsBuilder final
{
private:
	FString Url;
	// Default values
	uint16 PacketRetryIntervalSeconds           = 5.0f;
	uint16 SocketConnectionTimeoutSeconds       = 10;
	uint16 KeepAliveIntervalSeconds             = 60;
	uint16 MqttConnectionTimeoutSeconds         = 10;
	uint8 MaxConnectionRetries                  = 5;
	uint8 MaxPacketRetries                      = 5;
	uint16 InitialRetryIntervalSeconds          = 3.0f;
	EMqttifyProtocolVersion MqttProtocolVersion = EMqttifyProtocolVersion::Mqtt_5;
	EMqttifyThreadMode ThreadMode               = EMqttifyThreadMode::BackgroundThreadWithCallbackMarshalling;

public:
	/**
	 * @brief Construct from URL.
	 * @param InUrl The URL to connect to.
	 */
	explicit FMqttifyConnectionSettingsBuilder(const FString& InUrl)
		: Url(InUrl) {}

	/**
	 * @brief Sets the socket connection timeout in seconds.
	 * @param InSeconds The socket connection timeout in seconds.
	 * @return A reference to this builder.
	 */
	FMqttifyConnectionSettingsBuilder& SetSocketConnectionTimeoutSeconds(const uint16 InSeconds)
	{
		SocketConnectionTimeoutSeconds = InSeconds;
		return *this;
	}

	/**
	 * @brief Sets the MQTT keep alive interval in seconds.
	 * @param InSeconds The MQTT keep alive interval in seconds.
	 * @return A reference to this builder.
	 */
	FMqttifyConnectionSettingsBuilder& SetKeepAliveIntervalSeconds(const uint16 InSeconds)
	{
		KeepAliveIntervalSeconds = InSeconds;
		return *this;
	}

	/**
	 * @brief Sets the MQTT connection timeout in seconds.
	 * @param InSeconds The MQTT connection timeout in seconds.
	 * @return A reference to this builder.
	 */
	FMqttifyConnectionSettingsBuilder& SetMqttConnectionTimeoutSeconds(const uint16 InSeconds)
	{
		MqttConnectionTimeoutSeconds = InSeconds;
		return *this;
	}

	/**
	 * @brief Sets the maximum number of connection retries.
	 * @param InRetries The maximum number of connection retries.
	 * @return A reference to this builder.
	 */
	FMqttifyConnectionSettingsBuilder& SetMaxConnectionRetries(const uint8 InRetries)
	{
		MaxConnectionRetries = InRetries;
		return *this;
	}

	/**
	 * @brief Sets the initial retry interval in seconds.
	 * @param InInterval The initial retry interval in seconds.
	 * @return A reference to this builder.
	 */
	FMqttifyConnectionSettingsBuilder& SetInitialRetryInterval(const uint16 InInterval)
	{
		InitialRetryIntervalSeconds = InInterval;
		return *this;
	}

	/**
	 * @brief Sets the thread mode to use for the connection.
	 * @param InThreadMode The thread mode to use for the connection.
	 * @return A reference to this builder.
	 */
	FMqttifyConnectionSettingsBuilder& SetThreadMode(const EMqttifyThreadMode InThreadMode)
	{
		ThreadMode = InThreadMode;
		return *this;
	}

	/**
	 * @brief Sets the packet retry interval in seconds.
	 * @param InSeconds The packet retry interval in seconds.
	 * @return A reference to this builder.
	 */
	FMqttifyConnectionSettingsBuilder& SetPacketRetryIntervalSeconds(const uint16 InSeconds)
	{
		PacketRetryIntervalSeconds = InSeconds;
		return *this;
	}

	/**
	 * @brief Sets the maximum number of packet retries.
	 * @param InRetries The maximum number of packet retries.
	 * @return A reference to this builder.
	 */
	FMqttifyConnectionSettingsBuilder& SetMaxPacketRetries(const uint8 InRetries)
	{
		MaxPacketRetries = InRetries;
		return *this;
	}

	/**
	 * @brief Sets the MQTT protocol version.
	 * @param InMqttProtocolVersion The MQTT protocol version.
	 * @return A reference to this builder.
	 */
	FMqttifyConnectionSettingsBuilder& SetMqttProtocolVersion(const EMqttifyProtocolVersion InMqttProtocolVersion)
	{
		MqttProtocolVersion = InMqttProtocolVersion;
		return *this;
	}

	/**
	 * @brief Builds the connection settings.
	 * @return The connection settings.
	 */
	TSharedPtr<FMqttifyConnectionSettings> Build() const
	{
		return FMqttifyConnectionSettings::CreateShared(Url,
														MqttProtocolVersion,
														PacketRetryIntervalSeconds,
														SocketConnectionTimeoutSeconds,
														KeepAliveIntervalSeconds,
														MqttConnectionTimeoutSeconds,
														InitialRetryIntervalSeconds,
														MaxConnectionRetries,
														MaxPacketRetries,
														ThreadMode);
	}
};
