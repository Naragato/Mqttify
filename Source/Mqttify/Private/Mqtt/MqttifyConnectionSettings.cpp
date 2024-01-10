#include "Mqtt/MqttifyConnectionSettings.h"

#include "Mqtt/MqttifyProtocolVersion.h"

FMqttifyConnectionSettings::FMqttifyConnectionSettings(
	const EMqttifyConnectionProtocol InConnectionProtocol,
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
	const FString& InClientId)
	: ClientId{ InClientId }
	, Port{ InPort }
	, Host{ InHost }
	, Username{ InUsername }
	, Password{ InPassword }
	, ConnectionProtocol{ InConnectionProtocol }
	, Path{ InPath }
	, PacketRetryIntervalSeconds{ InPacketRetryIntervalSeconds }
	, InitialRetryConnectionIntervalSeconds{ InInitialRetryIntervalSeconds }
	, SocketConnectionTimeoutSeconds{ InSocketConnectionTimeoutSeconds }
	, KeepAliveIntervalSeconds{ InKeepAliveIntervalSeconds }
	, MqttConnectionTimeoutSeconds{ InMqttConnectionTimeoutSeconds }
	, MaxConnectionRetries{ InMaxConnectionRetries }
	, MaxPacketRetries{ InMaxPacketRetries }
{
	if (ClientId.IsEmpty())
	{
		ClientId = GenerateClientId();
	}
}

FString FMqttifyConnectionSettings::GetClientId() const
{
	return ClientId;
}

uint32 FMqttifyConnectionSettings::GetHashCode() const
{
	// We're deliberately not hashing the password or client Id here
	// as they are not used for equality checks.
	uint32 Hash = 0;

	// Hash the transport protocol
	Hash = HashCombine(Hash, GetTypeHash(ConnectionProtocol));

	// Hash the port
	Hash = HashCombine(Hash, GetTypeHash(Port));

	// Hash the host
	Hash = HashCombine(Hash, GetTypeHash(Host));

	// Hash the username
	Hash = HashCombine(Hash, GetTypeHash(Username));

	// Hash InitialRetryInterval
	Hash = HashCombine(Hash, GetTypeHash(InitialRetryConnectionIntervalSeconds));

	// Hash SocketConnectionTimeoutSeconds
	Hash = HashCombine(Hash, GetTypeHash(SocketConnectionTimeoutSeconds));

	// Hash KeepAliveIntervalSeconds
	Hash = HashCombine(Hash, GetTypeHash(KeepAliveIntervalSeconds));

	// Hash MqttConnectionTimeoutSeconds
	Hash = HashCombine(Hash, GetTypeHash(MqttConnectionTimeoutSeconds));

	// Hash MaxConnectionRetries
	Hash = HashCombine(Hash, GetTypeHash(MaxConnectionRetries));

	return Hash;
}

TSharedPtr<FMqttifyConnectionSettings> FMqttifyConnectionSettings::CreateShared(
	const FString& InURL,
	const EMqttifyProtocolVersion InMqttProtocolVersion,
	const uint16 InPacketRetryIntervalSeconds,
	const uint16 InSocketConnectionTimeoutSeconds,
	const uint16 InKeepAliveIntervalSeconds,
	const uint16 InMqttConnectionTimeoutSeconds,
	const uint16 InInitialRetryIntervalSeconds,
	const uint8 InMaxConnectionRetries,
	const uint8 InMaxPacketRetries,
	const EMqttifyThreadMode InThreadMode)
{
	const FRegexPattern URLPattern(
		TEXT("(mqtt[s]?|ws[s]?)://(?:([^:/]+)(?::([^@/]+))?@)?([^:/@]+)(?::(\\d+))?(?:/([^?#/]+))?"));
	if (FRegexMatcher Matcher(URLPattern, InURL);
		Matcher.FindNext())
	{
		EMqttifyConnectionProtocol Protocol = EMqttifyConnectionProtocol::Mqtt;
		int32 Port                          = 1883; // Default port

		// Assign values based on regex groups
		const FString Scheme = Matcher.GetCaptureGroup(1);
		if (Scheme.Equals(TEXT("mqtt"), ESearchCase::IgnoreCase))
		{
			Protocol = EMqttifyConnectionProtocol::Mqtt;
		}
		else if (Scheme.Equals(TEXT("mqtts"), ESearchCase::IgnoreCase))
		{
			Protocol = EMqttifyConnectionProtocol::Mqtts;
		}
		else if (Scheme.Equals(TEXT("ws"), ESearchCase::IgnoreCase))
		{
			Protocol = EMqttifyConnectionProtocol::Ws;
		}
		else if (Scheme.Equals(TEXT("wss"), ESearchCase::IgnoreCase))
		{
			Protocol = EMqttifyConnectionProtocol::Wss;
		}

		const FString Username = Matcher.GetCaptureGroup(2);
		const FString Password = Matcher.GetCaptureGroup(3);
		const FString Host     = Matcher.GetCaptureGroup(4);
		const FString Path     = Matcher.GetCaptureGroup(6);
		const FString PortStr  = Matcher.GetCaptureGroup(5);
		if (!PortStr.IsEmpty())
		{
			Port = FCString::Atoi(*PortStr);
		}

		return MakeShareable<FMqttifyConnectionSettings>(new FMqttifyConnectionSettings(
			Protocol,
			Port,
			Host,
			Username,
			Password,
			Path,
			InPacketRetryIntervalSeconds,
			InSocketConnectionTimeoutSeconds,
			InKeepAliveIntervalSeconds,
			InMqttConnectionTimeoutSeconds,
			InInitialRetryIntervalSeconds,
			InMaxConnectionRetries,
			InMaxPacketRetries));
	}

	return nullptr;
}

FString FMqttifyConnectionSettings::ToString() const
{
	FString Result{};
	// Estimate final size and pre-allocate.

	int ReserveSize = (Username.IsEmpty() ? 0 : Username.Len() + 1)                     // username : or @
		+ (Password.IsEmpty() ? 0 : (Username.IsEmpty() ? 10 : 0) + Password.Len() + 1) // password @ or anonymous:
		+ Host.Len()                                                                    // host
		+ (Port > 0 ? 6 : 0)                                                            // :port
		+ (Path.IsEmpty() ? 0 : Path.Len() + 1);                                        // /path

	Result.Reserve(ReserveSize);

	// Protocol
	switch (ConnectionProtocol)
	{
		case EMqttifyConnectionProtocol::Mqtt:
			Result += TEXT("mqtt://");
			break;

		case EMqttifyConnectionProtocol::Mqtts:
			Result += TEXT("mqtts://");
			break;

		case EMqttifyConnectionProtocol::Ws:
			Result += TEXT("ws://");
			break;

		case EMqttifyConnectionProtocol::Wss:
			Result += TEXT("wss://");
			break;

		default:
			checkNoEntry();
	}

	// Username and password
	if (!Username.IsEmpty() || !Password.IsEmpty())
	{
		const FString ActualUsername = Username.IsEmpty() && !Password.IsEmpty() ? TEXT("anonymous") : Username;

		Result += ActualUsername;
		if (!Password.IsEmpty())
		{
			Result += TEXT(":");
			Result += Password;
		}
		Result += TEXT("@");
	}

	// Hostname and port
	Result += Host;
	if (Port > 0)
	{
		Result += FString::Printf(TEXT(":%i"), Port);
	}

	if (!Path.IsEmpty())
	{
		Result += TEXT("/");
		Result += Path;
	}

	return Result;
} // namespace Mqttify
