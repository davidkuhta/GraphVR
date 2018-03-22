// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class Boost : ModuleRules
{
    private string ThirdPartyPath
    {
        get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "../ThirdParty/")); }
    }
	public Boost(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "BoostLibrary"));

        PublicSystemIncludePaths.Add(Path.Combine(ThirdPartyPath, "BoostLibrary"));

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
		
		//PublicIncludePaths.AddRange(
		//	new string[] {
		//		"Boost/Public"
		//		// ... add public include paths required here ...
		//	}
		//	);
				
		
		//PrivateIncludePaths.AddRange(
		//	new string[] {
		//		"Boost/Private",
		//		 ... add other private include paths required here ...
		//	}
		//	);
			
		
		//PublicDependencyModuleNames.AddRange(
		//	new string[]
		//	{
		//		"Core",
		//		"BoostLibrary",
		//		"Projects"
		//		// ... add other public dependencies that you statically link with here ...
		//	}
		//	);
			
		
		//PrivateDependencyModuleNames.AddRange(
		//	new string[]
		//	{
		//		// ... add private dependencies that you statically link with here ...	
		//	}
		//	);
		
		
		//DynamicallyLoadedModuleNames.AddRange(
		//	new string[]
		//	{
		//		// ... add any modules that your module loads dynamically here ...
		//	}
		//	);
	}
}
