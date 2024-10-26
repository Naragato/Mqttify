#pragma once

#include "CoreMinimal.h"
#include "Mqtt/Delegates/OnConnect.h"
#include "Mqtt/Delegates/OnDisconnect.h"
#include "Mqtt/Delegates/OnMessage.h"
#include "Mqtt/Delegates/OnPublish.h"
#include "Mqtt/Delegates/OnSubscribe.h"
#include "Mqtt/Delegates/OnUnsubscribe.h"
#include "Mqtt/Interface/IMqttifyConnectableAsync.h"
#include "Mqtt/Interface/IMqttifyDisconnectableAsync.h"
#include "Mqtt/Interface/IMqttifyPublishableAsync.h"
#include "Mqtt/Interface/IMqttifySubscribableAsync.h"
#include "Mqtt/Interface/IMqttifyUnsubscribableAsync.h"

class FMqttifyConnectionSettings;
enum class EMqttifyConnectReturnCode : uint8;
struct FMqttifyMessage;
struct FMqttifySubscribeResult;
struct FMqttifyTopicFilter;
struct FMqttifyUnsubscribeResult;

using FMqttifyConnectionSettingsRef = TSharedRef<FMqttifyConnectionSettings, ESPMode::ThreadSafe>;

class MQTTIFY_API IMqttifyClient : public Mqttify::IMqttifyDisconnectableAsync,
                                   public Mqttify::IMqttifyPublishableAsync,
                                   public Mqttify::IMqttifySubscribableAsync,
                                   public Mqttify::IMqttifyUnsubscribableAsync,
                                   public Mqttify::IMqttifyConnectableAsync
{
public:
	virtual ~IMqttifyClient() override = default;

	virtual Mqttify::FOnConnect& OnConnect() = 0;
	virtual Mqttify::FOnDisconnect& OnDisconnect() = 0;
	virtual Mqttify::FOnPublish& OnPublish() = 0;
	virtual Mqttify::FOnSubscribe& OnSubscribe() = 0;
	virtual Mqttify::FOnUnsubscribe& OnUnsubscribe() = 0;
	virtual Mqttify::FOnMessage& OnMessage() = 0;

	/**
	 * @brief Get the Settings for this client.
	 * @return Settings for this client.
	 */
	virtual const FMqttifyConnectionSettingsRef GetConnectionSettings() const = 0;

	/**
	 * @brief Is client currently connected?
	 * @return Is client currently connected?
	 */
	virtual bool IsConnected() const = 0;
};
