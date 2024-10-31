#pragma once

#include "Mqtt/Interface/IMqttifyCredentialsProvider.h"

class FMqttifyBasicCredentialsProvider final : public IMqttifyCredentialsProvider
{
private:
	FMqttifyCredentials Credentials;

public:
	FMqttifyBasicCredentialsProvider(const FString& InUsername, const FString& InPassword)
		: Credentials{InUsername, InPassword}
	{
	}

	FMqttifyBasicCredentialsProvider(FString&& InUsername, FString&& InPassword)
		: Credentials{MoveTemp(InUsername), MoveTemp(InPassword)}
	{
	}

	virtual ~FMqttifyBasicCredentialsProvider() override = default;
	virtual FMqttifyCredentials GetCredentials() override
	{
		return Credentials;
	}
};
