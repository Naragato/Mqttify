#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "LogMqttify.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Misc/Paths.h"
#include "Misc/Timeout.h"
#include "Mqtt/MqttifyConnectionProtocol.h"

namespace Mqttify
{
	enum class ContainerType
	{
		Mqtt,
		Echo
	};

	struct FMqttifyTestDockerSpec
	{
		uint32 PrivatePort;
		uint32 PublicPort;
		EMqttifyConnectionProtocol Protocol;
		ContainerType Type = ContainerType::Mqtt;
	};

	inline FString GetDockerExecutable()
	{
#if PLATFORM_WINDOWS
		return TEXT("docker");
#else
		const TArray<FString> PathArray = {
			TEXT("/usr/bin"),
			TEXT("/usr/local/bin"),
			TEXT("/opt/homebrew/bin"),
			TEXT("/opt/local/bin"),
			TEXT("/usr/sbin"),
			TEXT("/sbin")
		};
		for (const FString& Path : PathArray)
		{
			FString DockerPath = FPaths::Combine(Path, TEXT("docker"));
			if (FPaths::FileExists(DockerPath))
			{
				return DockerPath;
			}
		}
		return TEXT("docker");
#endif
	}

	inline uint16 FindAvailablePort(const uint16 InStartPort, const uint16 InEndPort)
	{
		ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
		const TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();

		const int16 CurrentPort = FMath::RandRange(InStartPort, InEndPort);
		bool bCanBind = false;
		do
		{
			FSocket* Socket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("PortCheck"), false);
			if (!Socket)
			{
				continue;
			}

			Addr->SetAnyAddress();
			Addr->SetPort(CurrentPort);

			bCanBind = Socket->Bind(*Addr);

			Socket->Close();
			SocketSubsystem->DestroySocket(Socket);

			if (bCanBind)
			{
				return CurrentPort;
			}
		}
		while (!bCanBind);
		return -1;
	}

	inline bool HasContainerStarted(const FString& InDockerContainerName, const float InTimeoutSeconds)
	{
		// Wait for broker to startup
		const FString RunningParams = FString::Printf(
			TEXT("inspect -f \"{{.State.Running}}\" %s"),
			*InDockerContainerName);

		int32 ReturnCode;
		FString OutString;

		const UE::FTimeout Timeout{FTimespan::FromSeconds(InTimeoutSeconds)};
		while (Timeout.GetRemainingTime() > 0.0f)
		{
			FPlatformProcess::ExecProcess(*GetDockerExecutable(), *RunningParams, &ReturnCode, &OutString, nullptr);
			OutString.TrimStartAndEndInline();
			if (OutString.Equals(TEXT("true"), ESearchCase::IgnoreCase))
			{
				return true;
			}
			LOG_MQTTIFY(Verbose, TEXT("Docker inspect output: %s, return code: %d"), *OutString, ReturnCode);
			FPlatformProcess::Sleep(0.1f);
		}

		return false;
	}

	inline bool RemoveExistingContainer(const FString& InDockerContainerName)
	{
		int32 ReturnCode;
		FString OutString;
		const FString StopParams = FString::Printf(TEXT("stop %s"), *InDockerContainerName);
		FPlatformProcess::ExecProcess(*GetDockerExecutable(), *StopParams, &ReturnCode, &OutString, nullptr);
		const FString RemoveParams = FString::Printf(TEXT("rm %s"), *InDockerContainerName);
		FPlatformProcess::ExecProcess(*GetDockerExecutable(), *RemoveParams, &ReturnCode, &OutString, nullptr);
		if (ReturnCode != 0)
		{
			return false;
		}

		return true;
	}

	inline bool IsSocketAvailable(const FMqttifyTestDockerSpec& InSpec, const float InTimeoutSeconds)
	{
		ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

		if (nullptr == SocketSubsystem)
		{
			return false;
		}

		const UE::FTimeout Timeout{FTimespan::FromSeconds(InTimeoutSeconds)};
		while (Timeout.GetRemainingTime() > 0.0f)
		{
			TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();
			bool bIsValid;
			Addr->SetIp(TEXT("127.0.0.1"), bIsValid);
			Addr->SetPort(InSpec.PublicPort);

			if (bIsValid)
			{
				FSocket* Socket = SocketSubsystem->CreateSocket(
					NAME_Stream,
					TEXT("BrokerReadinessCheck"),
					Addr->GetProtocolType());
				if (Socket)
				{
					Socket->SetNonBlocking(false);
					Socket->SetReuseAddr(true);
					Socket->SetRecvErr();

					const bool bConnected = Socket->Connect(*Addr);
					if (bConnected)
					{
						SocketSubsystem->DestroySocket(Socket);
						LOG_MQTTIFY(Verbose, TEXT("Socket available"));
						return true;
					}
					SocketSubsystem->DestroySocket(Socket);
				}
			}
			FPlatformProcess::Sleep(0.5f);
		}

		LOG_MQTTIFY(Warning, TEXT("Socket not available"));
		return false;
	}

	inline bool StartBroker(const FString& InDockerContainerName, const FMqttifyTestDockerSpec& InSpec)
	{
		int32 ReturnCode;
		FString OutString;
		FPlatformProcess::ExecProcess(*GetDockerExecutable(), TEXT("--version"), &ReturnCode, &OutString, nullptr);

		LOG_MQTTIFY(Verbose, TEXT("Docker version output: %s"), *OutString);
		if (ReturnCode != 0)
		{
			LOG_MQTTIFY(Warning, TEXT("Docker is not installed or not available in PATH"));
			return false;
		}

		if (!RemoveExistingContainer(InDockerContainerName))
		{
			LOG_MQTTIFY(Warning, TEXT("Failed to remove MQTT broker container."));
		}

		const FString RunParams = InSpec.Type == ContainerType::Mqtt
									? FString::Printf(
										TEXT(
											"run -p %d:%d -e \"DOCKER_VERNEMQ_ACCEPT_EULA=yes\" -e \"DOCKER_VERNEMQ_LOG__CONSOLE__LEVEL=debug\" -e \"DOCKER_VERNEMQ_ALLOW_ANONYMOUS=on\" --name %s -d vernemq/vernemq"),
										InSpec.PublicPort,
										InSpec.PrivatePort,
										*InDockerContainerName)
									: FString::Printf(
										TEXT(
											"run -p %d:%d --name %s -d alpine sh -c  \"apk add --no-cache openssl && openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes -subj '/CN=localhost' && openssl s_server -accept %d -cert cert.pem -key key.pem -rev\""),
										InSpec.PublicPort,
										InSpec.PrivatePort,
										*InDockerContainerName,
										InSpec.PrivatePort);

		FPlatformProcess::ExecProcess(*GetDockerExecutable(), *RunParams, &ReturnCode, &OutString, nullptr);

		LOG_MQTTIFY(Verbose, TEXT("Docker run output: %s"), *OutString);
		if (ReturnCode != 0)
		{
			LOG_MQTTIFY(Warning, TEXT("Docker run failed with error code %d"), ReturnCode);
			return false;
		}

		FPlatformProcess::Sleep(60.f);

		if (!HasContainerStarted(InDockerContainerName, 60.0f))
		{
			LOG_MQTTIFY(Warning, TEXT("Container failed to start."));
			return false;
		}

		if (!IsSocketAvailable(InSpec, 60.0f))
		{
			LOG_MQTTIFY(Warning, TEXT("Failed to connect to MQTT broker."));
			return false;
		}

		return true;
	}

	inline FString GetDockerLogs(const FString& InContainerName)
	{
		int32 ReturnCode;
		FString OutString;
		const FString LogsParams = FString::Printf(TEXT("logs %s --details --tail all --timestamps"), *InContainerName);
		FPlatformProcess::ExecProcess(*GetDockerExecutable(), *LogsParams, &ReturnCode, &OutString, nullptr);
		OutString.TrimStartAndEndInline();
		return OutString;
	}


	inline const TCHAR* GetProtocolString(const EMqttifyConnectionProtocol InProtocol)
	{
		switch (InProtocol)
		{
		case EMqttifyConnectionProtocol::Mqtt:
			return TEXT("mqtt");
		case EMqttifyConnectionProtocol::Mqtts:
			return TEXT("mqtts");
		case EMqttifyConnectionProtocol::Ws:
			return TEXT("ws");
		case EMqttifyConnectionProtocol::Wss:
			return TEXT("wss");
		}
		return TEXT("");
	}

	inline const TCHAR* GetPath(const EMqttifyConnectionProtocol InProtocol)
	{
		switch (InProtocol)
		{
		case EMqttifyConnectionProtocol::Mqtts:
		case EMqttifyConnectionProtocol::Mqtt:
			return TEXT("");
		case EMqttifyConnectionProtocol::Ws:
		case EMqttifyConnectionProtocol::Wss:
			return TEXT("/mqtt");
		}
		return TEXT("");
	}
} // namespace Mqttify
#endif // WITH_DEV_AUTOMATION_TESTS
