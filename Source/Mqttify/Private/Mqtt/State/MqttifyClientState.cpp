#include "Mqtt/State/MqttifyClientState.h"

#include "LogMqttify.h"
#include "Packets/MqttifyAuthPacket.h"
#include "Packets/MqttifyConnAckPacket.h"
#include "Packets/MqttifyConnectPacket.h"
#include "Packets/MqttifyDisconnectPacket.h"
#include "Packets/MqttifyPingReqPacket.h"
#include "Packets/MqttifyPingRespPacket.h"
#include "Packets/MqttifyPubAckPacket.h"
#include "Packets/MqttifyPubCompPacket.h"
#include "Packets/MqttifyPublishPacket.h"
#include "Packets/MqttifyPubRecPacket.h"
#include "Packets/MqttifyPubRelPacket.h"
#include "Packets/MqttifySubAckPacket.h"
#include "Packets/MqttifySubscribePacket.h"
#include "Packets/MqttifyUnsubAckPacket.h"
#include "Packets/MqttifyUnsubscribePacket.h"

namespace Mqttify
{
	FMqttifyClientState::FMqttifyClientState(const FOnStateChangedDelegate& InOnStateChanged,
											const TSharedRef<FMqttifyClientContext>& InContext)
		: Context(InContext), OnStateChanged(InOnStateChanged)
	{
	}

	void FMqttifyClientState::TransitionTo(TUniquePtr<FMqttifyClientState>&& InState)
	{
		FScopeLock Lock(&StateTransitionLock);
		OnStateChanged.ExecuteIfBound(MoveTemp(InState));
	}

	FMqttifyPacketPtr FMqttifyClientState::CreatePacket(const TSharedPtr<FArrayReader>& InData)
	{
		if (InData == nullptr)
		{
			LOG_MQTTIFY(Error, TEXT("Failed to parse packet."));
			return nullptr;
		}

		FArrayReader& Reader = *InData;
		const FMqttifyFixedHeader FixedHeader = FMqttifyFixedHeader::Create(Reader);
		FMqttifyPacketPtr Result = nullptr;
		switch (FixedHeader.GetPacketType())
		{
		case EMqttifyPacketType::Connect:
			Result = MakeUnique<TMqttifyConnectPacket<GMqttifyProtocol>>(Reader, FixedHeader);
			break;
		case EMqttifyPacketType::ConnAck:
			Result = MakeUnique<TMqttifyConnAckPacket<GMqttifyProtocol>>(Reader, FixedHeader);
			break;
		case EMqttifyPacketType::Publish:
			Result = MakeUnique<TMqttifyPublishPacket<GMqttifyProtocol>>(Reader, FixedHeader);
			break;
		case EMqttifyPacketType::PubAck:
			Result = MakeUnique<TMqttifyPubAckPacket<GMqttifyProtocol>>(Reader, FixedHeader);
			break;
		case EMqttifyPacketType::PubRec:
			Result = MakeUnique<TMqttifyPubRecPacket<GMqttifyProtocol>>(Reader, FixedHeader);
			break;
		case EMqttifyPacketType::PubRel:
			Result = MakeUnique<TMqttifyPubRelPacket<GMqttifyProtocol>>(Reader, FixedHeader);
			break;
		case EMqttifyPacketType::PubComp:
			Result = MakeUnique<TMqttifyPubCompPacket<GMqttifyProtocol>>(Reader, FixedHeader);
			break;
		case EMqttifyPacketType::Subscribe:
			Result = MakeUnique<TMqttifySubscribePacket<GMqttifyProtocol>>(Reader, FixedHeader);
			break;
		case EMqttifyPacketType::SubAck:
			Result = MakeUnique<TMqttifySubAckPacket<GMqttifyProtocol>>(Reader, FixedHeader);
			break;
		case EMqttifyPacketType::Unsubscribe:
			Result = MakeUnique<TMqttifyUnsubscribePacket<GMqttifyProtocol>>(Reader, FixedHeader);
			break;
		case EMqttifyPacketType::UnsubAck:
			Result = MakeUnique<TMqttifyUnsubAckPacket<GMqttifyProtocol>>(Reader, FixedHeader);
			break;
		case EMqttifyPacketType::PingReq:
			Result = MakeUnique<FMqttifyPingReqPacket>(FixedHeader);
			break;
		case EMqttifyPacketType::PingResp:
			Result = MakeUnique<FMqttifyPingRespPacket>(FixedHeader);
			break;
		case EMqttifyPacketType::Disconnect:
			Result = MakeUnique<TMqttifyDisconnectPacket<GMqttifyProtocol>>(Reader, FixedHeader);
			break;
		case EMqttifyPacketType::Auth:
			if constexpr (GMqttifyProtocol == EMqttifyProtocolVersion::Mqtt_5)
			{
				Result = MakeUnique<FMqttifyAuthPacket>(Reader, FixedHeader);
				break;
			}
		case EMqttifyPacketType::Max:
		case EMqttifyPacketType::None:
		default:
			checkNoEntry();
			LOG_MQTTIFY(Error,
						TEXT("Unknown packet type: %s, for protocol: %s."),
						EnumToTCharString(GMqttifyProtocol),
						EnumToTCharString(FixedHeader.GetPacketType()));
			Result = nullptr;
			break;
		}

		LOG_MQTTIFY(VeryVerbose,
					TEXT("Created packet: %s, %d"),
					EnumToTCharString(Result->GetPacketType()),
					Result->GetPacketId());
		return Result;
	}
} // namespace Mqttify
