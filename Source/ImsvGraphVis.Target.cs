// Copyright 2017 Oh-Hyun Kwon. All Rights Reserved.
// Copyright 2018 David Kuhta. All Rights Reserved for additions.

using UnrealBuildTool;
using System.Collections.Generic;

public class ImsvGraphVisTarget : TargetRules
{
	public ImsvGraphVisTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

        //PublicDependencyModuleNames.AddRange(new string[] { "HeadMountedDisplay" });

        ExtraModuleNames.AddRange( new string[] { "ImsvGraphVis", "Boost" /*, "HeadMountedDisplay"*/ } );
	}
}
