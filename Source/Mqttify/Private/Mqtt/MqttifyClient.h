#pragma once

#include "Async/Future.h"
#include "Interface/ITickableMqttifyClient.h"
#include "Mqtt/MqttifyTopicFilter.h"
#include "Mqtt/Interface/IMqttifyClient.h"
#include "Socket/Interface/MqttifySocketBase.h"
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
		virtual ~FMqttifyClient() override = default;

		// ITickableMqttifyClient
		/**
		 * @brief Tick the MQTT client
		 */
		virtual void Tick() override;
		// ~ITickableMqttifyClient

		// IMqttifyPublishableAsync
		virtual FPublishFuture PublishAsync(FMqttifyMessage&& InMessage) override;
		// ~IMqttifyPublishableAsync

		// IMqttifySubscribableAsync
		virtual FSubscribesFuture SubscribeAsync(const TArray<FMqttifyTopicFilter>& InTopicFilters) override;
		virtual FSubscribeFuture SubscribeAsync(FMqttifyTopicFilter&& InTopicFilter) override;
		virtual FSubscribeFuture SubscribeAsync(FString& InTopicFilter) override;
		// ~IMqttifySubscribableAsync

		// IMqttifyUnsubscribableAsync
		virtual FUnsubscribesFuture UnsubscribeAsync(const TSet<FString>& InTopicFilters) override;
		// ~IMqttifyUnsubscribableAsync

		// IMqttifyClient
		virtual FOnConnect& OnConnect() override;
		virtual FOnDisconnect& OnDisconnect() override;
		virtual FOnPublish& OnPublish() override;
		virtual FOnSubscribe& OnSubscribe() override;
		virtual FOnUnsubscribe& OnUnsubscribe() override;
		virtual FOnMessage& OnMessage() override;
		virtual const FMqttifyConnectionSettingsRef GetConnectionSettings() const override;
		virtual bool IsConnected() const override;
		// ~IMqttifyClient

		// IMqttifyConnectableAsync
		virtual FConnectFuture ConnectAsync(bool bCleanSession) override;
		// ~IMqttifyConnectableAsync

		// IMqttifyDisconnectableAsync
		virtual FDisconnectFuture DisconnectAsync() override;
		// ~IMqttifyDisconnectableAsync
	private:
		void TransitionTo(const TSharedPtr<FMqttifyClientState>& InState);

		// FMqttifySocketBase Callbacks
		void OnSocketConnect(bool bWasSuccessful) const;
		void OnSocketDisconnect() const;
		void OnReceivePacket(const TSharedPtr<FArrayReader>& InPacket) const;
		// ~FMqttifySocketBase Callbacks
	private:
		/// @brief The current state of the MQTT client.
		TSharedPtr<FMqttifyClientState> CurrentState;
		TSharedRef<FMqttifyClientContext> Context;
		FMqttifySocketPtr Socket;
		friend class FMqttifyClientState;
		mutable FCriticalSection StateLock;
	};
} // namespace Mqttify
