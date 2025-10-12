#include "Mqtt/Commands/MqttifyPublish.h"

#include "Mqtt/MqttifyMessage.h"
#include "Packets/MqttifyPubRelPacket.h"

namespace Mqttify
{
	TMqttifyPublish<EMqttifyQualityOfService::AtLeastOnce>::TMqttifyPublish(
		FMqttifyMessage&& InMessage,
		const uint16 InPacketId,
		const TWeakPtr<FMqttifySocketBase>& InSocket,
		const FMqttifyConnectionSettingsRef& InConnectionSettings
		)
		: TMqttifyAcknowledgeable{InPacketId, InSocket, InConnectionSettings}
		, PublishPacket{MakeShared<TMqttifyPublishPacket<GMqttifyProtocol>>(MoveTemp(InMessage), InPacketId)}
		, bIsDone{false} {}

	bool TMqttifyPublish<EMqttifyQualityOfService::AtLeastOnce>::NextImpl()
	{
		if (PacketTries == 1)
		{
			PublishPacket = PublishPacket->GetDuplicate(MoveTemp(PublishPacket));
		}

		SendPacketInternal(PublishPacket);
		return false;
	}

	void TMqttifyPublish<EMqttifyQualityOfService::AtLeastOnce>::Abandon()
	{
		FScopeLock Lock{&CriticalSection};
		if (!bIsDone)
		{
			bIsDone = true;
			LOG_MQTTIFY_PACKET(
				Error,
				TEXT("[Publish (Connection %s, ClientId %s)] Abandoning"),
				PublishPacket,
				*Settings->GetHost(),
				*Settings->GetClientId());
			SetPromiseValue(TMqttifyResult<void>{false});
		}
	}

	bool TMqttifyPublish<EMqttifyQualityOfService::AtLeastOnce>::Acknowledge(const FMqttifyPacketPtr& InPacket)
	{
		FScopeLock Lock{&CriticalSection};
		LOG_MQTTIFY(VeryVerbose, TEXT("Acknowledge %s"), EnumToTCharString(InPacket->GetPacketType()));
		if (InPacket->GetPacketType() != EMqttifyPacketType::PubAck)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[Publish (Connection %s, ClientId %s)] %s Expected: Actual: %s"),
				*Settings->GetHost(),
				*Settings->GetClientId(),
				EnumToTCharString(EMqttifyPacketType::PubAck),
				EnumToTCharString(InPacket->GetPacketType()));
			Abandon();
			return true;
		}

		if (!bIsDone)
		{
			bIsDone = true;
			SetPromiseValue(TMqttifyResult<void>{true});
		}

		return true;
	}

	bool TMqttifyPublish<EMqttifyQualityOfService::AtLeastOnce>::IsDone() const
	{
		FScopeLock Lock{&CriticalSection};
		return bIsDone;
	}

	TMqttifyPublish<EMqttifyQualityOfService::ExactlyOnce>::TMqttifyPublish(
		FMqttifyMessage&& InMessage,
		const uint16 InPacketId,
		const TWeakPtr<FMqttifySocketBase>& InSocket,
		const FMqttifyConnectionSettingsRef& InConnectionSettings
		)
		: TMqttifyAcknowledgeable{InPacketId, InSocket, InConnectionSettings}
		, PublishPacket{MakeShared<TMqttifyPublishPacket<GMqttifyProtocol>>(MoveTemp(InMessage), InPacketId)}
		, PublishState{EPublishState::Unacknowledged} {}

	bool TMqttifyPublish<EMqttifyQualityOfService::ExactlyOnce>::Acknowledge(const FMqttifyPacketPtr& InPacket)
	{
		FScopeLock Lock{&CriticalSection};
		switch (InPacket->GetPacketType())
		{
			case EMqttifyPacketType::PubRec:
				return HandlePubRec(InPacket);
			case EMqttifyPacketType::PubComp:
				return HandlePubComp(InPacket);
			default: LOG_MQTTIFY(
					Error,
					TEXT("[Publish (Connection %s, ClientId %s)] %s Expected: %s Actual: %s"),
					*Settings->GetHost(),
					*Settings->GetClientId(),
					MqttifyPacketType::InvalidPacketType,
					EnumToTCharString(EMqttifyPacketType::PubRec),
					EnumToTCharString(InPacket->GetPacketType()))				;
				Abandon();
				return true;
		}
	}

	bool TMqttifyPublish<EMqttifyQualityOfService::ExactlyOnce>::NextImpl()
	{
		switch (PublishState)
		{
			case EPublishState::Unacknowledged:
				SendPacketInternal(PublishPacket);
				return false;
			case EPublishState::Received:
				SendPacketInternal(MakeShared<TMqttifyPubRelPacket<GMqttifyProtocol>>(PacketId));
				return false;
			case EPublishState::Complete:
				return true;
		}
		return false;
	}


	bool TMqttifyPublish<EMqttifyQualityOfService::ExactlyOnce>::IsDone() const
	{
		FScopeLock Lock{&CriticalSection};
		return PublishState == EPublishState::Complete;
	}

	bool TMqttifyPublish<EMqttifyQualityOfService::ExactlyOnce>::HandlePubRec(const FMqttifyPacketPtr& InPacket)
	{
		if (PublishState == EPublishState::Complete)
		{
			return true;
		}

		if (InPacket->GetPacketId() != PacketId)
		{
			LOG_MQTTIFY(Error, TEXT("Invalid packet id. Expected:%d Actual: %d"), PacketId, InPacket->GetPacketId());
			return false;
		}

		if (PublishState == EPublishState::Unacknowledged)
		{
			PublishState = EPublishState::Received;
			SendPacketInternal(MakeShared<TMqttifyPubRelPacket<GMqttifyProtocol>>(PacketId));
			SetPromiseValue(TMqttifyResult<void>{true});
			RetryWaitTime = FDateTime::MinValue();
			PacketTries = 0;
		}

		return false;
	}

	bool TMqttifyPublish<EMqttifyQualityOfService::ExactlyOnce>::HandlePubComp(const FMqttifyPacketPtr& InPacket)
	{
		if (PublishState == EPublishState::Complete)
		{
			return true;
		}

		if (InPacket->GetPacketId() != PacketId)
		{
			LOG_MQTTIFY(Error, TEXT("Invalid packet id. Expected:%d Actual: %d"), PacketId, InPacket->GetPacketId());
			return false;
		}

		PublishState = EPublishState::Complete;
		return true;
	}

	void TMqttifyPublish<EMqttifyQualityOfService::ExactlyOnce>::Abandon()
	{
		FScopeLock Lock{&CriticalSection};
		if (PublishState != EPublishState::Complete)
		{
			PublishState = EPublishState::Complete;
			LOG_MQTTIFY_PACKET(
				Error,
				TEXT("[Publish (Connection %s, ClientId %s)] Abandoning"),
				PublishPacket,
				*Settings->GetHost(),
				*Settings->GetClientId());
			SetPromiseValue(TMqttifyResult<void>{false});
		}
	}


	TMqttifyPublish<EMqttifyQualityOfService::AtMostOnce>::TMqttifyPublish(
		FMqttifyMessage&& InMessage,
		const TWeakPtr<FMqttifySocketBase>& InSocket,
		const FMqttifyConnectionSettingsRef& InConnectionSettings
		)
		: TMqttifyQueueable{InSocket, InConnectionSettings}
		, PublishPacket{MakeShared<TMqttifyPublishPacket<GMqttifyProtocol>>(MoveTemp(InMessage), 0)}
		, bIsDone{false} {}

	bool TMqttifyPublish<EMqttifyQualityOfService::AtMostOnce>::NextImpl()
	{
		SendPacketInternal(PublishPacket);
		SetPromiseValue(TMqttifyResult<void>{true});
		bIsDone = true;
		return true;
	}

	void TMqttifyPublish<EMqttifyQualityOfService::AtMostOnce>::Abandon()
	{
		FScopeLock Lock{&CriticalSection};
		if (!bIsDone)
		{
			bIsDone = true;
			LOG_MQTTIFY_PACKET(
				Warning,
				TEXT("[Publish (Connection %s, ClientId %s)] Abandoning"),
				PublishPacket,
				*Settings->GetHost(),
				*Settings->GetClientId());
			SetPromiseValue(TMqttifyResult<void>{false});
		}
	}

	bool TMqttifyPublish<EMqttifyQualityOfService::AtMostOnce>::IsDone() const
	{
		FScopeLock Lock{&CriticalSection};
		return bIsDone;
	}
}
