#pragma once
#include "CoreMinimal.h"
#include "Misc/DateTime.h"
#include "Misc/Guid.h"
#include "MqttifyTestMessage.generated.h"

USTRUCT()
struct FMqttifyTestMessage
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid MessageId{};
	UPROPERTY()
	FDateTime Time = FDateTime::UtcNow();

	bool operator==(const FMqttifyTestMessage& Other) const
	{
		return MessageId == Other.MessageId;
	}

	bool operator!=(const FMqttifyTestMessage& Other) const
	{
		return !(*this == Other);
	}

	friend uint32 GetTypeHash(const FMqttifyTestMessage& InMessage)
	{
		return GetTypeHash(InMessage.MessageId);
	}
};
