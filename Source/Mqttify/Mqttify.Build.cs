// Copyright 31st Union. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class Mqttify : ModuleRules
{
	public Mqttify(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"Engine",
				"Json",
				"JsonUtilities",
				"Networking",
				"Projects",
				"SSL",
				"Sockets",
				"WebSockets"
			}
		);

		if (Target.WithAutomationTests)
		{
			AddEngineThirdPartyPrivateStaticDependencies(Target, "libWebSockets");
		}

		AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenSSL");

		// Build
		CppStandard = CppStandardVersion.Cpp20;
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));

		// Valid options are:
		// 3 = MQTT 3.1.1
		// 5 = MQTT 5.0
		PrivateDefinitions.Add("MQTTIFY_PROTOCOL_VERSION=5");

		// 0 = game thread,
		// 1 = worker thread with marshalling of callbacks onto main thread
		// 2 = worker thread without marshalling of callbacks
		PrivateDefinitions.Add("MQTTIFY_THREAD=1");
	}
}