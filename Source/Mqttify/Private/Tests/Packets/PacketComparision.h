#pragma once
#include "Misc/Base64.h"
#if WITH_DEV_AUTOMATION_TESTS

#include "Packets/Interface/IMqttifyControlPacket.h"

namespace Mqttify
{
	FORCEINLINE bool TestPacketsEqual(const TCHAR* What,
									IMqttifyControlPacket& Actual,
									const TArray<uint8>& Expected,
									FAutomationSpecBase* const AutomationSpec)
	{
		// Serialize the actual packet to a byte array
		TArray<uint8> ActualBytes;
		FMemoryWriter Writer(ActualBytes);
		Actual.Encode(Writer);

		bool bAreEqual = true;

		// Check lengths
		if (ActualBytes.Num() != Expected.Num())
		{
			bAreEqual = false;
			AutomationSpec->AddError(FString::Printf(
				TEXT("%s: Length mismatch. Actual: %d, Expected: %d."),
				What,
				ActualBytes.Num(),
				Expected.Num()));
		}

		// Generate hex strings and check each byte

		FString ActualHex, ExpectedHex;
		FBase64::Encode(ActualBytes);
		for (int32 i = 0; i < FMath::Max(ActualBytes.Num(), Expected.Num()); ++i)
		{
			if (i < ActualBytes.Num())
			{
				ActualHex += FString::Printf(TEXT("%02x "), ActualBytes[i]);
			}

			if (i < Expected.Num())
			{
				ExpectedHex += FString::Printf(TEXT("%02x "), Expected[i]);
			}

			if (i < ActualBytes.Num() && i < Expected.Num() && ActualBytes[i] != Expected[i])
			{
				bAreEqual = false;
				AutomationSpec->AddError(FString::Printf(
					TEXT("%s: Byte mismatch at index %d. Actual: %02x, Expected: %02x."),
					What,
					i,
					ActualBytes[i],
					Expected[i]));
			}
		}

		if (!bAreEqual)
		{
			AutomationSpec->AddError(
				FString::Printf(TEXT("%s: Actual: %s, Expected: %s."), What, *ActualHex, *ExpectedHex));
		}
		return bAreEqual;
	}
} // namespace Mqttify
#endif // WITH_DEV_AUTOMATION_TESTS
