using UnrealBuildTool;

public class RevoltEditorBridgeEditor : ModuleRules
{
	public RevoltEditorBridgeEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"ApplicationCore",
				"AssetRegistry",
				"AssetTools",
				"BlueprintGraph",
				"LevelEditor",
				"Kismet",
				"KismetCompiler",
				"Networking",
				"Projects",
				"RevoltEditorBridge",
				"Sockets",
				"Json",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"UMG",
				"UnrealEd"
			}
		);
	}
}
