#pragma once

#include "MqttifyProtocolVersion.h"
#include "MqttifyThreadMode.h"
#include "Misc/Base64.h"
#include "Misc/Fnv.h"
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
	FString ClientId;

	/// @brief Port number for the connection. Default port is 1883 for MQTT and 8883 for MQTTS.
	int16 Port;

	/// @brief MQTT Host name or IP. Defaults to "localhost".
	FString Host;

	/// @brief Username for MQTT authentication.
	FString Username;

	/// @brief Password for MQTT authentication. Only used if a username is provided.
	FString Password;

	/// @brief Protocol to use for MQTT connection.
	EMqttifyConnectionProtocol ConnectionProtocol;

	/// @brief Path for url of the connection.
	FString Path;

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

public:
	/// @brief Copy constructor.
	FMqttifyConnectionSettings(const FMqttifyConnectionSettings& Other)
	{
		ClientId                              = Other.ClientId;
		Port                                  = Other.Port;
		Host                                  = Other.Host;
		Username                              = Other.Username;
		Password                              = Other.Password;
		ConnectionProtocol                    = Other.ConnectionProtocol;
		Path                                  = Other.Path;
		PacketRetryIntervalSeconds            = Other.PacketRetryIntervalSeconds;
		SocketConnectionTimeoutSeconds        = Other.SocketConnectionTimeoutSeconds;
		KeepAliveIntervalSeconds              = Other.KeepAliveIntervalSeconds;
		MqttConnectionTimeoutSeconds          = Other.MqttConnectionTimeoutSeconds;
		InitialRetryConnectionIntervalSeconds = Other.InitialRetryConnectionIntervalSeconds;
		MaxConnectionRetries                  = Other.MaxConnectionRetries;
		MaxPacketRetries                      = Other.MaxPacketRetries;
	}

	FMqttifyConnectionSettings& operator=(const FMqttifyConnectionSettings&)
	{
		return *this;
	}

	/// @brief Get the transport protocol used for the connection.
	EMqttifyConnectionProtocol GetTransportProtocol() const { return ConnectionProtocol; }

	/// @brief Get's the host URL for the connection.
	const FString& GetHost() const { return Host; }

	/// @brief Get's the username for the connection.
	const FString& GetUsername() const { return Username; }

	/// @brief Get's the password for the connection.
	const FString& GetPassword() const { return Password; }

	/// @brief Get Path for url of the connection.
	const FString& GetPath() const { return Path; }

	/// @brief Get the port number for the connection.
	int32 GetPort() const { return Port; }

	/// @brief Get's the socket connection timeout.
	uint16 GetSocketConnectionTimoutSeconds() const { return SocketConnectionTimeoutSeconds; }

	/// @brief Get's the keep alive interval.
	uint16 GetKeepAliveIntervalSeconds() const { return KeepAliveIntervalSeconds; }

	/// @brief Get's the MQTT connection timeout.
	uint16 GetMqttConnectionTimeoutSeconds() const { return MqttConnectionTimeoutSeconds; }

	/// @brief Get's the packet retry interval.
	uint16 GetPacketRetryIntervalSeconds() const { return PacketRetryIntervalSeconds; }

	/// @brief Get's the Max Connection Retries.
	uint8 GetMaxConnectionRetries() const { return MaxConnectionRetries; }

	/// @brief Get's the Max Packet Retries.
	uint8 GetMaxPacketRetries() const { return MaxPacketRetries; }

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
	static TSharedPtr<FMqttifyConnectionSettings> CreateShared(const FString& InURL,
																EMqttifyProtocolVersion InMqttProtocolVersion,
																uint16 InPacketRetryIntervalSeconds,
																uint16 InSocketConnectionTimeoutSeconds,
																uint16 InKeepAliveIntervalSeconds,
																uint16 InMqttConnectionTimeoutSeconds,
																uint16 InInitialRetryIntervalSeconds,
																uint8 InMaxConnectionRetries,
																uint8 InMaxPacketRetries,
																EMqttifyThreadMode InThreadMode);

	/**
	 * @brief Converts the connection settings to a string format.
	 * @return The connection settings as a string.
	 */
	FString ToString() const;

	/**
	 * @brief Creates a new connection settings with the updated password.
	 * @param InPassword The new password to use.
	 * @return A new connection settings with the updated password.
	 */
	FMqttifyConnectionSettingsRef FromNewPassword(const FString& InPassword) const
	{
		return MakeShareable<FMqttifyConnectionSettings>(new FMqttifyConnectionSettings(
			ConnectionProtocol,
			Port,
			Host,
			Username,
			Password,
			Path,
			PacketRetryIntervalSeconds,
			SocketConnectionTimeoutSeconds,
			KeepAliveIntervalSeconds,
			MqttConnectionTimeoutSeconds,
			InitialRetryConnectionIntervalSeconds,
			MaxConnectionRetries,
			MaxPacketRetries));
	}

private:
	FMqttifyConnectionSettings() = delete;

	/**
	 * @brief Creates a new instance of the connection settings.
	 * @param InConnectionProtocol The protocol to use for the connection. WSS/WS/MQTTS/MQTT
	 * @param InPort The port to connect to.
	 * @param InHost The host to connect to.
	 * @param InUsername The username to use for authentication.
	 * @param InPassword The password to use for authentication.
	 * @param InPath The Uri path to use for the connection.
	 * @param InPacketRetryIntervalSeconds The packet retry interval in seconds.
	 * @param InSocketConnectionTimeoutSeconds The socket connection timeout in seconds.
	 * @param InKeepAliveIntervalSeconds The MQTT keep alive interval in seconds.
	 * @param InMqttConnectionTimeoutSeconds The MQTT connection timeout in seconds.
	 * @param InInitialRetryIntervalSeconds The initial retry interval in seconds.
	 * @param InMaxConnectionRetries The maximum number of connection retries.
	 * @param InMaxPacketRetries The maximum number time to retry sending a packet.
	 * @param InClientId The ClientId to use for the connection.
	 */
	explicit FMqttifyConnectionSettings(const EMqttifyConnectionProtocol InConnectionProtocol,
										const int16 InPort,
										const FString& InHost,
										const FString& InUsername,
										const FString& InPassword,
										const FString& InPath,
										const uint16 InPacketRetryIntervalSeconds,
										const uint16 InSocketConnectionTimeoutSeconds,
										const uint16 InKeepAliveIntervalSeconds,
										const uint16 InMqttConnectionTimeoutSeconds,
										const uint16 InInitialRetryIntervalSeconds,
										const uint8 InMaxConnectionRetries,
										const uint8 InMaxPacketRetries,
										const FString& InClientId = TEXT(""));

	/**
	 * @brief Adds the given bytes to the given array.
	 * @param Bytes The array to add the bytes to.
	 * @param Src The bytes to add.
	 * @param Size The size of the bytes to add.
	 */
	static FORCEINLINE void AddToBytes(TArray<uint8>& Bytes, const void* Src, const int32 Size)
	{
		const int32 Offset = Bytes.Num();
		Bytes.SetNumUninitialized(Offset + Size, false);
		FMemory::Memcpy(Bytes.GetData() + Offset, Src, Size);
	}

	/**
	 * @brief Generates a deterministic ClientId based on the connection settings.
	 * We're using the connection settings fields excluding the password to generate a unique id.
	 * Allowing the user of the client to update the password without it changing
	 * @return A ClientId.
	 */
	FORCEINLINE FString GenerateClientId()
	{
		// 12 bytes for the hash codes of Username, Host, Path, where each hash code is 4 bytes (uint32)
		constexpr int32 Num = 12 +
			sizeof(Port) /*Port*/ +
			sizeof(ConnectionProtocol) +
			sizeof(MaxPacketRetries) +
			sizeof(PacketRetryIntervalSeconds) +
			sizeof(SocketConnectionTimeoutSeconds) +
			sizeof(KeepAliveIntervalSeconds) +
			sizeof(MqttConnectionTimeoutSeconds) +
			sizeof(InitialRetryConnectionIntervalSeconds) +
			sizeof(MaxConnectionRetries);

		TArray<uint8> Bytes;
		Bytes.Reserve(Num);

		// Add hash codes of Username, Host, Path
		int32 CurrentHash = FFnv::MemFnv32(*Username, Username.Len());
		AddToBytes(Bytes, &CurrentHash, sizeof(CurrentHash));

		CurrentHash = FFnv::MemFnv32(*Host, Host.Len());
		AddToBytes(Bytes, &CurrentHash, sizeof(CurrentHash));

		CurrentHash = FFnv::MemFnv32(*Path, Path.Len());
		AddToBytes(Bytes, &CurrentHash, sizeof(CurrentHash));

		// Add other data
		AddToBytes(Bytes, &ConnectionProtocol, sizeof(ConnectionProtocol));
		AddToBytes(Bytes, &Port, sizeof(Port));
		AddToBytes(Bytes, &PacketRetryIntervalSeconds, sizeof(PacketRetryIntervalSeconds));
		AddToBytes(Bytes, &SocketConnectionTimeoutSeconds, sizeof(SocketConnectionTimeoutSeconds));
		AddToBytes(Bytes, &KeepAliveIntervalSeconds, sizeof(KeepAliveIntervalSeconds));
		AddToBytes(Bytes, &MqttConnectionTimeoutSeconds, sizeof(MqttConnectionTimeoutSeconds));
		AddToBytes(Bytes, &InitialRetryConnectionIntervalSeconds, sizeof(InitialRetryConnectionIntervalSeconds));
		AddToBytes(Bytes, &MaxConnectionRetries, sizeof(MaxConnectionRetries));

		FString Result;

		// Reserve enough space for the username and the base64 encoded bytes.
		const int32 Base64Size = FBase64::GetEncodedDataSize(Bytes.Num());
		Result.Reserve(Username.Len() + Base64Size + 1);

		// Append the username, a separator and the base64 encoded bytes.
		Result.Append(Username);
		Result.Append(TEXT("_"));
		Result.Append(FBase64::Encode(Bytes));

		return Result;
	}
};


FORCEINLINE uint32 GetTypeHash(const FMqttifyConnectionSettings& ConnectionSettings)
{
	return ConnectionSettings.GetHashCode();
}
