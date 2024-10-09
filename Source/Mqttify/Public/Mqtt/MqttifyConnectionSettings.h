#pragma once

#include "MqttifyProtocolVersion.h"
#include "MqttifyThreadMode.h"
#include "Interface/IMqttifyCredentialsProvider.h"
#include "Misc/Base64.h"
#include "Mqtt/MqttifyConnectionProtocol.h"

enum class EMqttifyProtocolVersion : uint8;

using FMqttifyConnectionSettingsRef = TSharedRef<class FMqttifyConnectionSettings>;
/**
 * @brief Represents a structured MQTT URL.
 * Handles URL formation as: mqtt[s]://[username][:password]@host.domain[:port]
 */
class MQTTIFY_API FMqttifyConnectionSettings final
{
private:
	/// @brief Unique Id for this game instance / credentials.
	FString ClientId{};

	/// @brief Port number for the connection. Default port is 1883 for MQTT and 8883 for MQTTS.
	int16 Port;

	/// @brief MQTT Host name or IP. Defaults to "localhost".
	FString Host{};

	/// @brief Protocol to use for MQTT connection.
	EMqttifyConnectionProtocol ConnectionProtocol;

	/// @brief Provider to resolve Credentials used by the connection.
	FMqttifyCredentialsProviderPtr CredentialsProvider = nullptr;

	/// @brief Path for url of the connection.
	FString Path{};

	/// @brief Min seconds to wait before retrying sending a packet.
	uint16 PacketRetryIntervalSeconds;
	/// @brief Initial seconds to wait before retrying a connection.
	uint16 InitialRetryConnectionIntervalSeconds;
	/// @brief The time to wait before giving up on a connection attempt.
	uint16 SocketConnectionTimeoutSeconds;
	/// @brief The time to wait before sending a keep alive packet.
	uint16 KeepAliveIntervalSeconds;
	/// @brief The time to wait before giving up on a connection attempt.
	uint16 MqttConnectionTimeoutSeconds;
	/// @brief Number of times to retry a connection. Before giving up.
	uint8 MaxConnectionRetries;
	/// @brief Number of times to retry sending packet before giving up.
	uint8 MaxPacketRetries;
	/// @breif Verify the server certificate.
	bool bShouldVerifyServerCertificate;

public:
	/// @brief Copy constructor.
	FMqttifyConnectionSettings(const FMqttifyConnectionSettings& Other)
	{
		ClientId = Other.ClientId;
		Port = Other.Port;
		Host = Other.Host;
		CredentialsProvider = Other.CredentialsProvider;
		ConnectionProtocol = Other.ConnectionProtocol;
		Path = Other.Path;
		PacketRetryIntervalSeconds = Other.PacketRetryIntervalSeconds;
		SocketConnectionTimeoutSeconds = Other.SocketConnectionTimeoutSeconds;
		KeepAliveIntervalSeconds = Other.KeepAliveIntervalSeconds;
		MqttConnectionTimeoutSeconds = Other.MqttConnectionTimeoutSeconds;
		InitialRetryConnectionIntervalSeconds = Other.InitialRetryConnectionIntervalSeconds;
		MaxConnectionRetries = Other.MaxConnectionRetries;
		MaxPacketRetries = Other.MaxPacketRetries;
		bShouldVerifyServerCertificate = Other.bShouldVerifyServerCertificate;
	}

	FMqttifyConnectionSettings& operator=(const FMqttifyConnectionSettings&)
	{
		return *this;
	}

	/// @brief Get the transport protocol used for the connection.
	EMqttifyConnectionProtocol GetTransportProtocol() const { return ConnectionProtocol; }

	/// @brief Returns the host URL for the connection.
	const FString& GetHost() const { return Host; }

	/// @brief Returns the credentials used by connection.
	FMqttifyCredentialsProviderRef GetCredentialsProvider() const { return CredentialsProvider.ToSharedRef(); }

	/// @brief Get Path for url of the connection.
	const FString& GetPath() const { return Path; }

	/// @brief Get the port number for the connection.
	int32 GetPort() const { return Port; }

	/// @brief Returns the socket connection timeout.
	uint16 GetSocketConnectionTimeoutSeconds() const { return SocketConnectionTimeoutSeconds; }

	/// @brief Returns the keep alive interval.
	uint16 GetKeepAliveIntervalSeconds() const { return KeepAliveIntervalSeconds; }

	/// @brief Returns the MQTT connection timeout.
	uint16 GetMqttConnectionTimeoutSeconds() const { return MqttConnectionTimeoutSeconds; }

	/// @brief Returns the packet retry interval.
	uint16 GetPacketRetryIntervalSeconds() const { return PacketRetryIntervalSeconds; }

	/// @brief Returns the Max Connection Retries.
	uint8 GetMaxConnectionRetries() const { return MaxConnectionRetries; }

	/// @brief Returns the Max Packet Retries.
	uint8 GetMaxPacketRetries() const { return MaxPacketRetries; }

	/// @brief Whether to verify the server certificate.
	bool ShouldVerifyServerCertificate() const { return bShouldVerifyServerCertificate; }

	/**
	 * @brief Generates a deterministic ClientId based on the connection settings.
	 * We're using the host, path and username to generate a unique id.
	 * Allowing the user of the client to update the password without it changing
	 * @return A ClientId.
	 */
	FString GetClientId() const;

	/**
	 * @brief Generates a hash code based on the connection settings.
	 * We ignore the password as we want to use the same Hash if the password changes
	 * @return The hash code for the connection settings.
	 */
	uint32 GetHashCode() const;

	/**
	 * @brief Parses the input string to populate the struct.
	 * @return The connection settings.
	 */
	static TSharedPtr<FMqttifyConnectionSettings> CreateShared(
		const FString& InURL,
		uint16 InPacketRetryIntervalSeconds,
		uint16 InSocketConnectionTimeoutSeconds,
		uint16 InKeepAliveIntervalSeconds,
		uint16 InMqttConnectionTimeoutSeconds,
		uint16 InInitialRetryIntervalSeconds,
		uint8 InMaxConnectionRetries,
		uint8 InMaxPacketRetries,
		bool bInShouldVerifyCertificate
		);

	static TSharedPtr<FMqttifyConnectionSettings> CreateShared(
		const FString& InURL,
		const TSharedRef<IMqttifyCredentialsProvider>& CredentialsProvider,
		const uint16 InPacketRetryIntervalSeconds,
		const uint16 InSocketConnectionTimeoutSeconds,
		const uint16 InKeepAliveIntervalSeconds,
		const uint16 InMqttConnectionTimeoutSeconds,
		const uint16 InInitialRetryIntervalSeconds,
		const uint8 InMaxConnectionRetries,
		const uint8 InMaxPacketRetries,
		const bool bInShouldVerifyCertificate
		);

	// Helper function to create FMqttifyConnectionSettings
	static TSharedPtr<FMqttifyConnectionSettings> CreateSharedInternal(
		FString&& Host,
		FString&& Path,
		const EMqttifyConnectionProtocol Protocol,
		const int32 Port,
		const TSharedRef<IMqttifyCredentialsProvider>& CredentialsProvider,
		const uint16 PacketRetryIntervalSeconds,
		const uint16 SocketConnectionTimeoutSeconds,
		const uint16 KeepAliveIntervalSeconds,
		const uint16 MqttConnectionTimeoutSeconds,
		const uint16 InitialRetryIntervalSeconds,
		const uint8 MaxConnectionRetries,
		const uint8 MaxPacketRetries,
		const bool bShouldVerifyServerCertificate
		)
	{
		return MakeShareable(
			new FMqttifyConnectionSettings(
				MoveTemp(Host),
				MoveTemp(Path),
				Protocol,
				Port,
				CredentialsProvider,
				PacketRetryIntervalSeconds,
				SocketConnectionTimeoutSeconds,
				KeepAliveIntervalSeconds,
				MqttConnectionTimeoutSeconds,
				InitialRetryIntervalSeconds,
				MaxConnectionRetries,
				MaxPacketRetries,
				bShouldVerifyServerCertificate));
	}

	/**
	 * @brief Converts the connection settings to a string format.
	 * @return The connection settings as a string. For logging.
	 */
	FString ToString() const;
	FString ToConnectionString() const;

private:
	FMqttifyConnectionSettings() = delete;

	/**
	 * @brief Creates a new instance of the connection settings.
	 * @param InConnectionProtocol The protocol to use for the connection. WSS/WS/MQTTS/MQTT
	 * @param InPort The port to connect to.
	 * @param InHost The host to connect to.
	 * @param InCredentialsProvider The credential provider to use for authentication.
	 * @param InPath The Uri path to use for the connection.
	 * @param InPacketRetryIntervalSeconds The packet retry interval in seconds.
	 * @param InSocketConnectionTimeoutSeconds The socket connection timeout in seconds.
	 * @param InKeepAliveIntervalSeconds The MQTT keep alive interval in seconds.
	 * @param InMqttConnectionTimeoutSeconds The MQTT connection timeout in seconds.
	 * @param InInitialRetryIntervalSeconds The initial retry interval in seconds.
	 * @param InMaxConnectionRetries The maximum number of connection retries.
	 * @param InMaxPacketRetries The maximum number time to retry sending a packet.
	 * @param bInShouldVerifyCertificate Whether to verify the server certificate.
	 * @param InClientId The ClientId to use for the connection.
	 */
	explicit FMqttifyConnectionSettings(
		FString&& InHost,
		FString&& InPath,
		const EMqttifyConnectionProtocol InConnectionProtocol,
		const int16 InPort,
		const FMqttifyCredentialsProviderRef& InCredentialsProvider,
		const uint16 InPacketRetryIntervalSeconds,
		const uint16 InSocketConnectionTimeoutSeconds,
		const uint16 InKeepAliveIntervalSeconds,
		const uint16 InMqttConnectionTimeoutSeconds,
		const uint16 InInitialRetryIntervalSeconds,
		const uint8 InMaxConnectionRetries,
		const uint8 InMaxPacketRetries,
		const bool bInShouldVerifyCertificate,
		FString&& InClientId = TEXT("")
		);

	/// @brief Helper function to parse protocol
	static FORCEINLINE EMqttifyConnectionProtocol ParseProtocol(const FString& Scheme);

	/// @brief Helper function to determine default port based on protocol
	static FORCEINLINE int32 DefaultPort(EMqttifyConnectionProtocol Protocol);

	/**
	 * @brief Adds the given bytes to the given array.
	 * @param Bytes The array to add the bytes to.
	 * @param Src The bytes to add.
	 * @param Size The size of the bytes to add.
	 */
	static FORCEINLINE void AddToBytes(TArray<uint8>& Bytes, const void* Src, const int32 Size);

	/**
	 * @brief Generates a deterministic ClientId based on the connection settings.
	 * We're using the connection settings fields excluding the password to generate a unique id.
	 * Allowing the user of the client to update the password without it changing
	 * @return A ClientId.
	 */
	FORCEINLINE FString GenerateClientId() const;
};

FORCEINLINE uint32 GetTypeHash(const FMqttifyConnectionSettings& ConnectionSettings)
{
	return ConnectionSettings.GetHashCode();
}
