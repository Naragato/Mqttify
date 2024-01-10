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
		: Context(InContext)
		, OnStateChanged(InOnStateChanged) {}

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

		FArrayReader& Reader                  = *InData;
		const FMqttifyFixedHeader FixedHeader = FMqttifyFixedHeader::Create(Reader);
		switch (FixedHeader.GetPacketType())
		{
			case EMqttifyPacketType::Connect:
				return MakeUnique<TMqttifyConnectPacket<GMqttifyProtocol>>(Reader, FixedHeader);
			case EMqttifyPacketType::ConnAck:
				return MakeUnique<TMqttifyConnAckPacket<GMqttifyProtocol>>(Reader, FixedHeader);
			case EMqttifyPacketType::Publish:
				return MakeUnique<TMqttifyPublishPacket<GMqttifyProtocol>>(Reader, FixedHeader);
			case EMqttifyPacketType::PubAck:
				return MakeUnique<TMqttifyPubAckPacket<GMqttifyProtocol>>(Reader, FixedHeader);
			case EMqttifyPacketType::PubRec:
				return MakeUnique<TMqttifyPubRecPacket<GMqttifyProtocol>>(Reader, FixedHeader);
			case EMqttifyPacketType::PubRel:
				return MakeUnique<TMqttifyPubRelPacket<GMqttifyProtocol>>(Reader, FixedHeader);
			case EMqttifyPacketType::PubComp:
				return MakeUnique<TMqttifyPubCompPacket<GMqttifyProtocol>>(Reader, FixedHeader);
			case EMqttifyPacketType::Subscribe:
				return MakeUnique<TMqttifySubscribePacket<GMqttifyProtocol>>(Reader, FixedHeader);
			case EMqttifyPacketType::SubAck:
				return MakeUnique<TMqttifySubAckPacket<GMqttifyProtocol>>(Reader, FixedHeader);
			case EMqttifyPacketType::Unsubscribe:
				return MakeUnique<TMqttifyUnsubscribePacket<GMqttifyProtocol>>(Reader, FixedHeader);
			case EMqttifyPacketType::UnsubAck:
				return MakeUnique<TMqttifyUnsubAckPacket<GMqttifyProtocol>>(Reader, FixedHeader);
			case EMqttifyPacketType::PingReq:
				return MakeUnique<FMqttifyPingReqPacket>(FixedHeader);
			case EMqttifyPacketType::PingResp:
				return MakeUnique<FMqttifyPingRespPacket>(FixedHeader);
			case EMqttifyPacketType::Disconnect:
				return MakeUnique<TMqttifyDisconnectPacket<GMqttifyProtocol>>(Reader, FixedHeader);
			case EMqttifyPacketType::Auth:
				if constexpr (GMqttifyProtocol == EMqttifyProtocolVersion::Mqtt_5)
				{
					return MakeUnique<FMqttifyAuthPacket>(Reader, FixedHeader);
				}
			case EMqttifyPacketType::Max:
			case EMqttifyPacketType::None:
			default:
				checkNoEntry();
				LOG_MQTTIFY(Error,
							TEXT("Unknown packet type: %s, for protocol: %s."),
							EnumToTCharString(GMqttifyProtocol),
							EnumToTCharString(FixedHeader.GetPacketType()));
				return nullptr;
		}

		checkNoEntry();
		return nullptr;
	}
} // namespace Mqttify
