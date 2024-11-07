#include "MqttifyClientContext.h"

#include "LogMqttify.h"
#include "MqttifyAsync.h"
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
		AbandonCommands();
	}

	FMqttifyClientContext::FMqttifyClientContext(const FMqttifyConnectionSettingsRef& InConnectionSettings)
		: ConnectionSettings{InConnectionSettings}
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

	void FMqttifyClientContext::AddAcknowledgeableCommand(const TSharedRef<FMqttifyQueueable>& InCommand)
	{
		FScopeLock Lock(&AcknowledgeableCommandsCriticalSection);
		if (const IMqttifyAcknowledgeable* Acknowledgeable = InCommand->AsAcknowledgeable())
		{
			AcknowledgeableCommands.Add(Acknowledgeable->GetId(), InCommand);
		}
	}

	bool FMqttifyClientContext::HasAcknowledgeableCommand(const uint16 InPacketIdentifier) const
	{
		FScopeLock Lock(&AcknowledgeableCommandsCriticalSection);
		return AcknowledgeableCommands.Contains(InPacketIdentifier);
	}

	void FMqttifyClientContext::AddOneShotCommand(const TSharedRef<FMqttifyQueueable>& InCommand)
	{
		OneShotCommands.Enqueue(InCommand);
	}

	void FMqttifyClientContext::ProcessCommands()
	{
		// TODO SB - Come back to this, since we have the option of running on the game thread
		// and the loop may block while writing to the socket (which is not ideal). If there
		// is a large number of commands to process, we may want to process them in batches
		// and set a limit to the number of commands we have pending

		/// Process OneShotCommands
		TSharedPtr<FMqttifyQueueable> Command = nullptr;
		while (OneShotCommands.Dequeue(Command))
		{
			if (Command.IsValid())
			{
				Command->Next();
			}
		}

		{
			FScopeLock Lock(&AcknowledgeableCommandsCriticalSection);
			for (auto It = AcknowledgeableCommands.CreateIterator(); It; ++It)
			{
				if (It.Value()->Next())
				{
					It.RemoveCurrent();
				}
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
		LOG_MQTTIFY(VeryVerbose, TEXT("GetDisconnectPromise %d"), OnDisconnectPromises.Num());
		return Promise;
	}

	TSharedPtr<TPromise<TMqttifyResult<void>>> FMqttifyClientContext::GetConnectPromise()
	{
		FScopeLock Lock(&OnConnectPromisesCriticalSection);
		const TSharedPtr<TPromise<TMqttifyResult<void>>> Promise = MakeShared<TPromise<TMqttifyResult<void>>>();
		OnConnectPromises.Add(Promise);
		LOG_MQTTIFY(VeryVerbose, TEXT("GetConnectPromise %d"), OnConnectPromises.Num());
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
		LOG_MQTTIFY(VeryVerbose, TEXT("GetMessageDelegate %s %d"), *InTopic, OnMessageDelegates.Num());
		return Delegate;
	}

	void FMqttifyClientContext::ClearMessageDelegates(
		const TSharedPtr<TArray<FMqttifyUnsubscribeResult>>& InUnsubscribeResults
		)
	{
		FScopeLock Lock(&OnMessageDelegatesCriticalSection);

		LOG_MQTTIFY(VeryVerbose, TEXT("ClearMessageDelegates %d"), OnMessageDelegates.Num());
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
		TWeakPtr<FMqttifyClientContext> ThisWeakPtr = AsWeak();
		AbandonCommands();
		DispatchWithThreadHandling(
			[ThisWeakPtr]
			{
				if (const TSharedPtr<FMqttifyClientContext> ThisSharedPtr = ThisWeakPtr.Pin())
				{
					FScopeLock Lock(&ThisSharedPtr->OnDisconnectPromisesCriticalSection);
					ThisSharedPtr->OnDisconnect().Broadcast(true);
					for (const auto& Promise : ThisSharedPtr->OnDisconnectPromises)
					{
						Promise->SetValue(TMqttifyResult<void>{true});
					}
					ThisSharedPtr->OnDisconnectPromises.Empty();
				}
			});
	}

	void FMqttifyClientContext::CompleteConnect()
	{
		TWeakPtr<FMqttifyClientContext> ThisWeakPtr = AsWeak();
		DispatchWithThreadHandling(
			[ThisWeakPtr]
			{
				if (const TSharedPtr<FMqttifyClientContext> ThisSharedPtr = ThisWeakPtr.Pin())
				{
					FScopeLock Lock(&ThisSharedPtr->OnConnectPromisesCriticalSection);
					ThisSharedPtr->OnConnect().Broadcast(true);
					for (const auto& Promise : ThisSharedPtr->OnConnectPromises)
					{
						Promise->SetValue(TMqttifyResult<void>{true});
					}
					ThisSharedPtr->OnConnectPromises.Empty();
				}
			});
	}

	void FMqttifyClientContext::CompleteMessage(FMqttifyMessage&& InMessage)
	{
		TWeakPtr<FMqttifyClientContext> ThisWeakPtr = AsWeak();
		DispatchWithThreadHandling(
			[ThisWeakPtr, Message = MoveTemp(InMessage)]() mutable
			{
				if (const TSharedPtr<FMqttifyClientContext> ThisSharedPtr = ThisWeakPtr.Pin())
				{
					FScopeLock Lock(&ThisSharedPtr->OnMessageDelegatesCriticalSection);
					ThisSharedPtr->OnMessage().Broadcast(Message);
					if (const TSharedRef<FOnMessage>* Delegate = ThisSharedPtr->OnMessageDelegates.Find(
						Message.GetTopic()))
					{
						(*Delegate)->Broadcast(Message);
					}
				}
			});
	}

	void FMqttifyClientContext::Acknowledge(const FMqttifyPacketPtr& InPacket)
	{
		FScopeLock Lock(&AcknowledgeableCommandsCriticalSection);

		if (!InPacket.IsValid())
		{
			LOG_MQTTIFY(Error, TEXT("Invalid packet pointer"));
			return;
		}

		LOG_MQTTIFY(VeryVerbose, TEXT("Acknowledging %s"), EnumToTCharString(InPacket->GetPacketType()));

		// The reason for the bit shift is we are using the first 16 bits for our packet identifier
		// and the last 16 bits for the identifiers generated by the broker.
		const uint32 InPacketIdentifier = InPacket->GetPacketType() != EMqttifyPacketType::PubRel
			                                  ? InPacket->GetPacketId()
			                                  : static_cast<uint32>(InPacket->GetPacketId()) << 16;;

		if (!InPacketIdentifier)
		{
			return;
		}

		if (const TSharedRef<FMqttifyQueueable>* Command = AcknowledgeableCommands.Find(InPacketIdentifier))
		{
			if (IMqttifyAcknowledgeable* Acknowledgeable = (*Command)->AsAcknowledgeable())
			{
				if (Acknowledgeable->Acknowledge(InPacket))
				{
					LOG_MQTTIFY(
						VeryVerbose,
						TEXT("Removing packet identifier %d, Type %s"),
						InPacketIdentifier,
						EnumToTCharString(InPacket->GetPacketType()));
					AcknowledgeableCommands.Remove(InPacketIdentifier);
					if (InPacketIdentifier <= kMaxCount)
					{
						ReleaseId(InPacket->GetPacketId());
					}
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
			DispatchWithThreadHandling(
				[Promise]
				{
					Promise->SetValue(TMqttifyResult<void>{false});
				});
		}
		OnDisconnectPromises.Empty();
	}

	void FMqttifyClientContext::ClearConnectPromises()
	{
		FScopeLock Lock(&OnConnectPromisesCriticalSection);
		for (const auto& Promise : OnConnectPromises)
		{
			DispatchWithThreadHandling(
				[Promise]
				{
					Promise->SetValue(TMqttifyResult<void>{false});
				});
		}
		OnConnectPromises.Empty();
	}

	void FMqttifyClientContext::AbandonCommands()
	{
		OneShotCommands.Empty();
		{
			FScopeLock Lock(&AcknowledgeableCommandsCriticalSection);
			AcknowledgeableCommands.Empty();
		}
	}
} // namespace Mqttify
