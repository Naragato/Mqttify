
#if WITH_DEV_AUTOMATION_TESTS

#include "LogMqttify.h"
#include "MqttifyModule.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Mqtt/MqttifyConnectionSettingsBuilder.h"
#include "Mqtt/Interface/IMqttifyClient.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Math/UnrealMathUtility.h"

using namespace Mqttify;

BEGIN_DEFINE_SPEC(
	FMqttifyClientConnectionTests,
	"Mqttify.Automation.MqttifyClientConnectionTests",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

	TSharedPtr<IMqttifyClient> MqttClient;
	bool bOriginalDisableCertValidation = false;
	FString DockerContainerName = TEXT("vernemq_test_container");
	int16 BrokerPort = 1883;

	static uint16 FindAvailablePort(uint16 StartPort, uint16 EndPort);
	bool IsBrokerReady(int32 Port, float TimeoutSeconds) const;
	void RemoveExistingContainer();
	FString GetDockerLogs() const;

END_DEFINE_SPEC(FMqttifyClientConnectionTests)

void FMqttifyClientConnectionTests::Define()
{
	BeforeEach(
		[this]()
		{
			RemoveExistingContainer();
			int32 ReturnCode;
			FString OutString;
			FPlatformProcess::ExecProcess(TEXT("docker"), TEXT("--version"), EExecProcFlags::None, &ReturnCode, &OutString, nullptr);

			LOG_MQTTIFY(Verbose, TEXT("Docker version output: %s"), *OutString);
			if (ReturnCode != 0)
			{
				AddError(TEXT("Docker is not installed or not available in PATH."));
				return;
			}

			BrokerPort = FindAvailablePort(1025, 32000);
			if (BrokerPort == -1)
			{
				AddError(TEXT("No available port found for MQTT broker."));
				return;
			}

			const FString RunParams = FString::Printf(TEXT("run -p %d:8080 -e \"DOCKER_VERNEMQ_ACCEPT_EULA=yes\" -e \"DOCKER_VERNEMQ_LOG__CONSOLE__LEVEL=debug\" -e \"DOCKER_VERNEMQ_ALLOW_ANONYMOUS=on\" --name %s -d vernemq/vernemq"), BrokerPort, *DockerContainerName);

			FPlatformProcess::ExecProcess(TEXT("docker"), *RunParams, EExecProcFlags::None, &ReturnCode, &OutString, nullptr);

			LOG_MQTTIFY(Verbose, TEXT("Docker run output: %s"), *OutString);
			if (ReturnCode != 0)
			{
				AddError(TEXT("Docker run did not succeed."));
				return;
			}

			if (!IsBrokerReady(BrokerPort, 30.0f))
			{
				AddError(TEXT("MQTT broker did not become ready in time."));
				return;
			}

			GConfig->GetBool(TEXT("LwsWebSocket"), TEXT("bDisableCertValidation"), bOriginalDisableCertValidation, GEngineIni);
			GConfig->SetBool(TEXT("LwsWebSocket"), TEXT("bDisableCertValidation"), true, GEngineIni);
			GConfig->Flush(false, GEngineIni);
		});

	AfterEach(
		[this]()
		{
			const FString DockerLogs = GetDockerLogs();
			LOG_MQTTIFY(Verbose, TEXT("Docker logs: \n---\n%s\n---"), *DockerLogs);
			RemoveExistingContainer();
			GConfig->SetBool(TEXT("LwsWebSocket"), TEXT("bDisableCertValidation"), bOriginalDisableCertValidation, GEngineIni);
			GConfig->Flush(false, GEngineIni);
		});

	LatentIt(
		"ConnectToBroker_Success",
		EAsyncExecution::TaskGraphMainThread,
		[this](const FDoneDelegate& TestDone)
		{
			const FString BrokerURL = FString::Printf(TEXT("ws://localhost:%d/mqtt"), BrokerPort);
			const FMqttifyConnectionSettingsBuilder ConnectionSettingsBuilder = FMqttifyConnectionSettingsBuilder(BrokerURL);
			const TSharedPtr<FMqttifyConnectionSettings> ConnectionSettings = ConnectionSettingsBuilder.Build();

			if (!IMqttifyModule::IsAvailable())
			{
				AddError(TEXT("Mqttify module is not available."));
				TestDone.Execute();
				return;
			}

			IMqttifyModule& MqttifyModule = IMqttifyModule::Get();
			MqttClient = MqttifyModule.GetOrCreateClient(ConnectionSettings.ToSharedRef());
			if (!MqttClient.IsValid())
			{
				AddError(TEXT("Failed to create MQTT client."));
				TestDone.Execute();
				return;
			}

			MqttClient->OnConnect().AddLambda(
				[this, TestDone](const bool bReturnCode)
				{
					TestTrue(TEXT("Client connected successfully."), bReturnCode);
				});

			MqttClient->ConnectAsync(false).Then(
				[this, TestDone](const TFuture<TMqttifyResult<void>>& InFuture)
				{
					TestTrue(TEXT("Future should have result"), InFuture.IsReady());
					TestTrue(TEXT("Future should be successful"), InFuture.Get().HasSucceeded());
					if (!InFuture.Get().HasSucceeded())
					{
						TestDone.Execute();
						return;
					}
					MqttClient->DisconnectAsync().Then(
						[this, TestDone](const TFuture<TMqttifyResult<void>>& InFuture)
						{
							TestTrue(TEXT("Future should have result"), InFuture.IsReady());
							TestTrue(TEXT("Future should be successful"), InFuture.Get().HasSucceeded());
							TestDone.Execute();
						});
				});
		});
}

uint16 FMqttifyClientConnectionTests::FindAvailablePort(const uint16 StartPort, const uint16 EndPort)
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	const TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();

	const int16 CurrentPort = FMath::RandRange(StartPort, EndPort);
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

bool FMqttifyClientConnectionTests::IsBrokerReady(const int32 Port, const float TimeoutSeconds) const
{
	// Wait for broker to startup
	FPlatformProcess::Sleep(TimeoutSeconds);
	const FString RunningParams = FString::Printf(TEXT("inspect -f \"{{.State.Running}}\" %s"), *DockerContainerName);

	int32 ReturnCode;
	FString OutString;
	FPlatformProcess::ExecProcess(TEXT("docker"), *RunningParams, EExecProcFlags::None, &ReturnCode, &OutString, nullptr);
	OutString.TrimStartAndEndInline();

	if (ReturnCode == 0 && OutString.Equals(TEXT("true"), ESearchCase::IgnoreCase))
	{
		return true;
	}

	return false;
}

void FMqttifyClientConnectionTests::RemoveExistingContainer()
{
	int32 ReturnCode;
	FString OutString;
	const FString StopParams = FString::Printf(TEXT("stop %s"), *DockerContainerName);
	FPlatformProcess::ExecProcess(TEXT("docker"), *StopParams, EExecProcFlags::None, &ReturnCode, &OutString, nullptr);
	if (ReturnCode != 0)
	{
		AddWarning(TEXT("Failed to stop Docker container."));
	}

	const FString RemoveParams = FString::Printf(TEXT("rm %s"), *DockerContainerName);
	FPlatformProcess::ExecProcess(TEXT("docker"), *RemoveParams, EExecProcFlags::None, &ReturnCode, &OutString, nullptr);
	if (ReturnCode != 0)
	{
		AddWarning(TEXT("Failed to remove Docker container."));
	}
}

FString FMqttifyClientConnectionTests::GetDockerLogs() const
{
	int32 ReturnCode;
	FString OutString;
	const FString LogsParams = FString::Printf(TEXT("logs %s --details --tail all --timestamps"), *DockerContainerName);
	FPlatformProcess::ExecProcess(TEXT("docker"), *LogsParams, EExecProcFlags::None, &ReturnCode, &OutString, nullptr);
	OutString.TrimStartAndEndInline();

	if (ReturnCode == 0)
	{
		return OutString;
	}

	return FString();
}

#endif // WITH_DEV_AUTOMATION_TESTS
