#pragma once

#include "Mqtt/Interface/IMqttifyCredentialsProvider.h"

class FBasicCredentialsProvider final : public IMqttifyCredentialsProvider
{
private:
	FMqttifyCredentials Credentials;

public:
	FBasicCredentialsProvider(const FString& InUsername, const FString& InPassword)
		: Credentials{InUsername, InPassword}
	{
	}

	FBasicCredentialsProvider(FString&& InUsername, FString&& InPassword)
		: Credentials{MoveTemp(InUsername), MoveTemp(InPassword)}
	{
	}

	virtual ~FBasicCredentialsProvider() override = default;
	virtual FMqttifyCredentials GetCredentials() override
	{
		return Credentials;
	}
};
