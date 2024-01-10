#include "Mqtt/State/MqttifyClientConnectingState.h"

#include "LogMqttify.h"
#include "Mqtt/State/MqttifyClientConnectedState.h"
#include "Mqtt/State/MqttifyClientDisconnectedState.h"
#include "Mqtt/State/MqttifyClientDisconnectingState.h"
#include "Packets/MqttifyConnectPacket.h"
#include "Serialization/MemoryWriter.h"

namespace Mqttify
{
	void FMqttifyClientConnectingState::OnSocketConnect(const bool bInSuccess)
	{
		bIsConnecting.store(false, std::memory_order_release);
		if (!bInSuccess)
		{
			++AttemptCount;
		}

		TryConnect();
	}

	void FMqttifyClientConnectingState::OnSocketDisconnect()
	{
		bIsConnecting.store(false, std::memory_order_release);
		++AttemptCount;
		TryConnect();
	}

	void FMqttifyClientConnectingState::OnSocketDataReceive(TSharedPtr<FArrayReader> InData)
	{
		FMqttifyPacketPtr Packet = CreatePacket(InData);

		if (Packet == nullptr)
		{
			LOG_MQTTIFY(Error, TEXT("Failed to parse packet."));
			return;
		}

		if (!Packet->IsValid())
		{
			LOG_MQTTIFY(Error, TEXT("Malformed packet."));
			TransitionTo(MakeUnique<FMqttifyClientDisconnectingState>(OnStateChanged, Context, Socket));
		}

		if (Packet->GetPacketType() != EMqttifyPacketType::ConnAck)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("Expected ConnAck packet, but got %s."),
				EnumToTCharString(Packet->GetPacketType()));
			Socket->Disconnect();
			return;
		}

		if constexpr (GMqttifyProtocol == EMqttifyProtocolVersion::Mqtt_5)
		{
			const auto ConnAckPacket =
				TUniquePtr<TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_5>>(
					static_cast<TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_5>*>(Packet.Release()));
			if (ConnAckPacket->GetReasonCode() != EMqttifyReasonCode::Success)
			{
				LOG_MQTTIFY(
					Error,
					TEXT("Failed to connect. Reason: %s."),
					EnumToTCharString(ConnAckPacket->GetReasonCode()));
				Socket->Disconnect();
				return;
			}
		}
		else if constexpr (GMqttifyProtocol == EMqttifyProtocolVersion::Mqtt_3_1_1)
		{
			const auto ConnAckPacket =
				TUniquePtr<TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>>(
					static_cast<TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>*>(Packet.Release()));
			if (ConnAckPacket->GetReasonCode() != EMqttifyConnectReturnCode::Accepted)
			{
				LOG_MQTTIFY(
					Error,
					TEXT("Failed to connect. Reason: %s."),
					EnumToTCharString(ConnAckPacket->GetReasonCode()));
				Socket->Disconnect();
				return;
			}
		}

		Context->CompleteConnect();
		TransitionTo(MakeUnique<FMqttifyClientConnectedState>(OnStateChanged, Context, Socket));
	}

	void FMqttifyClientConnectingState::TryConnect()
	{
		if (AttemptCount >= Context->GetConnectionSettings()->GetMaxConnectionRetries())
		{
			LOG_MQTTIFY(Error, TEXT("Failed to connect after %d attempts."), AttemptCount);
			Context->ClearConnectPromises();
			TransitionTo(MakeUnique<FMqttifyClientDisconnectingState>(OnStateChanged, Context, Socket));
			return;
		}

		bool Expected = false;
		if (!Socket->IsConnected() && bIsConnecting.compare_exchange_strong(Expected, true))
		{
			Socket->Connect();
			return;
		}

		const FDateTime Now = FDateTime::Now();
		if (Now - LastConnectAttempt < FTimespan::FromSeconds(
			Context->GetConnectionSettings()->GetPacketRetryIntervalSeconds()) * (AttemptCount + 1))
		{
			return;
		}
		LastConnectAttempt = Now;
		++AttemptCount;
		TMqttifyConnectPacket<GMqttifyProtocol> ConnectPacket{ Context->GetConnectionSettings()->GetClientId(),
																Context->GetConnectionSettings()->
																		GetKeepAliveIntervalSeconds(),
																Context->GetConnectionSettings()->GetUsername(),
																Context->GetConnectionSettings()->GetPassword(),
																bCleanSession,
																false };

		TArray<uint8> ActualBytes;
		FMemoryWriter Writer(ActualBytes);
		ConnectPacket.Encode(Writer);
		Socket->Send(ActualBytes.GetData(), ActualBytes.Num());
	}

	TFuture<TMqttifyResult<void>> FMqttifyClientConnectingState::ConnectAsync(bool bInCleanSession)
	{
		// if we're already connecting and we're requesting a clean session, then we can't do anything.
		// return a failed promise.
		if (bInCleanSession == true)
		{
			LOG_MQTTIFY(Error, TEXT("Cannot connect with clean session while already connecting."));
			return MakeFulfilledPromise<TMqttifyResult<void>>(TMqttifyResult<void>{ false }).GetFuture();
		}
		// We're already connecting. so just return the promise.
		const TSharedPtr<TPromise<TMqttifyResult<void>>> Promise = Context->GetConnectPromise();
		return Promise->GetFuture();
	}

	void FMqttifyClientConnectingState::Tick()
	{
		if (Socket == nullptr)
		{
			LOG_MQTTIFY(Error, TEXT("Socket is null."));
			Context->ClearConnectPromises();
			TransitionTo(MakeUnique<FMqttifyDisconnectedState>(OnStateChanged, Context));
			return;
		}

		Socket->Tick();
	}
} // namespace Mqttify
