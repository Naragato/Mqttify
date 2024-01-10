#pragma once

#include "Async/Future.h"
#include "Interface/ITickableMqttifyClient.h"
#include "Mqtt/MqttifySubscribeResult.h"
#include "Mqtt/MqttifyTopicFilter.h"
#include "Mqtt/Interface/IMqttifyClient.h"
#include "State/MqttifyClientContext.h"
#include "State/MqttifyClientDisconnectedState.h"


enum class EMqttifyConnectReturnCode : uint8;
struct FMqttifyMessage;

namespace Mqttify
{
	class IMqttifyConnectableAsync;
	class IMqttifyControlPacket;

	/**
	 * @brief A MQTT client that can be ticked.
	 */
	class FMqttifyClient final : public ITickableMqttifyClient, public TSharedFromThis<FMqttifyClient>
	{
	public:
		explicit FMqttifyClient(const FMqttifyConnectionSettingsRef& InConnectionSettings);

		/**
		 * @brief Tick the MQTT client
		 */
		void Tick() override;

		void UpdatePassword(const FString& InPassword) override;
		~FMqttifyClient() override = default;

		// IMqttifyClient
		TFuture<TMqttifyResult<void>> ConnectAsync(bool bCleanSession) override;
		TFuture<TMqttifyResult<void>> DisconnectAsync() override;
		TFuture<TMqttifyResult<void>> PublishAsync(FMqttifyMessage&& InMessage) override;
		TFuture<TMqttifyResult<TArray<FMqttifySubscribeResult>>> SubscribeAsync(
			const TArray<FMqttifyTopicFilter>& InTopicFilters) override;
		TFuture<TMqttifyResult<FMqttifySubscribeResult>> SubscribeAsync(FMqttifyTopicFilter& InTopicFilter) override;
		TFuture<TMqttifyResult<FMqttifySubscribeResult>> SubscribeAsync(FString& InTopicFilter) override;
		TFuture<TMqttifyResult<TArray<FMqttifyUnsubscribeResult>>>
		UnsubscribeAsync(const TSet<FString>& InTopicFilters) override;
		FOnConnect& OnConnect() override;
		FOnDisconnect& OnDisconnect() override;
		FOnPublish& OnPublish() override;
		FOnSubscribe& OnSubscribe() override;
		FOnUnsubscribe& OnUnsubscribe() override;
		FOnMessage& OnMessage() override;
		const FMqttifyConnectionSettingsRef GetConnectionSettings() const override;
		bool IsConnected() const override;

	private:
		void TransitionTo(TUniquePtr<FMqttifyClientState>&& InState);

	private:
		/// @brief The current state of the MQTT client.
		TUniquePtr<FMqttifyClientState> CurrentState;
		TSharedRef<FMqttifyClientContext> Context;
		friend class FMqttifyClientState;
	};
} // namespace Mqttify
