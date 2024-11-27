#include "Mqtt/Commands/MqttifyPingReq.h"

#include "LogMqttify.h"
#include "Packets/MqttifyPacketType.h"
#include "Packets/MqttifyPingReqPacket.h"
#include "Packets/Interface/IMqttifyControlPacket.h"

namespace Mqttify
{
	bool FMqttifyPingReq::Acknowledge(const FMqttifyPacketPtr& InPacket)
	{
		FScopeLock Lock{&CriticalSection};
		LOG_MQTTIFY(VeryVerbose, TEXT("Acknowledge %s"), EnumToTCharString(InPacket->GetPacketType()));
		if (InPacket->GetPacketType() != EMqttifyPacketType::PingResp)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[PingReq (Connection %s, ClientId %s)] %s Expected: %s Actual: %s"),
				*Settings->GetHost(),
				*Settings->GetClientId(),
				MqttifyPacketType::InvalidPacketType,
				EnumToTCharString(EMqttifyPacketType::PingResp),
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

	bool FMqttifyPingReq::NextImpl()
	{
		if (bIsDone)
		{
			return true;
		}

		SendPacketInternal(MakeShared<FMqttifyPingReqPacket>());
		RetryWaitTime = FDateTime::UtcNow() + FTimespan::FromSeconds(Settings->GetKeepAliveIntervalSeconds());
		return false;
	}

	void FMqttifyPingReq::Abandon()
	{
		FScopeLock Lock{&CriticalSection};
		if (!bIsDone)
		{
			bIsDone = true;
			LOG_MQTTIFY_PACKET(
				Warning,
				TEXT("[PingReq (Connection %s, ClientId %s)] Abandoning"),
				MakeShared<FMqttifyPingReqPacket>(),
				*Settings->GetHost(),
				*Settings->GetClientId());
			SetPromiseValue(TMqttifyResult<void>{false});
		}
	}

	bool FMqttifyPingReq::IsDone() const
	{
		FScopeLock Lock{&CriticalSection};
		return bIsDone;
	}
}
