#include "Mqtt/State/MqttifyClientConnectedState.h"

#include "LogMqttify.h"
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

		return MakeFulfilledPromise<TMqttifyResult<void>>(TMqttifyResult<void>{true}).GetFuture();
	}

	FDisconnectFuture FMqttifyClientConnectedState::DisconnectAsync()
	{
		FDisconnectFuture Future = Context->GetDisconnectPromise()->GetFuture();
		TransitionTo(MakeShared<FMqttifyClientDisconnectingState>(OnStateChanged, Context, Socket));
		return Future;
	}

	FPublishFuture FMqttifyClientConnectedState::PublishAsync(FMqttifyMessage&& InMessage)
	{
		switch (InMessage.GetQualityOfService())
		{
		case EMqttifyQualityOfService::AtMostOnce:
			{
				TMqttifyPublishPacket PublishPacket = TMqttifyPublishPacket<GMqttifyProtocol>{MoveTemp(InMessage), 0};
				TArray<uint8> ActualBytes;
				FMemoryWriter Writer(ActualBytes);
				PublishPacket.Encode(Writer);
				Socket->Send(ActualBytes.GetData(), ActualBytes.Num());
				return MakeFulfilledPromise<TMqttifyResult<void>>(TMqttifyResult<void>{true}).GetFuture();
			}
		case EMqttifyQualityOfService::AtLeastOnce:
			{
				const uint16 PacketId = Context->GetNextId();
				FMqttifyPubAtLeastOnceRef PublishCommand = MakeMqttifyPublish<EMqttifyQualityOfService::AtLeastOnce>(
					MoveTemp(InMessage),
					PacketId,
					Socket,
					Context->GetConnectionSettings());
				Context->AddAcknowledgeableCommand(PublishCommand);
				return PublishCommand->GetFuture();
			}
		case EMqttifyQualityOfService::ExactlyOnce:
			{
				const uint16 PacketId = Context->GetNextId();
				FMqttifyPubExactlyOnceRef PublishCommand = MakeMqttifyPublish<EMqttifyQualityOfService::ExactlyOnce>(
					MoveTemp(InMessage),
					PacketId,
					Socket,
					Context->GetConnectionSettings());
				Context->AddAcknowledgeableCommand(PublishCommand);
				return PublishCommand->GetFuture();
			}
		default:
			{
				ensureMsgf(false, TEXT("Invalid quality of service"));
				return MakeFulfilledPromise<TMqttifyResult<void>>(TMqttifyResult<void>{false}).GetFuture();
			}
		}
	}

	FSubscribesFuture FMqttifyClientConnectedState::SubscribeAsync(const TArray<FMqttifyTopicFilter>& InTopicFilters)
	{
		const uint16 PacketId = Context->GetNextId();

		TArray<TTuple<FMqttifyTopicFilter, TSharedRef<FOnMessage>>> TopicFilters;
		for (const FMqttifyTopicFilter& TopicFilter : InTopicFilters)
		{
			auto Tuple = MakeTuple(TopicFilter, Context->GetMessageDelegate(TopicFilter.GetFilter()));
			TopicFilters.Emplace(Tuple);
		}

		const TSharedRef<FMqttifySubscribe> SubscribeCommand = MakeShared<FMqttifySubscribe>(
			TopicFilters,
			PacketId,
			Socket,
			Context->GetConnectionSettings());

		Context->AddAcknowledgeableCommand(SubscribeCommand);
		TWeakPtr<FMqttifyClientContext> WeakContext = Context;
		return SubscribeCommand->GetFuture().Next(
			[WeakContext](const TMqttifyResult<TArray<FMqttifySubscribeResult>>& InResult)
			{
				if (InResult.HasSucceeded())
				{
					if (const TSharedPtr<FMqttifyClientContext> ContextPtr = WeakContext.Pin())
					{
						ContextPtr->OnSubscribe().Broadcast(InResult.GetResult());
					}
				}
				else
				{
					LOG_MQTTIFY(
						Warning,
						TEXT("Failed to subscribe to topic filter. Number of results: %d"),
						InResult.GetResult()->Num());
				}

				return InResult;
			});
	}

	FSubscribeFuture FMqttifyClientConnectedState::SubscribeAsync(FMqttifyTopicFilter&& InTopicFilter)
	{
		return SubscribeAsync(TArray{InTopicFilter}).Next(
			[](const TMqttifyResult<TArray<FMqttifySubscribeResult>>& InResult)
			{
				if (InResult.HasSucceeded() && InResult.GetResult()->Num() == 1)
				{
					FMqttifySubscribeResult OutResult = InResult.GetResult()->operator[](0);
					return TMqttifyResult{true, MoveTemp(OutResult)};
				}

				LOG_MQTTIFY(
					Error,
					TEXT("Failed to subscribe to topic filter. Success: %s, Number of results: %d"),
					InResult.HasSucceeded() ? TEXT("true") : TEXT("false"),
					InResult.GetResult()->Num());
				return TMqttifyResult<FMqttifySubscribeResult>{false};
			});
	}

	FSubscribeFuture FMqttifyClientConnectedState::SubscribeAsync(FString& InTopicFilter)
	{
		FMqttifyTopicFilter TopicFilter{InTopicFilter};
		return SubscribeAsync(MoveTemp(TopicFilter));
	}

	FUnsubscribesFuture FMqttifyClientConnectedState::UnsubscribeAsync(const TSet<FString>& InTopicFilters)
	{
		TArray<FMqttifyTopicFilter> TopicFilters;
		for (const FString& TopicFilter : InTopicFilters)
		{
			TopicFilters.Emplace(TopicFilter);
		}

		const uint16 PacketId = Context->GetNextId();
		const TSharedRef<FMqttifyUnsubscribe> UnsubscribeCommand = MakeShared<FMqttifyUnsubscribe>(
			TopicFilters,
			PacketId,
			Socket,
			Context->GetConnectionSettings());

		Context->AddAcknowledgeableCommand(UnsubscribeCommand);
		TWeakPtr<FMqttifyClientContext> WeakContext = Context;
		return UnsubscribeCommand->GetFuture().Next(
			[WeakContext](const TMqttifyResult<TArray<FMqttifyUnsubscribeResult>>& InResult)
			{
				if (InResult.HasSucceeded())
				{
					if (const TSharedPtr<FMqttifyClientContext> ContextPtr = WeakContext.Pin())
					{
						ContextPtr->OnUnsubscribe().Broadcast(InResult.GetResult());
						ContextPtr->ClearMessageDelegates(InResult.GetResult());
					}
				}
				else
				{
					LOG_MQTTIFY(
						Warning,
						TEXT("Failed to unsubscribe from topic filter. Number of results: %d"),
						InResult.GetResult()->Num());
				}

				return InResult;
			});
	}

	void FMqttifyClientConnectedState::Tick()
	{
		if (!Socket.IsValid())
		{
			LOG_MQTTIFY(Error, TEXT("Socket is null."));
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

				TWeakPtr<FMqttifySocketBase> WeakSocket = Socket;
				PingReqCommand->GetFuture().Next(
					[this](const TMqttifyResult<void>& InResult)
					{
						if (!InResult.HasSucceeded())
						{
							LOG_MQTTIFY(Warning, TEXT("Failed to send ping request. Reconnecting."));
							TransitionTo(
								MakeShared<FMqttifyClientConnectingState>(OnStateChanged, Context, false, Socket));
						}
					});
			}

			if (nullptr != PingReqCommand && PingReqCommand->Next())
			{
				PingReqCommand = nullptr;
			}

			Context->ProcessAcknowledgeableCommands();
		}
	}

	void FMqttifyClientConnectedState::OnReceivePacket(const TSharedPtr<FArrayReader>& InPacket)
	{
		FMqttifyPacketPtr Packet = CreatePacket(InPacket);

		if (Packet == nullptr)
		{
			LOG_MQTTIFY(Error, TEXT("Failed to parse packet."));
			return;
		}

		if (!Packet->IsValid())
		{
			LOG_MQTTIFY(Error, TEXT("Malformed packet. %s"), EnumToTCharString(Packet->GetPacketType()));
			SocketTransitionToState<FMqttifyClientConnectingState>();
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

				Context->OnMessage().Broadcast(FMqttifyPublishPacketBase::ToMqttifyMessage(MoveTemp(PublishPacket)));
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
