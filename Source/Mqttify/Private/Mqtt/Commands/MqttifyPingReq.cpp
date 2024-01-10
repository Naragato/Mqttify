#include "Mqtt/Commands/MqttifyPingReq.h"

#include "LogMqttify.h"
#include "Packets/MqttifyPacketType.h"
#include "Packets/MqttifyPingReqPacket.h"
#include "Packets/Interface/IMqttifyControlPacket.h"

namespace Mqttify
{
	bool FMqttifyPingReq::Acknowledge(const FMqttifyPacketPtr& InPacket)
	{
		FScopeLock Lock{ &CriticalSection };
		if (InPacket->GetPacketType() != EMqttifyPacketType::PingResp)
		{
			LOG_MQTTIFY(Error,
						TEXT("[PingReq] %s Expected: Actual: %s."),
						MqttifyPacketType::InvalidPacketType,
						EnumToTCharString(EMqttifyPacketType::PingResp),
						EnumToTCharString(InPacket->GetPacketType()));
			Abandon();
			return true;
		}

		if (!bIsDone)
		{
			bIsDone = true;
			SetPromiseValue(TMqttifyResult<void>{ true });
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
		return false;
	}

	void FMqttifyPingReq::Abandon()
	{
		FScopeLock Lock{ &CriticalSection };
		if (!bIsDone)
		{
			bIsDone = true;
			SetPromiseValue(TMqttifyResult<void>{ false });
		}
	}

	bool FMqttifyPingReq::IsDone() const
	{
		FScopeLock Lock{ &CriticalSection };
		return bIsDone;

	}
}
