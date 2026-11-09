using UnrealBuildTool;

public class BackSlotRadial : ModuleRules
{
	public BackSlotRadial(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"NetCore",
			"SlateCore", "Slate", "UMG",
			"GameplayTags",
			"AkAudio",
		});

		// Header stubs
		PublicDependencyModuleNames.AddRange(new string[] {
			"DummyHeaders",
		});

		PublicDependencyModuleNames.AddRange(new string[] {"FactoryGame", "SML"});
	}
}
