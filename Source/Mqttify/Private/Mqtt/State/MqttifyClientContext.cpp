#include "MqttifyClientContext.h"

#include "Mqtt/MqttifyResult.h"
#include "Mqtt/Commands/MqttifyAcknowledgeable.h"
#include "Packets/Interface/IMqttifyControlPacket.h"

namespace Mqttify
{
	FMqttifyClientContext::~FMqttifyClientContext()
	{
		ClearMessageDelegates();
		ClearDisconnectPromises();
		ClearConnectPromises();
		AbandonAcknowledgeableCommands();
	}

	FMqttifyClientContext::FMqttifyClientContext(const FMqttifyConnectionSettingsRef& InConnectionSettings)
		: ConnectionSettings{ InConnectionSettings }
	{

		for (uint16 i = 1; i < kMaxCount; ++i)
		{
			IdPool.Enqueue(i);
		}
	}

	uint16 FMqttifyClientContext::GetNextId()
	{
		FScopeLock Lock(&IdPoolCriticalSection);
		uint16 Id;
		if (!IdPool.Dequeue(Id))
		{
			UE_LOG(LogTemp, Warning, TEXT("No more IDs available!"));
			Id = 0; // or any other error indication
		}
		return Id;
	}

	void FMqttifyClientContext::ReleaseId(const uint16 Id)
	{
		FScopeLock Lock(&IdPoolCriticalSection);
		if (Id > 0 && Id <= kMaxCount)
		{
			IdPool.Enqueue(Id);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid ID!"));
		}
	}

	void FMqttifyClientContext::UpdatePassword(const FString& InPassword)
	{
		FScopeLock Lock(&ConnectionSettingsCriticalSection);
		ConnectionSettings = ConnectionSettings->FromNewPassword(InPassword);
	}

	void FMqttifyClientContext::AddAcknowledgeableCommand(
		const TSharedRef<FMqttifyAcknowledgeable>& InCommand)
	{
		FScopeLock Lock(&AcknowledgeableCommandsCriticalSection);
		AcknowledgeableCommands.Add(InCommand->GetKey(), InCommand);
	}

	bool FMqttifyClientContext::HasAcknowledgeableCommand(const uint16 InPacketIdentifier) const
	{
		FScopeLock Lock(&AcknowledgeableCommandsCriticalSection);
		return AcknowledgeableCommands.Contains(InPacketIdentifier);
	}

	void FMqttifyClientContext::ProcessAcknowledgeableCommands()
	{
		// TODO SB - Come back to this, since we have the option of running on the game thread
		// and the loop may block while writing to the socket (which is not ideal). If there
		// is a large number of commands to process, we may want to process them in batches
		// and set a limit to the number of commands we have pending
		FScopeLock Lock(&AcknowledgeableCommandsCriticalSection);
		for (auto It = AcknowledgeableCommands.CreateIterator(); It; ++It)
		{
			if (It.Value()->Next())
			{
				It.RemoveCurrent();
			}
		}
	}

	FMqttifyConnectionSettingsRef FMqttifyClientContext::GetConnectionSettings() const
	{
		FScopeLock Lock(&ConnectionSettingsCriticalSection);
		return ConnectionSettings;
	}

	TSharedPtr<TPromise<TMqttifyResult<void>>> FMqttifyClientContext::GetDisconnectPromise()
	{
		FScopeLock Lock(&OnDisconnectPromisesCriticalSection);
		const TSharedPtr<TPromise<TMqttifyResult<void>>> Promise = MakeShared<TPromise<TMqttifyResult<void>>>();
		OnDisconnectPromises.Add(Promise);
		return Promise;
	}

	TSharedPtr<TPromise<TMqttifyResult<void>>> FMqttifyClientContext::GetConnectPromise()
	{
		FScopeLock Lock(&OnConnectPromisesCriticalSection);
		const TSharedPtr<TPromise<TMqttifyResult<void>>> Promise = MakeShared<TPromise<TMqttifyResult<void>>>();
		OnConnectPromises.Add(Promise);
		return Promise;
	}

	TSharedRef<FOnMessage> FMqttifyClientContext::GetMessageDelegate(const FString& InTopic)
	{
		FScopeLock Lock(&OnMessageDelegatesCriticalSection);
		if (const TSharedRef<FOnMessage>* Delegate = OnMessageDelegates.Find(InTopic))
		{
			return *Delegate;
		}
		const TSharedRef<FOnMessage> Delegate = MakeShared<FOnMessage>();
		OnMessageDelegates.Add(InTopic, Delegate);
		return Delegate;
	}

	void FMqttifyClientContext::ClearMessageDelegates(
		const TSharedPtr<TArray<FMqttifyUnsubscribeResult>>& InUnsubscribeResults)
	{
		FScopeLock Lock(&OnMessageDelegatesCriticalSection);
		for (FMqttifyUnsubscribeResult& Result : *InUnsubscribeResults)
		{
			if (const TSharedRef<FOnMessage>* Delegate = OnMessageDelegates.Find(Result.GetFilter().GetFilter()))
			{
				(*Delegate)->Clear();
				OnMessageDelegates.Remove(Result.GetFilter().GetFilter());
			}
		}
	}

	void FMqttifyClientContext::CompleteDisconnect()
	{
		FScopeLock Lock(&OnDisconnectPromisesCriticalSection);
		OnDisconnect().Broadcast(true);
		for (const auto& Promise : OnDisconnectPromises)
		{
			Promise->SetValue(TMqttifyResult<void>{ true });
		}
		OnDisconnectPromises.Empty();
	}

	void FMqttifyClientContext::CompleteConnect()
	{
		FScopeLock Lock(&OnConnectPromisesCriticalSection);
		OnConnect().Broadcast(true);
		for (const auto& Promise : OnConnectPromises)
		{
			Promise->SetValue(TMqttifyResult<void>{ true });
		}
		OnConnectPromises.Empty();
	}

	void FMqttifyClientContext::CompleteMessage(const FMqttifyMessage& InMessage)
	{
		FScopeLock Lock(&OnMessageDelegatesCriticalSection);
		OnMessage().Broadcast(InMessage);

		if (const TSharedRef<FOnMessage>* Delegate = OnMessageDelegates.Find(InMessage.GetTopic()))
		{
			(*Delegate)->Broadcast(InMessage);
		}
	}

	void FMqttifyClientContext::Acknowledge(const FMqttifyPacketPtr& InPacket)
	{
		FScopeLock Lock(&AcknowledgeableCommandsCriticalSection);

		// The reason for the bit shift is we are using the first 16 bits for our packet identifier
		// and the last 16 bits for the identifiers generated by the broker.
		const uint32 InPacketIdentifier = InPacket->GetPacketType() != EMqttifyPacketType::PubRel
			? InPacket->GetPacketId()
			: static_cast<uint32>(InPacket->GetPacketId()) << 16;;

		if (!InPacketIdentifier)
		{
			return;
		}

		if (const TSharedRef<FMqttifyAcknowledgeable>* Command = AcknowledgeableCommands.Find(InPacketIdentifier))
		{
			if ((*Command)->Acknowledge(InPacket))
			{
				AcknowledgeableCommands.Remove(InPacketIdentifier);
				if (InPacketIdentifier <= kMaxCount)
				{
					ReleaseId(InPacket->GetPacketId());
				}
			}
		}
	}

	void FMqttifyClientContext::ClearMessageDelegates()
	{
		FScopeLock Lock(&OnMessageDelegatesCriticalSection);
		for (const auto& Pair : OnMessageDelegates)
		{
			Pair.Value->Clear();
		}
		OnMessageDelegates.Empty();
	}

	void FMqttifyClientContext::ClearDisconnectPromises()
	{
		FScopeLock Lock(&OnDisconnectPromisesCriticalSection);
		for (const auto& Promise : OnDisconnectPromises)
		{
			Promise->SetValue(TMqttifyResult<void>{ false });
		}
		OnDisconnectPromises.Empty();
	}

	void FMqttifyClientContext::ClearConnectPromises()
	{
		FScopeLock Lock(&OnConnectPromisesCriticalSection);
		for (const auto& Promise : OnConnectPromises)
		{
			Promise->SetValue(TMqttifyResult<void>{ false });
		}
		OnConnectPromises.Empty();
	}

	void FMqttifyClientContext::AbandonAcknowledgeableCommands()
	{
		FScopeLock Lock(&AcknowledgeableCommandsCriticalSection);
		AcknowledgeableCommands.Empty();
	}
} // namespace Mqttify
