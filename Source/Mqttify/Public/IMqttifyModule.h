#pragma  once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Mqtt/MqttifyConnectionSettings.h"

class FMqttifyConnectionSettings;
class IMqttifyClient;

class IMqttifyModule : public IModuleInterface
{
public:

	static constexpr auto ModuleName = TEXT("Mqttify");

	static FORCEINLINE bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded(ModuleName);
	}

	static FORCEINLINE IMqttifyModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IMqttifyModule>(ModuleName);
	}

	/**
	 * @brief Get or create a client for the given connection settings.
	 * @param InConnectionSettings The connection settings to use.
	 * @return A shared pointer to the MQTT client if the URL was valid
	 */
	virtual TSharedPtr<IMqttifyClient> GetOrCreateClient(const FMqttifyConnectionSettingsRef& InConnectionSettings) = 0;

	/**
	 * @Brief Update the credentials of a client.
	 * @param InUrl The URL of the client.
	 * @return A shared pointer to the MQTT client if the URL was valid
	 */
	virtual TSharedPtr<IMqttifyClient> GetOrCreateClient(const FString& InUrl) = 0;
};
