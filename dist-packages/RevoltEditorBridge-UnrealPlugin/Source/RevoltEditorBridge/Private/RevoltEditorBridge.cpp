#include "RevoltEditorBridge.h"

DEFINE_LOG_CATEGORY(LogRevoltEditorBridge);

void FRevoltEditorBridgeModule::StartupModule()
{
	UE_LOG(LogRevoltEditorBridge, Log, TEXT("RevoltEditorBridge runtime module started."));
}

void FRevoltEditorBridgeModule::ShutdownModule()
{
	UE_LOG(LogRevoltEditorBridge, Log, TEXT("RevoltEditorBridge runtime module shut down."));
}

IMPLEMENT_MODULE(FRevoltEditorBridgeModule, RevoltEditorBridge)
