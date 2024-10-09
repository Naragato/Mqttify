#pragma once

#include "Mqtt/Commands/MqttifyAcknowledgeable.h"
#include "Packets/MqttifyPublishPacket.h"

struct FMqttifyMessage;

namespace Mqttify
{
	class FMqttifySocketBase;

	template <EMqttifyQualityOfService InQualityOfService>
	class TMqttifyPublish;

	template <>
	class TMqttifyPublish<EMqttifyQualityOfService::AtLeastOnce> final : public TMqttifyAcknowledgeable<void>
	{
	private:
		TSharedRef<TMqttifyPublishPacket<GMqttifyProtocol>> PublishPacket;
		bool bIsDone;

	public:
		explicit TMqttifyPublish(
			FMqttifyMessage&& InMessage,
			const uint16 InPacketId,
			const TWeakPtr<FMqttifySocketBase>& InSocket,
			const FMqttifyConnectionSettingsRef& InConnectionSettings);

		virtual bool NextImpl() override;
		virtual void Abandon() override;
		virtual bool Acknowledge(const FMqttifyPacketPtr& InPacket) override;

	protected:
		virtual bool IsDone() const override;
	};

	template <>
	class TMqttifyPublish<EMqttifyQualityOfService::ExactlyOnce> final : public TMqttifyAcknowledgeable<void>
	{
	private:
		enum class EPublishState : uint8
		{
			Unacknowledged,
			Received,
			Complete
		};

		TSharedRef<FMqttifyPublishPacketBase> PublishPacket;
		EPublishState PublishState;

	public:
		TMqttifyPublish(
			FMqttifyMessage&& InMessage,
			const uint16 InPacketId,
			const TWeakPtr<FMqttifySocketBase>& InSocket,
			const FMqttifyConnectionSettingsRef& InConnectionSettings);

		virtual void Abandon() override;
		virtual bool Acknowledge(const FMqttifyPacketPtr& InPacket) override;

	protected:
		virtual bool NextImpl() override;
		virtual bool IsDone() const override;
		bool HandlePubRec(const FMqttifyPacketPtr& InPacket);
		bool HandlePubComp(const FMqttifyPacketPtr& InPacket);
	};

	using FMqttifyPubAtLeastOnce = TMqttifyPublish<EMqttifyQualityOfService::AtLeastOnce>;
	using FMqttifyPubExactlyOnce = TMqttifyPublish<EMqttifyQualityOfService::ExactlyOnce>;
	using FMqttifyPubAtLeastOnceRef = TSharedRef<FMqttifyPubAtLeastOnce>;
	using FMqttifyPubExactlyOnceRef = TSharedRef<FMqttifyPubExactlyOnce>;

	/**
	 * @brief Creates a publish command reference based on the quality of service.
	 * @tparam InQualityOfService The quality of service.
	 * @param InMessage The message to publish.
	 * @param InPacketId The packet identifier.
	 * @param InSocket The socket.
	 * @param InConnectionSettings The connection settings.
	 * @return The publish command.
	 */
	template <EMqttifyQualityOfService InQualityOfService>
	auto MakeMqttifyPublish(
		FMqttifyMessage&& InMessage,
		const uint16 InPacketId,
		const TWeakPtr<FMqttifySocketBase>& InSocket,
		const FMqttifyConnectionSettingsRef& InConnectionSettings)
	{
		if constexpr (InQualityOfService == EMqttifyQualityOfService::AtLeastOnce)
		{
			return MakeShared<FMqttifyPubAtLeastOnce, ESPMode::ThreadSafe>(
				MoveTemp(InMessage),
				InPacketId,
				InSocket,
				InConnectionSettings);
		}
		else if constexpr (InQualityOfService == EMqttifyQualityOfService::ExactlyOnce)
		{
			return MakeShared<FMqttifyPubExactlyOnce, ESPMode::ThreadSafe>(
				MoveTemp(InMessage),
				InPacketId,
				InSocket,
				InConnectionSettings);
		}
		else
		{
			static_assert(InQualityOfService == EMqttifyQualityOfService::AtMostOnce, "Invalid quality of service");
		}
	}
}
