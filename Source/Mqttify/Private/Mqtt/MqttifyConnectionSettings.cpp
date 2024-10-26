#include "Mqtt/MqttifyConnectionSettings.h"

#include "BasicCredentialsProvider.h"
#include "Internationalization/Regex.h"
#include "Misc/Fnv.h"

FMqttifyConnectionSettings::FMqttifyConnectionSettings(
	FString&& InHost,
	FString&& InPath,
	const EMqttifyConnectionProtocol InConnectionProtocol,
	const int16 InPort,
	const FMqttifyCredentialsProviderRef& InCredentialsProvider,
	const uint32 InMaxPacketSize,
	const uint16 InPacketRetryIntervalSeconds,
	const uint16 InSocketConnectionTimeoutSeconds,
	const uint16 InKeepAliveIntervalSeconds,
	const uint16 InMqttConnectionTimeoutSeconds,
	const uint16 InInitialRetryIntervalSeconds,
	const uint8 InMaxConnectionRetries,
	const uint8 InMaxPacketRetries,
	const bool bInShouldVerifyCertificate,
	FString&& InClientId
	)
	: MaxPacketSize{InMaxPacketSize}
	, ClientId{InClientId}
	, Port{InPort}
	, Host{InHost}
	, ConnectionProtocol{InConnectionProtocol}
	, CredentialsProvider{InCredentialsProvider}
	, Path{InPath}
	, PacketRetryIntervalSeconds{InPacketRetryIntervalSeconds}
	, InitialRetryConnectionIntervalSeconds{InInitialRetryIntervalSeconds}
	, SocketConnectionTimeoutSeconds{InSocketConnectionTimeoutSeconds}
	, KeepAliveIntervalSeconds{InKeepAliveIntervalSeconds}
	, MqttConnectionTimeoutSeconds{InMqttConnectionTimeoutSeconds}
	, MaxConnectionRetries{InMaxConnectionRetries}
	, MaxPacketRetries{InMaxPacketRetries}
	, bShouldVerifyServerCertificate{bInShouldVerifyCertificate}
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
	// We're deliberately not hashing the password or client ID here
	// as they are not used for equality checks.
	uint32 Hash = 0;

	// Hash the client Id
	Hash = HashCombine(Hash, GetTypeHash(ClientId));

	// Hash the transport protocol
	Hash = HashCombine(Hash, GetTypeHash(ConnectionProtocol));

	// Hash the port
	Hash = HashCombine(Hash, GetTypeHash(Port));

	// Hash the host
	Hash = HashCombine(Hash, GetTypeHash(Host));

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
	const uint32 InMaxPacketSize,
	const uint16 InPacketRetryIntervalSeconds,
	const uint16 InSocketConnectionTimeoutSeconds,
	const uint16 InKeepAliveIntervalSeconds,
	const uint16 InMqttConnectionTimeoutSeconds,
	const uint16 InInitialRetryIntervalSeconds,
	const uint8 InMaxConnectionRetries,
	const uint8 InMaxPacketRetries,
	const bool bInShouldVerifyCertificate,
	FString&& InClientId
	)
{
	const FRegexPattern URLPattern(TEXT("(mqtt[s]?|ws[s]?)://(?:([^:/]+)(?::([^@/]+))?@)?([^:/@]+)(?::(\\d+))?(?:/([^?#/]+))?"));
	FRegexMatcher Matcher(URLPattern, InURL);
	if (Matcher.FindNext())
	{
		// Parse protocol
		const EMqttifyConnectionProtocol Protocol = ParseProtocol(Matcher.GetCaptureGroup(1));

		// Parse host and port
		FString Host = Matcher.GetCaptureGroup(4);
		const FString PortStr = Matcher.GetCaptureGroup(5);
		const int32 Port = PortStr.IsEmpty() ? DefaultPort(Protocol) : FCString::Atoi(*PortStr);

		// Parse credentials
		FString Username = Matcher.GetCaptureGroup(2);
		FString Password = Matcher.GetCaptureGroup(3);
		const TSharedRef<IMqttifyCredentialsProvider> CredentialsProvider = MakeShared<FBasicCredentialsProvider>(
			MoveTemp(Username),
			MoveTemp(Password));

		// Parse path
		FString Path = Matcher.GetCaptureGroup(6);

		return CreateSharedInternal(
			MoveTemp(Host),
			MoveTemp(Path),
			Protocol,
			Port,
			CredentialsProvider,
			InMaxPacketSize,
			InPacketRetryIntervalSeconds,
			InSocketConnectionTimeoutSeconds,
			InKeepAliveIntervalSeconds,
			InMqttConnectionTimeoutSeconds,
			InInitialRetryIntervalSeconds,
			InMaxConnectionRetries,
			InMaxPacketRetries,
			bInShouldVerifyCertificate,
			MoveTemp(InClientId));
	}

	return nullptr;
}


TSharedPtr<FMqttifyConnectionSettings> FMqttifyConnectionSettings::CreateShared(
	const FString& InURL,
	const TSharedRef<IMqttifyCredentialsProvider>& CredentialsProvider,
	const uint32 InMaxPacketSize,
	const uint16 InPacketRetryIntervalSeconds,
	const uint16 InSocketConnectionTimeoutSeconds,
	const uint16 InKeepAliveIntervalSeconds,
	const uint16 InMqttConnectionTimeoutSeconds,
	const uint16 InInitialRetryIntervalSeconds,
	const uint8 InMaxConnectionRetries,
	const uint8 InMaxPacketRetries,
	const bool bInShouldVerifyCertificate,
	FString&& InClientId
	)
{
	const FRegexPattern URLPattern(TEXT("(mqtt[s]?|ws[s]?)://(?:([^:/]+)(?::([^@/]+))?@)?([^:/@]+)(?::(\\d+))?(?:/([^?#/]+))?"));
	FRegexMatcher Matcher(URLPattern, InURL);
	if (Matcher.FindNext())
	{
		// Ensure no username or password is present
		const FString Username = Matcher.GetCaptureGroup(2);
		const FString Password = Matcher.GetCaptureGroup(3);
		if (!Username.IsEmpty() || !Password.IsEmpty())
		{
			// Username or password should not be present
			return nullptr;
		}

		// Parse protocol
		const EMqttifyConnectionProtocol Protocol = ParseProtocol(Matcher.GetCaptureGroup(1));

		// Parse host and port
		FString Host = Matcher.GetCaptureGroup(4);
		const FString PortStr = Matcher.GetCaptureGroup(5);
		const int32 Port = PortStr.IsEmpty() ? DefaultPort(Protocol) : FCString::Atoi(*PortStr);

		// Parse path
		FString Path = Matcher.GetCaptureGroup(6);

		return CreateSharedInternal(
			MoveTemp(Host),
			MoveTemp(Path),
			Protocol,
			Port,
			CredentialsProvider,
			InMaxPacketSize,
			InPacketRetryIntervalSeconds,
			InSocketConnectionTimeoutSeconds,
			InKeepAliveIntervalSeconds,
			InMqttConnectionTimeoutSeconds,
			InInitialRetryIntervalSeconds,
			InMaxConnectionRetries,
			InMaxPacketRetries,
			bInShouldVerifyCertificate,
			MoveTemp(InClientId));
	}

	return nullptr;
}


EMqttifyConnectionProtocol FMqttifyConnectionSettings::ParseProtocol(const FString& Scheme)
{
	if (Scheme.Equals(TEXT("mqtt"), ESearchCase::IgnoreCase))
	{
		return EMqttifyConnectionProtocol::Mqtt;
	}
	if (Scheme.Equals(TEXT("mqtts"), ESearchCase::IgnoreCase))
	{
		return EMqttifyConnectionProtocol::Mqtts;
	}
	if (Scheme.Equals(TEXT("ws"), ESearchCase::IgnoreCase))
	{
		return EMqttifyConnectionProtocol::Ws;
	}
	if (Scheme.Equals(TEXT("wss"), ESearchCase::IgnoreCase))
	{
		return EMqttifyConnectionProtocol::Wss;
	}
	// Default to Mqtt if unknown
	return EMqttifyConnectionProtocol::Mqtt;
}

int32 FMqttifyConnectionSettings::DefaultPort(const EMqttifyConnectionProtocol Protocol)
{
	switch (Protocol)
	{
	case EMqttifyConnectionProtocol::Mqtt:
		return 1883;
	case EMqttifyConnectionProtocol::Mqtts:
		return 8883;
	case EMqttifyConnectionProtocol::Ws:
		return 80;
	case EMqttifyConnectionProtocol::Wss:
		return 443;
	default:
		return 1883;
	}
}

void FMqttifyConnectionSettings::AddToBytes(TArray<uint8>& Bytes, const void* Src, const int32 Size)
{
	const int32 Offset = Bytes.Num();
	Bytes.SetNumUninitialized(Offset + Size, false);
	FMemory::Memcpy(Bytes.GetData() + Offset, Src, Size);
}

FString FMqttifyConnectionSettings::GenerateClientId() const
{
	const FString Client = FString::Printf(
		TEXT("%s@%s:%u"),
		FPlatformProcess::UserName(),
		FPlatformProcess::ComputerName(),
		FPlatformProcess::GetCurrentProcessId());
	// 12 bytes for the hash codes of Client, Host, Path, where each hash code is 4 bytes (uint32)
	constexpr int32 Num = 12 + sizeof(Port) /*Port*/ + sizeof(ConnectionProtocol) + sizeof(MaxPacketRetries) + sizeof(PacketRetryIntervalSeconds) +
		sizeof(SocketConnectionTimeoutSeconds) + sizeof(KeepAliveIntervalSeconds) + sizeof(MqttConnectionTimeoutSeconds) + sizeof(
			InitialRetryConnectionIntervalSeconds) + sizeof(MaxConnectionRetries);

	TArray<uint8> Bytes;
	Bytes.Reserve(Num);

	// Add hash codes of Username, Host, Path
	int32 CurrentHash = FFnv::MemFnv32(*Client, Client.Len());
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

	// Reserve enough space for the Client and the base64 encoded bytes.
	const int32 Base64Size = FBase64::GetEncodedDataSize(Bytes.Num());
	Result.Reserve(Client.Len() + Base64Size + 1);

	// Append the username, a separator and the base64 encoded bytes.
	Result.Append(Client);
	Result.Append(TEXT("_"));
	Result.Append(FBase64::Encode(Bytes));

	return Result;
}

FString FMqttifyConnectionSettings::ToString() const
{
	FString Result{};
	// Estimate final size and pre-allocate.

	const int ReserveSize = 7 + // proto
		Host.Len() // host
		+ (Port > 0 ? 6 : 0) // :port
		+ (Path.IsEmpty() ? 0 : Path.Len() + 1); // /path

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

	default: checkNoEntry();
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
}

FString FMqttifyConnectionSettings::ToConnectionString() const
{
	FString Result{};
	// Estimate final size and pre-allocate.

	const int ReserveSize = 7 + // proto
		Host.Len() // host
		+ (Port > 0 ? 6 : 0) // :port
		+ (Path.IsEmpty() ? 0 : Path.Len() + 1); // /path

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

	default: checkNoEntry();
	}

	const auto [Password, Username] = CredentialsProvider->GetCredentials();
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
}
