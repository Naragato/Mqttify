#include "Mqtt/State/MqttifyClientConnectingState.h"

#include "LogMqttify.h"
#include "Mqtt/State/MqttifyClientConnectedState.h"
#include "Mqtt/State/MqttifyClientDisconnectedState.h"
#include "Mqtt/State/MqttifyClientDisconnectingState.h"
#include "Packets/MqttifyConnectPacket.h"
#include "Serialization/MemoryWriter.h"

namespace Mqttify
{
	void FMqttifyClientConnectingState::TryConnect()
	{
		if (AttemptCount >= Context->GetConnectionSettings()->GetMaxConnectionRetries())
		{
			LOG_MQTTIFY(Error, TEXT("Failed to connect after %d attempts"), AttemptCount);
			Context->ClearConnectPromises();
			SocketTransitionToState<FMqttifyClientDisconnectingState>();
			return;
		}

		bool Expected = false;
		if (!Socket->IsConnected() && bIsConnecting.compare_exchange_strong(Expected, true))
		{
			LOG_MQTTIFY(Verbose, TEXT("Connecting to %s."), *Context->GetConnectionSettings()->ToString());
			Socket->Disconnect();
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

		TArray<FMqttifyProperty> PropertyArr;
		const uint32 SessionExpiryInterval = Context->GetConnectionSettings()->GetSessionExpiryInterval();
		if (SessionExpiryInterval > 0)
		{
			PropertyArr.Add(
				{ 
					FMqttifyProperty::Create<EMqttifyPropertyIdentifier::SessionExpiryInterval>(static_cast<uint32>(SessionExpiryInterval)) 
				});
		}

		const FMqttifyCredentialsProviderRef Credentials = Context->GetConnectionSettings()->GetCredentialsProvider();
		const auto ConnectPacketRef = MakeShared<TMqttifyConnectPacket<GMqttifyProtocol>>(
			Context->GetConnectionSettings()->GetClientId(),
			Context->GetConnectionSettings()->GetKeepAliveIntervalSeconds(),
			Credentials->GetCredentials().Username,
			Credentials->GetCredentials().Password,
			bCleanSession,
			false,
			FString(),
			FString(),
			EMqttifyQualityOfService::AtMostOnce,
			FMqttifyProperties(),
			FMqttifyProperties(PropertyArr));

		Socket->Send(ConnectPacketRef);
	}

	FConnectFuture FMqttifyClientConnectingState::ConnectAsync(bool bInCleanSession)
	{
		// if we're already connecting, and we're requesting a clean session, then we can't do anything.
		// return a failed promise.
		if (bInCleanSession == true)
		{
			LOG_MQTTIFY(Error, TEXT("Cannot connect with clean session while already connecting."));
			return MakeThreadAwareFulfilledPromise<TMqttifyResult<void>>(TMqttifyResult<void>{false});
		}
		// We're already connecting. so just return the promise.
		const TSharedPtr<TPromise<TMqttifyResult<void>>> Promise = Context->GetConnectPromise();
		return Promise->GetFuture();
	}

	void FMqttifyClientConnectingState::OnSocketConnect(const bool bInSuccess)
	{
		bIsConnecting.store(false, std::memory_order_release);
		if (!bInSuccess)
		{
			LOG_MQTTIFY(Error, TEXT("Failed to connect to %s."), *Context->GetConnectionSettings()->ToString());
			++AttemptCount;
		}

		TryConnect();
	}

	void FMqttifyClientConnectingState::Tick()
	{
		Socket->Tick();
	}

	FDisconnectFuture FMqttifyClientConnectingState::DisconnectAsync()
	{
		FDisconnectFuture Future = Context->GetDisconnectPromise()->GetFuture();
		TransitionTo(MakeShared<FMqttifyClientDisconnectingState>(OnStateChanged, Context, Socket));
		return Future;
	}

	void FMqttifyClientConnectingState::OnSocketDisconnect()
	{
		LOG_MQTTIFY(Verbose, TEXT("Disconnected from %s."), *Context->GetConnectionSettings()->ToString());
		bIsConnecting.store(false, std::memory_order_release);
		++AttemptCount;
		TryConnect();
	}

	void FMqttifyClientConnectingState::OnReceivePacket(const TSharedPtr<FArrayReader>& InPacket)
	{
		FMqttifyPacketPtr Packet = CreatePacket(InPacket);

		if (Packet == nullptr)
		{
			LOG_MQTTIFY(Error, TEXT("Failed to parse packet."));
			return;
		}

		if (!Packet->IsValid())
		{
			LOG_MQTTIFY(Error, TEXT("Malformed packet."));
			SocketTransitionToState<FMqttifyClientDisconnectingState>();
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
			const auto ConnAckPacket = TUniquePtr<TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_5>>(
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
			else
			{
				LOG_MQTTIFY(
					Log,
					TEXT("Mqtt_5 connect success. GetSessionPresent(): %s."),
					ConnAckPacket->GetSessionPresent() ? TEXT("TRUE") : TEXT("FALSE"));
			}
		}
		else if constexpr (GMqttifyProtocol == EMqttifyProtocolVersion::Mqtt_3_1_1)
		{
			const auto ConnAckPacket = TUniquePtr<TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>>(
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

		const TSharedRef<FMqttifyClientConnectingState> StrongThis = AsShared();
		StrongThis->SocketTransitionToState<FMqttifyClientConnectedState>();
		StrongThis->Context->CompleteConnect();
	}
} // namespace Mqttify
