#pragma once
#include "Mqtt/MqttifyConnectionProtocol.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "LogMqttify.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

namespace Mqttify
{
	struct FMqttifyTestDockerSpec
	{
		uint32 PrivatePort;
		uint32 PublicPort;
		EMqttifyConnectionProtocol Protocol;
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

	inline bool IsBrokerReady(const FString& InDockerContainerName, const float InTimeoutSeconds)
	{
		// Wait for broker to startup
		FPlatformProcess::Sleep(InTimeoutSeconds);
		const FString RunningParams = FString::Printf(
			TEXT("inspect -f \"{{.State.Running}}\" %s"),
			*InDockerContainerName);

		int32 ReturnCode;
		FString OutString;
		FPlatformProcess::ExecProcess(*GetDockerExecutable(), *RunningParams, &ReturnCode, &OutString, nullptr);
		OutString.TrimStartAndEndInline();

		if (ReturnCode == 0 && OutString.Equals(TEXT("true"), ESearchCase::IgnoreCase))
		{
			return true;
		}

		return false;
	}

	inline bool RemoveExistingContainer(const FString& InDockerContainerName)
	{
		int32 ReturnCode;
		FString OutString;
		const FString StopParams = FString::Printf(TEXT("stop %s"), *InDockerContainerName);
		FPlatformProcess::ExecProcess(*GetDockerExecutable(), *StopParams, &ReturnCode, &OutString, nullptr);
		if (ReturnCode != 0)
		{
			return false;
		}

		const FString RemoveParams = FString::Printf(TEXT("rm %s"), *InDockerContainerName);
		FPlatformProcess::ExecProcess(*GetDockerExecutable(), *RemoveParams, &ReturnCode, &OutString, nullptr);
		if (ReturnCode != 0)
		{
			return false;
		}

		return true;
	}

	inline bool StartBroker(const FString& InDockerContainerName, const FMqttifyTestDockerSpec& InSpec)
	{
		int32 ReturnCode;
		FString OutString;
		FPlatformProcess::ExecProcess(*GetDockerExecutable(), TEXT("--version"), &ReturnCode, &OutString, nullptr);

		LOG_MQTTIFY(Verbose, TEXT("Docker version output: %s"), *OutString);
		if (ReturnCode != 0)
		{
			LOG_MQTTIFY(Warning, TEXT("Docker is not installed or not available in PATH."));
			return false;
		}

		if (!RemoveExistingContainer(InDockerContainerName))
		{
			LOG_MQTTIFY(Warning, TEXT("Failed to remove MQTT broker container."));
		}

		const FString RunParams = FString::Printf(
			TEXT(
				"run -p %d:%d -e \"DOCKER_VERNEMQ_ACCEPT_EULA=yes\" -e \"DOCKER_VERNEMQ_LOG__CONSOLE__LEVEL=debug\" -e \"DOCKER_VERNEMQ_ALLOW_ANONYMOUS=on\" --name %s -d vernemq/vernemq"),
			InSpec.PublicPort,
			InSpec.PrivatePort,
			*InDockerContainerName);

		FPlatformProcess::ExecProcess(*GetDockerExecutable(), *RunParams, &ReturnCode, &OutString, nullptr);

		LOG_MQTTIFY(Verbose, TEXT("Docker run output: %s"), *OutString);
		if (ReturnCode != 0)
		{
			LOG_MQTTIFY(Warning, TEXT("Docker run failed with error code %d"), ReturnCode);
			return false;
		}

		if (!IsBrokerReady(InDockerContainerName, 30.0f))
		{
			LOG_MQTTIFY(Warning, TEXT("Failed to start MQTT broker."));
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
