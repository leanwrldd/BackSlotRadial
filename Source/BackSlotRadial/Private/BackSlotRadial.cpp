#include "BackSlotRadial.h"

#include "Engine/AssetManager.h"
#include "Input/FGChildInputMappingContext.h"

DEFINE_LOG_CATEGORY(LogBackSlotRadial)

void FBackSlotRadialModule::StartupModule()
{
	// Options -> Controls only lists child contexts the Asset Manager knows about.
	UAssetManager::CallOrRegister_OnAssetManagerCreated(FSimpleMulticastDelegate::FDelegate::CreateLambda([]
	{
		const int32 found = UAssetManager::Get().ScanPathForPrimaryAssets(
			FPrimaryAssetType(UFGChildInputMappingContext::StaticClass()->GetFName()),
			TEXT("/BackSlotRadial/Input"), UFGChildInputMappingContext::StaticClass(), false);
		UE_LOG(LogBackSlotRadial, Log, TEXT("Registered %d keybind context(s)."), found);
	}));
}

IMPLEMENT_MODULE(FBackSlotRadialModule, BackSlotRadial)
