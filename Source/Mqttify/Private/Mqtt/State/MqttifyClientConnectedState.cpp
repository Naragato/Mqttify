#include "Mqtt/State/MqttifyClientConnectedState.h"

#include "LogMqttify.h"
#include "MqttifyAsync.h"
#include "Mqtt/Commands/MqttifyPublish.h"
#include "Mqtt/Commands/MqttifyPubRec.h"
#include "Mqtt/Commands/MqttifySubscribe.h"
#include "Mqtt/Commands/MqttifyUnsubscribe.h"
#include "Mqtt/State/MqttifyClientConnectingState.h"
#include "Mqtt/State/MqttifyClientDisconnectedState.h"
#include "Mqtt/State/MqttifyClientDisconnectingState.h"
#include "Packets/MqttifyPubAckPacket.h"
#include "Packets/MqttifyPublishPacket.h"

namespace Mqttify
{
	void FMqttifyClientConnectedState::OnSocketDisconnect()
	{
		SocketTransitionToState<FMqttifyClientConnectingState>();
	}

	FConnectFuture FMqttifyClientConnectedState::ConnectAsync(bool bCleanSession)
	{
		if (bCleanSession)
		{
			TransitionTo(MakeShared<FMqttifyClientConnectingState>(OnStateChanged, Context, bCleanSession, Socket));
			return Context->GetConnectPromise()->GetFuture();
		}

		return MakeThreadAwareFulfilledPromise<TMqttifyResult<void>>(TMqttifyResult<void>{true});
	}

	FDisconnectFuture FMqttifyClientConnectedState::DisconnectAsync()
	{
		FDisconnectFuture Future = Context->GetDisconnectPromise()->GetFuture();
		TransitionTo(MakeShared<FMqttifyClientDisconnectingState>(OnStateChanged, Context, Socket));
		return Future;
	}

	void FMqttifyClientConnectedState::Tick()
	{
		if (!Socket.IsValid())
		{
			LOG_MQTTIFY(Error, TEXT("Socket is null"));
			TransitionTo(MakeShared<FMqttifyClientDisconnectedState>(OnStateChanged, Context, Socket));
			return;
		}

		if (!Socket->IsConnected())
		{
			TransitionTo(MakeShared<FMqttifyClientConnectingState>(OnStateChanged, Context, Socket));
			return;
		}

		Socket->Tick();

		if (Context->GetConnectionSettings()->GetKeepAliveIntervalSeconds() > 0)
		{
			const FDateTime Now = FDateTime::UtcNow();
			const FDateTime KeepAliveEndTime = LastPingTime + FTimespan::FromSeconds(
				Context->GetConnectionSettings()->GetKeepAliveIntervalSeconds());
			if (PingReqCommand == nullptr && KeepAliveEndTime < Now)
			{
				LastPingTime = Now;
				PingReqCommand = MakeShared<FMqttifyPingReq>(Socket, Context->GetConnectionSettings());

				TWeakPtr<FMqttifyClientConnectedState> WeakConnectedPtr = AsWeak();
				PingReqCommand->GetFuture().Next(
					[this, WeakConnectedPtr](const TMqttifyResult<void>& InResult)
					{
						const TSharedPtr<FMqttifyClientConnectedState> SharedConnectedPtr = WeakConnectedPtr.Pin();
						if (!InResult.HasSucceeded() && SharedConnectedPtr.IsValid())
						{
							LOG_MQTTIFY(Warning, TEXT("Failed to send ping request. Reconnecting"));
							SharedConnectedPtr->TransitionTo(
								MakeShared<FMqttifyClientConnectingState>(OnStateChanged, Context, false, Socket));
						}
					});
			}

			if (nullptr != PingReqCommand && PingReqCommand->Next())
			{
				PingReqCommand = nullptr;
			}

			Context->ProcessCommands();
		}
	}

	void FMqttifyClientConnectedState::OnReceivePacket(const TSharedPtr<FArrayReader>& InPacket)
	{
		FMqttifyPacketPtr Packet = CreatePacket(InPacket);

		if (Packet == nullptr)
		{
			LOG_MQTTIFY(Error, TEXT("Failed to parse packet"));
			return;
		}

		if (!Packet->IsValid())
		{
			LOG_MQTTIFY(Error, TEXT("Malformed packet. %s"), EnumToTCharString(Packet->GetPacketType()));
			SocketTransitionToState<FMqttifyClientConnectingState>();
			return;
		}

		switch (Packet->GetPacketType())
		{
		case EMqttifyPacketType::PingResp:
			{
				if (PingReqCommand != nullptr && PingReqCommand->Acknowledge(Packet))
				{
					PingReqCommand = nullptr;
				}
				break;
			}
		case EMqttifyPacketType::PubAck:
		case EMqttifyPacketType::PubRec:
		case EMqttifyPacketType::PubComp:
		case EMqttifyPacketType::SubAck:
		case EMqttifyPacketType::UnsubAck:
			Context->Acknowledge(Packet);
			break;

		case EMqttifyPacketType::Publish:
			{
				const uint32 ServerId = static_cast<uint32>(Packet->GetPacketId()) << 16;
				TUniquePtr<FMqttifyPublishPacketBase> PublishPacket = TUniquePtr<FMqttifyPublishPacketBase>(
					static_cast<FMqttifyPublishPacketBase*>(Packet.Release()));

				if (PublishPacket->GetIsDuplicate() && Context->HasAcknowledgeableCommand(ServerId))
				{
					return;
				}

				// ReSharper disable once CppIncompleteSwitchStatement
				// ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
				switch (PublishPacket->GetQualityOfService())
				{
				case EMqttifyQualityOfService::AtLeastOnce:
					{
						TMqttifyPubAckPacket<GMqttifyProtocol> PubAckPacket{PublishPacket->GetPacketId()};
						TArray<uint8> ActualBytes;
						FMemoryWriter Writer(ActualBytes);
						PubAckPacket.Encode(Writer);
						Socket->Send(ActualBytes.GetData(), ActualBytes.Num());
						break;
					}
				case EMqttifyQualityOfService::ExactlyOnce:
					{
						const auto PubRec = MakeShared<FMqttifyPubRec>(
							PublishPacket->GetPacketId(),
							EMqttifyReasonCode::Success,
							Socket,
							Context->GetConnectionSettings());
						Context->AddAcknowledgeableCommand(PubRec);
						break;
					}
				}

				Context->CompleteMessage(FMqttifyPublishPacketBase::ToMqttifyMessage(MoveTemp(PublishPacket)));
				break;
			}

		case EMqttifyPacketType::PubRel:
			{
				Context->Acknowledge(Packet);
				break;
			}
		default: ;
		}
	}
} // namespace Mqttify
