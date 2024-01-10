#pragma  once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Mqtt/MqttifyClientPool.h"
#include "Mqtt/MqttifyProtocolVersion.h"

class FMqttifyConnectionSettings;
class IMqttifyClient;

class FMqttifyModule final : public IModuleInterface
{
public:
	static FORCEINLINE bool IsAvailable()
	{
		return Get() != nullptr;
	}

	static FORCEINLINE IModuleInterface* Get()
	{
		check(IsInGameThread());
		return FModuleManager::LoadModulePtr<IModuleInterface>("Mqttify");
	}

	/**
	 * @brief Get or create a client for the given connection settings.
	 * @param InConnectionSettings The connection settings to use.
	 * @return A shared pointer to the MQTT client if the URL was valid
	 */
	TSharedPtr<IMqttifyClient> GetOrCreateClient(const FMqttifyConnectionSettingsRef& InConnectionSettings);

	/**
	 * @Brief Update the credentials of a client.
	 * @param InUrl The URL of the client.
	 * @return A shared pointer to the MQTT client if the URL was valid
	 */
	TSharedPtr<IMqttifyClient> GetOrCreateClient(const FString& InUrl);

private:
	Mqttify::FMqttifyClientPool ClientPool{};

	void StartupModule() override;
	void ShutdownModule() override;
	void OnEndPlay();
};
