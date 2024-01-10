#include "MqttifyModule.h"

#include "GameDelegates.h"
#include "Mqtt/MqttifyConnectionSettings.h"
#include "Mqtt/MqttifyConnectionSettingsBuilder.h"

IMPLEMENT_MODULE(FMqttifyModule, Mqttify);

void FMqttifyModule::StartupModule()
{
	FGameDelegates::Get().GetEndPlayMapDelegate().AddRaw(this, &FMqttifyModule::OnEndPlay);
}

void FMqttifyModule::ShutdownModule()
{
	FGameDelegates::Get().GetEndPlayMapDelegate().RemoveAll(this);
}

void FMqttifyModule::OnEndPlay()
{
	ClientPool.Stop();
	ClientPool.Clear();
}


TSharedPtr<IMqttifyClient> FMqttifyModule::GetOrCreateClient(const FMqttifyConnectionSettingsRef& InConnectionSettings)
{
	check(IsInGameThread());
	return ClientPool.GetOrCreateClient(InConnectionSettings);
}

TSharedPtr<IMqttifyClient> FMqttifyModule::GetOrCreateClient(const FString& InUrl)
{
	check(IsInGameThread());
	const FMqttifyConnectionSettingsBuilder Builder{ InUrl };
	const TSharedPtr<FMqttifyConnectionSettings> ConnectionSettings = Builder.Build();

	if (nullptr != ConnectionSettings)
	{
		return ClientPool.GetOrCreateClient(ConnectionSettings.ToSharedRef());
	}

	return nullptr;
}
