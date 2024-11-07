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
			const FMqttifyConnectionSettingsRef& InConnectionSettings
			);

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
			const FMqttifyConnectionSettingsRef& InConnectionSettings
			);

		virtual void Abandon() override;
		virtual bool Acknowledge(const FMqttifyPacketPtr& InPacket) override;

	protected:
		virtual bool NextImpl() override;
		virtual bool IsDone() const override;
		bool HandlePubRec(const FMqttifyPacketPtr& InPacket);
		bool HandlePubComp(const FMqttifyPacketPtr& InPacket);
	};

	template <>
	class TMqttifyPublish<EMqttifyQualityOfService::AtMostOnce> final : public TMqttifyQueueable<void>
	{
	private:
		TSharedRef<TMqttifyPublishPacket<GMqttifyProtocol>> PublishPacket;
		bool bIsDone;

	public:
		explicit TMqttifyPublish(
			FMqttifyMessage&& InMessage,
			const TWeakPtr<FMqttifySocketBase>& InSocket,
			const FMqttifyConnectionSettingsRef& InConnectionSettings
			);

		virtual bool NextImpl() override;
		virtual void Abandon() override;

	protected:
		virtual bool IsDone() const override;
	};

	using FMqttifyPubAtLeastOnce = TMqttifyPublish<EMqttifyQualityOfService::AtLeastOnce>;
	using FMqttifyPubExactlyOnce = TMqttifyPublish<EMqttifyQualityOfService::ExactlyOnce>;
	using FMqttifyPubAtMostOnce = TMqttifyPublish<EMqttifyQualityOfService::AtMostOnce>;
	using FMqttifyPubAtLeastOnceRef = TSharedRef<FMqttifyPubAtLeastOnce>;
	using FMqttifyPubExactlyOnceRef = TSharedRef<FMqttifyPubExactlyOnce>;
	using FMqttifyPubAtMostOnceRef = TSharedRef<FMqttifyPubAtMostOnce>;
}
