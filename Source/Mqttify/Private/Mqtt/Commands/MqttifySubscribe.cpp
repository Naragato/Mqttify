#include "Mqtt/Commands/MqttifySubscribe.h"

#include "Mqtt/MqttifySubscribeResult.h"
#include "Mqtt/Commands/MqttifyAcknowledgeable.h"
#include "Packets/MqttifySubAckPacket.h"

namespace Mqttify
{
	FMqttifySubscribe::FMqttifySubscribe(
		const TArray<TTuple<FMqttifyTopicFilter, TSharedRef<FOnMessage>>>& InTopicFilters,
		const uint16 InPacketId,
		const TWeakPtr<FMqttifySocketBase>& InSocket,
		const FMqttifyConnectionSettingsRef& InConnectionSettings
		)
		: TMqttifyAcknowledgeable{InPacketId, InSocket, InConnectionSettings}
		, bIsDone{false}
		, TopicFilters{InTopicFilters}
	{
	}

	void FMqttifySubscribe::Abandon()
	{
		FScopeLock Lock{&CriticalSection};
		if (bIsDone)
		{
			return;
		}

		LOG_MQTTIFY(
				Warning,
				TEXT("(Connection %s, ClientId %s, PacketId %u) Abandoning Subscribe"),
				*Settings->GetHost(),
				*Settings->GetClientId(),
				GetId());

		bIsDone = true;
		SetPromiseValue(TMqttifyResult<TArray<FMqttifySubscribeResult>>{false, {}});
	}

	bool FMqttifySubscribe::Acknowledge(const FMqttifyPacketPtr& InPacket)
	{
		FScopeLock Lock{&CriticalSection};
		LOG_MQTTIFY(VeryVerbose, TEXT("Acknowledge %s"), EnumToTCharString(InPacket->GetPacketType()));

		if (bIsDone)
		{
			return true;
		}

		if (InPacket->GetPacketType() != EMqttifyPacketType::SubAck)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[Subscribe] %s Expected: Actual: %s"),
				MqttifyPacketType::InvalidPacketType,
				EnumToTCharString(EMqttifyPacketType::SubAck),
				EnumToTCharString(InPacket->GetPacketType()));
			Abandon();
			return true;
		}

		bIsDone = true;

		TArray<FMqttifySubscribeResult> SubscribeResults;
		if constexpr (GMqttifyProtocol == EMqttifyProtocolVersion::Mqtt_5)
		{
			const FMqttifySubAckPacket5* SubAckPacket = static_cast<const FMqttifySubAckPacket5*>(InPacket.Get());
			for (int32 i = 0; i < TopicFilters.Num(); ++i)
			{
				switch (SubAckPacket->GetReasonCodes()[i])
				{
				case EMqttifyReasonCode::Success:
				case EMqttifyReasonCode::GrantedQualityOfService1:
				case EMqttifyReasonCode::GrantedQualityOfService2:
					LOG_MQTTIFY(
						Display,
						TEXT("%s - %s, subscribed to %s"),
						*Settings->GetHost(),
						*Settings->GetClientId(),
						*TopicFilters[i].Get<0>().GetFilter());
					SubscribeResults.Add(
						FMqttifySubscribeResult{TopicFilters[i].Get<0>(), true, TopicFilters[i].Get<1>()});
					break;
				default:
					SubscribeResults.Add(FMqttifySubscribeResult{TopicFilters[i].Get<0>(), false});;
				}
			}
		}
		else
		{
			const FMqttifySubAckPacket3* SubAckPacket = static_cast<const FMqttifySubAckPacket3*>(InPacket.Get());

			if (TopicFilters.Num() != SubAckPacket->GetReturnCodes().Num())
			{
				LOG_MQTTIFY(
					Error,
					TEXT("[Subscribe  (Connection %s, ClientId %s)] Expected: %d Actual: %d"),
					*Settings->GetHost(),
					*Settings->GetClientId(),
					TopicFilters.Num(),
					SubAckPacket->GetReturnCodes().Num());
				Abandon();
				return true;
			}

			for (int32 i = 0; i < TopicFilters.Num(); ++i)
			{
				const bool bSuccess = SubAckPacket->GetReturnCodes()[i] != EMqttifySubscribeReturnCode::Failure;
				if (!bSuccess)
				{
					SubscribeResults.Add(FMqttifySubscribeResult{TopicFilters[i].Get<0>(), bSuccess});
				}
				else
				{
					LOG_MQTTIFY(
						Display,
						TEXT("%s - %s, subscribed to %s"),
						*Settings->GetHost(),
						*Settings->GetClientId(),
						*TopicFilters[i].Get<0>().GetFilter());
					SubscribeResults.Add(
						FMqttifySubscribeResult{TopicFilters[i].Get<0>(), bSuccess, TopicFilters[i].Get<1>()});
				}
			}
		}


		SetPromiseValue(TMqttifyResult{true, MoveTemp(SubscribeResults)});

		return true;
	}

	bool FMqttifySubscribe::IsDone() const
	{
		FScopeLock Lock{&CriticalSection};
		return bIsDone;
	}

	bool FMqttifySubscribe::NextImpl()
	{
		FScopeLock Lock{&CriticalSection};
		if (bIsDone)
		{
			return true;
		}

		TArray<FMqttifyTopicFilter> PacketFilters;
		PacketFilters.Reserve(TopicFilters.Num());
		for (const TTuple<FMqttifyTopicFilter, TSharedRef<FOnMessage>>& TopicFilter : TopicFilters)
		{
			PacketFilters.Add(TopicFilter.Key);
		}

		SendPacketInternal(MakeShared<TMqttifySubscribePacket<GMqttifyProtocol>>(PacketFilters, PacketId));
		return false;
	}
}
