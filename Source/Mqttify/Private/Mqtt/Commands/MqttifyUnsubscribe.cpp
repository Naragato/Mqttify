#include "Mqtt/Commands/MqttifyUnsubscribe.h"

#include "Mqtt/MqttifyUnsubscribeResult.h"
#include "Packets/MqttifySubAckPacket.h"
#include "Packets/MqttifyUnsubAckPacket.h"

namespace Mqttify
{
	FMqttifyUnsubscribe::FMqttifyUnsubscribe(
		const TArray<FMqttifyTopicFilter>& InTopicFilters,
		const uint16 InPacketId,
		const TWeakPtr<FMqttifySocketBase>& InSocket,
		const FMqttifyConnectionSettingsRef& InConnectionSettings
		)
		: TMqttifyAcknowledgeable{InPacketId, InSocket, InConnectionSettings}
		, bIsDone{false}
		, TopicFilters{InTopicFilters}
	{
	}

	void FMqttifyUnsubscribe::Abandon()
	{
		FScopeLock Lock{&CriticalSection};
		if (bIsDone)
		{
			return;
		}

		bIsDone = true;
		SetPromiseValue(TMqttifyResult<TArray<FMqttifyUnsubscribeResult>>{false, {}});
	}

	bool FMqttifyUnsubscribe::IsDone() const
	{
		FScopeLock Lock{&CriticalSection};
		return bIsDone;
	}

	bool FMqttifyUnsubscribe::Acknowledge(const FMqttifyPacketPtr& InPacket)
	{
		FScopeLock Lock{&CriticalSection};
		LOG_MQTTIFY(
			VeryVerbose,
			TEXT("[Acknowledge (Connection %s, ClientId %s)] Acknowledge %s"),
			*Settings->GetHost(),
			*Settings->GetClientId(),
			EnumToTCharString(InPacket->GetPacketType()));

		if (bIsDone)
		{
			return true;
		}

		if (InPacket == nullptr)
		{
			checkNoEntry();
			return false;
		}

		if (InPacket->GetPacketType() != EMqttifyPacketType::UnsubAck)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[Unsubscribe  (Connection %s, ClientId %s)] %s Expected: Actual: %s"),
				*Settings->GetHost(),
				*Settings->GetClientId(),
				MqttifyPacketType::InvalidPacketType,
				EnumToTCharString(EMqttifyPacketType::UnsubAck),
				EnumToTCharString(InPacket->GetPacketType()));
			Abandon();
			return true;
		}

		bIsDone = true;

		TArray<FMqttifyUnsubscribeResult> UnsubscribeResults;

		if constexpr (GMqttifyProtocol == EMqttifyProtocolVersion::Mqtt_5)
		{
			const FMqttifyUnsubAckPacket5* UnsubAckPacket = static_cast<const FMqttifyUnsubAckPacket5*>(InPacket.Get());
			for (int32 i = 0; i < TopicFilters.Num(); ++i)
			{
				switch (UnsubAckPacket->GetReasonCodes()[i])
				{
				case EMqttifyReasonCode::Success:
				case EMqttifyReasonCode::NoSubscriptionExisted:
					LOG_MQTTIFY(
						Display,
						TEXT("%s - %s, unsubscribed from %s"),
						*Settings->GetHost(),
						*Settings->GetClientId(),
						*TopicFilters[i].GetFilter());
					UnsubscribeResults.Add(FMqttifyUnsubscribeResult{TopicFilters[i], true});
					break;
				default:
					UnsubscribeResults.Add(FMqttifyUnsubscribeResult{TopicFilters[i], false});
					break;
				}
			}
		}
		else
		{
			for (int32 i = 0; i < TopicFilters.Num(); ++i)
			{
				LOG_MQTTIFY(
					Display,
					TEXT("%s - %s, unsubscribed from %s"),
					*Settings->GetHost(),
					*Settings->GetClientId(),
					*TopicFilters[i].GetFilter());
				UnsubscribeResults.Add(FMqttifyUnsubscribeResult{TopicFilters[i], true});
			}
		}

		SetPromiseValue(TMqttifyResult{true, MoveTemp(UnsubscribeResults)});
		return true;
	}

	bool FMqttifyUnsubscribe::NextImpl()
	{
		FScopeLock Lock{&CriticalSection};
		if (bIsDone)
		{
			return true;
		}

		SendPacketInternal(MakeShared<TMqttifyUnsubscribePacket<GMqttifyProtocol>>(TopicFilters, PacketId));
		return false;
	}
}
