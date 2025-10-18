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
			Id = 0;
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
 	if (OnDisconnectPromises.Num() == 0)
 	{
 		OnDisconnectPromises.Reserve(kPromiseInitialReserve);
 	}
 	const TSharedPtr<TPromise<TMqttifyResult<void>>> Promise = MakeShared<TPromise<TMqttifyResult<void>>>();
 	OnDisconnectPromises.Add(Promise);
 	LOG_MQTTIFY(VeryVerbose, TEXT("GetDisconnectPromise %d"), OnDisconnectPromises.Num());
 	return Promise;
 }

	TSharedPtr<TPromise<TMqttifyResult<void>>> FMqttifyClientContext::GetConnectPromise()
	{
		FScopeLock Lock(&OnConnectPromisesCriticalSection);
		if (OnConnectPromises.Num() == 0)
		{
			OnConnectPromises.Reserve(kPromiseInitialReserve);
		}
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

		if (InTopic.Contains(TEXT("+")) || InTopic.Contains(TEXT("#")))
		{
			if (WildcardDelegates.Num() == 0)
			{
				WildcardDelegates.Reserve(kWildcardDelegatesInitialReserve);
			}
			WildcardDelegates.Emplace(FMqttifyTopicFilter{InTopic}, Delegate);
		}
		else
		{
			ExactDelegates.Add(InTopic, Delegate);
		}
		LOG_MQTTIFY(VeryVerbose, TEXT("GetMessageDelegate %s %d"), *InTopic, OnMessageDelegates.Num());
		return Delegate;
	}

	void FMqttifyClientContext::ClearMessageDelegates(const TSharedPtr<TArray<FMqttifyUnsubscribeResult>>& InUnsubscribeResults)
	{
		FScopeLock Lock(&OnMessageDelegatesCriticalSection);

		LOG_MQTTIFY(VeryVerbose, TEXT("ClearMessageDelegates %d"), OnMessageDelegates.Num());
		for (FMqttifyUnsubscribeResult& Result : *InUnsubscribeResults)
		{
			const FString Key = Result.GetFilter().GetFilter();
			if (const TSharedRef<FOnMessage>* Delegate = OnMessageDelegates.Find(Key))
			{
				(*Delegate)->Clear();
				OnMessageDelegates.Remove(Key);
			}
			ExactDelegates.Remove(Key);
			WildcardDelegates.RemoveAll([&](const TPair<FMqttifyTopicFilter, TSharedRef<FOnMessage>>& P) {
				return P.Key.GetFilter() == Key;
			});
		}
	}

	void FMqttifyClientContext::CompleteDisconnect()
	{
		TWeakPtr<FMqttifyClientContext> ThisWeakPtr = AsWeak();
		AbandonCommands();
		DispatchWithThreadHandling(
			[ThisWeakPtr] {
				if (const TSharedPtr<FMqttifyClientContext> ThisSharedPtr = ThisWeakPtr.Pin())
				{
					TArray<TSharedPtr<TPromise<TMqttifyResult<void>>>> Snapshot;
					{
						FScopeLock Lock(&ThisSharedPtr->OnDisconnectPromisesCriticalSection);
						Snapshot = ThisSharedPtr->OnDisconnectPromises;
						ThisSharedPtr->OnDisconnectPromises.Empty();
						ThisSharedPtr->OnDisconnectPromises.Reserve(kPromiseInitialReserve);
					}

					ThisSharedPtr->OnDisconnect().Broadcast(true);
					for (const auto& Promise : Snapshot)
					{
						Promise->SetValue(TMqttifyResult<void>{true});
					}
				}
			});
	}

	void FMqttifyClientContext::CompleteConnect()
	{
		TWeakPtr<FMqttifyClientContext> ThisWeakPtr = AsWeak();
		DispatchWithThreadHandling(
			[ThisWeakPtr] {
				if (const TSharedPtr<FMqttifyClientContext> ThisSharedPtr = ThisWeakPtr.Pin())
				{
					TArray<TSharedPtr<TPromise<TMqttifyResult<void>>>> Snapshot;
					{
						FScopeLock Lock(&ThisSharedPtr->OnConnectPromisesCriticalSection);
						Snapshot = ThisSharedPtr->OnConnectPromises;
						ThisSharedPtr->OnConnectPromises.Empty();
						ThisSharedPtr->OnConnectPromises.Reserve(kPromiseInitialReserve);
					}

					ThisSharedPtr->OnConnect().Broadcast(true);
					for (const auto& Promise : Snapshot)
					{
						Promise->SetValue(TMqttifyResult<void>{true});
					}
				}
			});
	}

	void FMqttifyClientContext::CompleteMessage(FMqttifyMessage&& InMessage)
	{
		TWeakPtr<FMqttifyClientContext> ThisWeakPtr = AsWeak();
		DispatchWithThreadHandling(
			[ThisWeakPtr, Message = MoveTemp(InMessage)]() mutable {
				if (const TSharedPtr<FMqttifyClientContext> ThisSharedPtr = ThisWeakPtr.Pin())
				{
					const FString Topic = Message.GetTopic();
					TSharedPtr<FOnMessage> ExactToCall;
					TArray<TPair<FMqttifyTopicFilter, TSharedRef<FOnMessage>>> WildcardSnapshot;
					{
						FScopeLock Lock(&ThisSharedPtr->OnMessageDelegatesCriticalSection);
						if (const TSharedRef<FOnMessage>* Exact = ThisSharedPtr->ExactDelegates.Find(Topic))
						{
							ExactToCall = *Exact;
						}
						WildcardSnapshot = ThisSharedPtr->WildcardDelegates;
					}

					ThisSharedPtr->OnMessage().Broadcast(Message);
					LOG_MQTTIFY(VeryVerbose, TEXT("OnMessage %s"), *Topic);

					if (ExactToCall.IsValid())
					{
						ExactToCall->Broadcast(Message);
					}

					for (const auto& Entry : WildcardSnapshot)
					{
						const FMqttifyTopicFilter& Filter = Entry.Key;
						if (Filter.MatchesWildcard(Topic))
						{
							Entry.Value->Broadcast(Message);
						}
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
		ExactDelegates.Empty();
		WildcardDelegates.Empty();
	}

	void FMqttifyClientContext::ClearDisconnectPromises()
	{
		TArray<TSharedPtr<TPromise<TMqttifyResult<void>>>> Snapshot;
		{
			FScopeLock Lock(&OnDisconnectPromisesCriticalSection);
			Snapshot = OnDisconnectPromises;
			OnDisconnectPromises.Empty();
			OnDisconnectPromises.Reserve(kPromiseInitialReserve);
		}
		for (const auto& Promise : Snapshot)
		{
			DispatchWithThreadHandling(
				[Promise] {
					Promise->SetValue(TMqttifyResult<void>{false});
				});
		}
	}

	void FMqttifyClientContext::ClearConnectPromises()
	{
		TArray<TSharedPtr<TPromise<TMqttifyResult<void>>>> Snapshot;
		{
			FScopeLock Lock(&OnConnectPromisesCriticalSection);
			Snapshot = OnConnectPromises;
			OnConnectPromises.Empty();
			OnConnectPromises.Reserve(kPromiseInitialReserve);
		}
		for (const auto& Promise : Snapshot)
		{
			DispatchWithThreadHandling(
				[Promise] {
					Promise->SetValue(TMqttifyResult<void>{false});
				});
		}
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