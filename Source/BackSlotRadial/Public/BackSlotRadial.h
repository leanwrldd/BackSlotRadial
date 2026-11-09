#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBackSlotRadial, Log, All);

class FBackSlotRadialModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
};
