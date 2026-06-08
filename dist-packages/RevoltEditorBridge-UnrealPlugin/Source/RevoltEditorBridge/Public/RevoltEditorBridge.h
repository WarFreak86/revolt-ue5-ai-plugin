#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

REVOLTEDITORBRIDGE_API DECLARE_LOG_CATEGORY_EXTERN(LogRevoltEditorBridge, Log, All);

class FRevoltEditorBridgeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
