#pragma  once

#include "CoreMinimal.h"
#include "IMqttifyModule.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Mqtt/MqttifyClientPool.h"
#include "Mqtt/MqttifyConnectionSettings.h"

class FMqttifyConnectionSettings;
class IMqttifyClient;


class FMqttifyModule final : public IMqttifyModule
{
public:
	/**
	 * @brief Get or create a client for the given connection settings.
	 * @param InConnectionSettings The connection settings to use.
	 * @return A shared pointer to the MQTT client if the URL was valid
	 */
	virtual TSharedPtr<IMqttifyClient>
	GetOrCreateClient(const FMqttifyConnectionSettingsRef& InConnectionSettings) override;

	/**
	 * @Brief Update the credentials of a client.
	 * @param InUrl The URL of the client.
	 * @return A shared pointer to the MQTT client if the URL was valid
	 */
	virtual TSharedPtr<IMqttifyClient> GetOrCreateClient(const FString& InUrl) override;

private:
	TSharedRef<Mqttify::FMqttifyClientPool> ClientPool = MakeShared<Mqttify::FMqttifyClientPool>();
};
