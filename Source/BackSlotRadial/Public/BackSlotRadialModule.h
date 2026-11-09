#pragma once

#include "CoreMinimal.h"
#include "Module/GameInstanceModule.h"
#include "BackSlotRadialModule.generated.h"

/// Registers the remote call object so multiplayer clients can ask the server to swap.
UCLASS()
class BACKSLOTRADIAL_API UBackSlotRadialGameInstanceModule : public UGameInstanceModule
{
	GENERATED_BODY()

public:
	UBackSlotRadialGameInstanceModule();
};
