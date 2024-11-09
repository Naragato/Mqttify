#include "Mqtt/Commands/MqttifyPubRec.h"

#include "Packets/MqttifyPubCompPacket.h"
#include "Packets/MqttifyPubRecPacket.h"

namespace Mqttify
{
	using FPubRecPacket = TMqttifyPubRecPacket<GMqttifyProtocol>;
	using FPubCompPacket = TMqttifyPubCompPacket<GMqttifyProtocol>;

	FMqttifyPubRec::FMqttifyPubRec(
		const uint16 InPacketId,
		const EMqttifyReasonCode InReasonCode,
		const TWeakPtr<FMqttifySocketBase>& InSocket,
		const FMqttifyConnectionSettingsRef& InConnectionSettings
		)
		: TMqttifyAcknowledgeable{InPacketId, InSocket, InConnectionSettings}
		, ReasonCode{InReasonCode}
		, PubRecState{EPubRecState::Unacknowledged}
	{
	}

	void FMqttifyPubRec::Abandon()
	{
		FScopeLock Lock{&CriticalSection};
		if (PubRecState == EPubRecState::Unacknowledged)
		{
			LOG_MQTTIFY(
				Warning,
				TEXT("(Connection %s, ClientId %s, PacketId %u) Abandoning"),
				*Settings->GetHost(),
				*Settings->GetClientId(),
				GetId());
			SetPromiseValue(TMqttifyResult<void>{false});
		}
		PubRecState = EPubRecState::Complete;
	}

	bool FMqttifyPubRec::Acknowledge(const FMqttifyPacketPtr& InPacket)
	{
		FScopeLock Lock{&CriticalSection};
		LOG_MQTTIFY(
			VeryVerbose,
			TEXT("(Connection %s, ClientId %s) Acknowledge %s"),
			*Settings->GetHost(),
			*Settings->GetClientId(),
			EnumToTCharString(InPacket->GetPacketType()));
		if (PubRecState == EPubRecState::Unacknowledged)
		{
			PubRecState = EPubRecState::Released;
			SetPromiseValue(TMqttifyResult<void>{true});
			RetryWaitTime = FDateTime::MinValue();
			PacketTries = 0;
		}

		return false;
	}

	bool FMqttifyPubRec::NextImpl()
	{
		TSharedPtr<IMqttifyControlPacket> Packet;
		switch (PubRecState)
		{
		case EPubRecState::Unacknowledged:
			Packet = GetPacket<FPubRecPacket>();
			break;

		case EPubRecState::Released:
			Packet = GetPacket<FPubCompPacket>();
			PubRecState = EPubRecState::Complete;
			break;
		case EPubRecState::Complete:
			return true;
		}

		SendPacketInternal(Packet);
		return PubRecState == EPubRecState::Complete;
	}

	bool FMqttifyPubRec::IsDone() const
	{
		FScopeLock Lock{&CriticalSection};
		return PubRecState == EPubRecState::Complete;
	}
}
