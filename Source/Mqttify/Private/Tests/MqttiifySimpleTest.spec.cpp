#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

BEGIN_DEFINE_SPEC(FMqttiifySimpleTest, "Mqttify.Automation.SimpleSpec", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)
END_DEFINE_SPEC(FMqttiifySimpleTest)

void FMqttiifySimpleTest::Define()
{
	Describe("Simple Arithmetic", [this]()
	{
		It("Should add two numbers correctly", [this]()
		{
			constexpr int32 A = 2;
			constexpr int32 B = 3;
			constexpr int32 Result = A + B;
			TestEqual(TEXT("2 + 3 should equal 5"), Result, 5);
		});

		It("Should subtract two numbers correctly", [this]()
		{
			constexpr int32 A = 10;
			constexpr int32 B = 4;
			constexpr int32 Result = A - B;
			TestEqual(TEXT("10 - 4 should equal 6"), Result, 6);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
