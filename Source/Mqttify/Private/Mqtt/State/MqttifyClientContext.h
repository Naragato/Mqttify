#pragma once

#include "Async/Future.h"
#include "Containers/Queue.h"

#include "Mqtt/MqttifyConnectionSettings.h"
#include "Mqtt/MqttifyResult.h"
#include "Mqtt/Delegates/OnConnect.h"
#include "Mqtt/Delegates/OnDisconnect.h"
#include "Mqtt/Delegates/OnMessage.h"
#include "Mqtt/Delegates/OnPublish.h"
#include "Mqtt/Delegates/OnSubscribe.h"
#include "Mqtt/Delegates/OnUnsubscribe.h"
#include "Packets/Interface/IMqttifyControlPacket.h"

struct FMqttifyUnsubscribeResult;
enum class EMqttifyConnectReturnCode : uint8;
struct FMqttifySubscribeResult;
struct FMqttifyTopicFilter;
class FMqttifyConnectionSettings;

namespace Mqttify
{
	class IMqttifyControlPacket;
	class FMqttifyAcknowledgeable;

	using FAcknowledgeableCommands = TMap<uint32, TSharedRef<FMqttifyAcknowledgeable>>;

	class IMqttifyClientContext
	{
	public:
		virtual ~IMqttifyClientContext() = default;
		virtual FOnConnect& OnConnect() = 0;
		virtual FOnDisconnect& OnDisconnect() = 0;
		virtual FOnPublish& OnPublish() = 0;
		virtual FOnSubscribe& OnSubscribe() = 0;
		virtual FOnUnsubscribe& OnUnsubscribe() = 0;
		virtual FOnMessage& OnMessage() = 0;
	};

	class FMqttifyClientContext final : public IMqttifyClientContext, public TSharedFromThis<FMqttifyClientContext>
	{
	private:
		FOnConnect OnConnectDelegate{};
		FOnDisconnect OnDisconnectDelegate{};
		FOnPublish OnPublishDelegate{};
		FOnSubscribe OnSubscribeDelegate{};
		FOnUnsubscribe OnUnsubscribeDelegate{};
		FOnMessage OnMessageDelegate{};

	private:
		static constexpr uint32 kMaxCount = std::numeric_limits<uint16>::max();
		TQueue<uint16> IdPool;
		mutable FCriticalSection IdPoolCriticalSection{};

		FMqttifyConnectionSettingsRef ConnectionSettings;
		mutable FCriticalSection ConnectionSettingsCriticalSection{};

		/// @brief Delegates for when a message is received matching a subscription.
		TMap<FString, TSharedRef<FOnMessage>> OnMessageDelegates{};
		mutable FCriticalSection OnMessageDelegatesCriticalSection{};

		/// @brief Promises for when a disconnect is complete.
		TArray<TSharedPtr<TPromise<TMqttifyResult<void>>>> OnDisconnectPromises;
		mutable FCriticalSection OnDisconnectPromisesCriticalSection{};

		/// @brief Promise for when a connect is complete.
		TArray<TSharedPtr<TPromise<TMqttifyResult<void>>>> OnConnectPromises;
		mutable FCriticalSection OnConnectPromisesCriticalSection{};

		/// @brief Acknowledgeable commands.
		FAcknowledgeableCommands AcknowledgeableCommands;
		mutable FCriticalSection AcknowledgeableCommandsCriticalSection{};

	public:
		virtual ~FMqttifyClientContext() override;

		explicit FMqttifyClientContext(const FMqttifyConnectionSettingsRef& InConnectionSettings);

		/**
		 * @brief Gets the next available Id
		 * @return the next available Id or 0 if none are available (or an error occurred)
		 */
		uint16 GetNextId();

		/**
		 * @brief Releases an ID back into the pool
		 * @param Id the ID to release
		 */
		void ReleaseId(const uint16 Id);

		/**
		 * @brief Add an acknowledgeable command.
		 * @param InCommand The acknowledgeable command.
		 */
		void AddAcknowledgeableCommand(
			const TSharedRef<FMqttifyAcknowledgeable>& InCommand);

		bool HasAcknowledgeableCommand(const uint16 InPacketIdentifier) const;

		void ProcessAcknowledgeableCommands();

		/**
		 * @brief Get the ConnectionSettings.
		 * @return A SharedRef to the ConnectionSettings.
		 */
		FMqttifyConnectionSettingsRef GetConnectionSettings() const;

		/**
		 * @brief Create a promise for when the disconnect is complete.
		 * @return A SharedPtr to the promise.
		 */
		TSharedPtr<TPromise<TMqttifyResult<void>>> GetDisconnectPromise();
		/**
		 * @brief Create a promise for when the connect is complete.
		 * @return A SharedPtr to the promise.
		 */
		TSharedPtr<TPromise<TMqttifyResult<void>>> GetConnectPromise();

		/**
		 * @brief Create a promise for when the message is complete.
		 * @return A SharedPtr to the promise.
		 */
		TSharedRef<FOnMessage> GetMessageDelegate(const FString& InTopic);

		/**
		 * @brief Clear the message delegate for the given topic.
		 * @param InUnsubscribeResults The results of the unsubscribe command.
		 */
		void ClearMessageDelegates(const TSharedPtr<TArray<FMqttifyUnsubscribeResult>>& InUnsubscribeResults);

		/// @brief Complete the disconnect.
		void CompleteDisconnect();

		/// @brief Complete the connect.
		void CompleteConnect();

		/**
		 * @brief Complete the given message.
		 * @param InMessage The message to complete.
		 */
		void CompleteMessage(const FMqttifyMessage& InMessage);

		/**
		 * @brief Acknowledge the given packet.
		 * @param InPacket The packet to acknowledge.
		 */
		void Acknowledge(const FMqttifyPacketPtr& InPacket);

		/// @brief Clear all Message delegates.
		void ClearMessageDelegates();
		/// @brief Clear all Disconnect promises.
		void ClearDisconnectPromises();
		/// @brief Clear all Connect promises.
		void ClearConnectPromises();
		/// @brief Abandon all Acknowledgeable commands.
		void AbandonAcknowledgeableCommands();

		/**
		 * @brief Get the OnConnect delegate.
		 * @return The OnConnect delegate.
		 */
		virtual FOnConnect& OnConnect() override { return OnConnectDelegate; }

		/**
		 * @brief Get the OnDisconnect delegate.
		 * @return The OnDisconnect delegate.
		 */
		virtual FOnDisconnect& OnDisconnect() override { return OnDisconnectDelegate; }

		/**
		 * @brief Get the OnPublish delegate.
		 * @return The OnPublish delegate.
		 */
		virtual FOnPublish& OnPublish() override { return OnPublishDelegate; }

		/**
		 * @brief Get the OnSubscribe delegate.
		 * @return The OnSubscribe delegate.
		 */
		virtual FOnSubscribe& OnSubscribe() override { return OnSubscribeDelegate; }

		/**
		 * @brief Get the OnUnsubscribe delegate.
		 * @return The OnUnsubscribe delegate.
		 */
		virtual FOnUnsubscribe& OnUnsubscribe() override { return OnUnsubscribeDelegate; }

		/**
		 * @brief Get the OnMessage delegate.
		 * @return The OnMessage delegate.
		 */
		virtual FOnMessage& OnMessage() override { return OnMessageDelegate; }
	};
} // namespace Mqttify
