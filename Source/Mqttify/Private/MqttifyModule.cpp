#include "MqttifyModule.h"

#include "Mqtt/MqttifyClientPool.h"
#include "Mqtt/MqttifyConnectionSettings.h"
#include "Mqtt/MqttifyConnectionSettingsBuilder.h"

IMPLEMENT_MODULE(FMqttifyModule, Mqttify);

TSharedPtr<IMqttifyClient> FMqttifyModule::GetOrCreateClient(const FMqttifyConnectionSettingsRef& InConnectionSettings)
{
	return ClientPool.GetOrCreateClient(InConnectionSettings);
}

TSharedPtr<IMqttifyClient> FMqttifyModule::GetOrCreateClient(const FString& InUrl)
{
	const FMqttifyConnectionSettingsBuilder Builder{ InUrl };
	const TSharedPtr<FMqttifyConnectionSettings> ConnectionSettings = Builder.Build();

	if (nullptr != ConnectionSettings)
	{
		return ClientPool.GetOrCreateClient(ConnectionSettings.ToSharedRef());
	}

	return nullptr;
}
