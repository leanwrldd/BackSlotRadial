#include "BackSlotRadialSubsystem.h"

#include "BackSlotRadial.h"
#include "BackSlotRadialRCO.h"
#include "BackSlotRadialWidget.h"
#include "Blueprint/UserWidget.h"
#include "AkAudioEvent.h"
#include "AkGameplayStatics.h"
#include "Engine/World.h"
#include "EnhancedPlayerInput.h"
#include "Equipment/FGWeapon.h"
#include "FGCharacterPlayer.h"
#include "FGInventoryComponentEquipment.h"
#include "FGPlayerController.h"
#include "InputAction.h"
#include "Resources/FGItemDescriptor.h"

namespace
{
const TCHAR* const OpenActionPath = TEXT("/BackSlotRadial/Input/IA_BSR_OpenMenu.IA_BSR_OpenMenu");

// The build-mode radial's own sounds, so the menus sound alike.
const TCHAR* const HoverSoundPath = TEXT("/Game/WwiseAudio/Events/Interface/BuildMenu/Play_UI_BuildMenu_Hover_Radial.Play_UI_BuildMenu_Hover_Radial");
const TCHAR* const SelectSoundPath = TEXT("/Game/WwiseAudio/Events/Interface/BuildMenu/Play_UI_BuildMenu_Hover_Select.Play_UI_BuildMenu_Hover_Select");

// Long enough that a tap on the key stays a tap.
constexpr float HoldSeconds = 0.25f;
constexpr int32 MenuZOrder = 100;

// In mouse units: the pointer stops here, so reversing direction takes effect at once.
constexpr double PointerMaxLength = 120.0;

int32 WedgeAt(double Degrees, int32 Count)
{
	return Count > 0 ? FMath::RoundToInt(Degrees / (360.0 / Count)) % Count : INDEX_NONE;
}

TArray<TSubclassOf<UFGItemDescriptor>> FindBackItems(UFGInventoryComponent* Inventory, UFGInventoryComponentEquipment* BackSlot)
{
	TArray<TSubclassOf<UFGItemDescriptor>> items;
	TArray<FInventoryStack> stacks;
	Inventory->GetInventoryStacks(stacks);
	for (const FInventoryStack& stack : stacks)
	{
		TSubclassOf<UFGItemDescriptor> item = stack.Item.GetItemClass();
		// The slot's own filter, so modded equipment needs no special handling.
		if (item != nullptr && BackSlot->IsItemAllowed(item, UBackSlotRadialRCO::GetBackIndex(BackSlot)))
		{
			items.AddUnique(item);
		}
	}
	return items;
}
}

bool UBackSlotRadialSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return !IsRunningDedicatedServer() && Super::ShouldCreateSubsystem(Outer);
}

void UBackSlotRadialSubsystem::Deinitialize()
{
	CloseMenu(false);
	Super::Deinitialize();
}

TStatId UBackSlotRadialSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBackSlotRadialSubsystem, STATGROUP_Tickables);
}

bool UBackSlotRadialSubsystem::CanUseMenu(AFGPlayerController* Controller, AFGCharacterPlayer* Character)
{
	// Photo mode possesses a camera pawn, so Character is null there, as it is in vehicles.
	return Character != nullptr
		&& Character->IsAliveAndWell()
		&& !Character->IsBuildGunEquipped()
		&& !Character->IsDrivingVehicle()
		&& !Controller->IsPauseMenuOpen()
		&& !Controller->ShouldShowMouseCursor()
		// R reloads.
		&& Cast<AFGWeapon>(Character->GetEquipmentInSlot(EEquipmentSlot::ES_ARMS)) == nullptr;
}

void UBackSlotRadialSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AFGPlayerController* controller = Cast<AFGPlayerController>(GetWorld()->GetFirstPlayerController());
	UEnhancedPlayerInput* input = controller != nullptr ? Cast<UEnhancedPlayerInput>(controller->PlayerInput) : nullptr;
	if (input == nullptr)
	{
		CloseMenu(false);
		return;
	}

	if (OpenAction == nullptr)
	{
		if (bActionMissing)
		{
			return;
		}
		OpenAction = LoadObject<UInputAction>(nullptr, OpenActionPath);
		if (OpenAction == nullptr)
		{
			bActionMissing = true;
			UE_LOG(LogBackSlotRadial, Error, TEXT("Could not load %s."), OpenActionPath);
			return;
		}
	}

	AFGCharacterPlayer* character = Cast<AFGCharacterPlayer>(controller->GetPawn());
	const bool bHeld = input->GetActionValue(OpenAction).Get<bool>();
	const bool bUsable = CanUseMenu(controller, character);

	if (Menu != nullptr)
	{
		if (!bUsable || character != MenuCharacter.Get())
		{
			CloseMenu(false);
			bWaitingForRelease = bHeld;
		}
		else if (bHeld)
		{
			UpdateSelection(controller);
		}
		else
		{
			CloseMenu(true);
		}
		return;
	}

	if (!bHeld)
	{
		HeldTime = 0.0f;
		bWaitingForRelease = false;
	}
	else if (!bWaitingForRelease)
	{
		if (!bUsable)
		{
			bWaitingForRelease = true;
		}
		else
		{
			HeldTime += DeltaTime;
			if (HeldTime >= HoldSeconds)
			{
				OpenMenu(controller, character);
			}
		}
	}
}

void UBackSlotRadialSubsystem::OpenMenu(AFGPlayerController* Controller, AFGCharacterPlayer* Character)
{
	UFGInventoryComponent* inventory = Character->GetInventory();
	UFGInventoryComponentEquipment* backSlot = Character->GetEquipmentSlot(EEquipmentSlot::ES_BACK);
	Menu = inventory != nullptr && backSlot != nullptr
		? CreateWidget<UBackSlotRadialWidget>(Controller, UBackSlotRadialWidget::StaticClass())
		: nullptr;
	if (Menu == nullptr)
	{
		bWaitingForRelease = true;
		return;
	}

	FInventoryStack equipped;
	backSlot->GetStackFromIndex(UBackSlotRadialRCO::GetBackIndex(backSlot), equipped);
	Menu->SetContents(FindBackItems(inventory, backSlot), equipped.HasItems() ? equipped.Item.GetItemClass() : nullptr);
	Menu->AddToViewport(MenuZOrder);
	LoadSounds();

	// The mouse picks a wedge now, so it mustn't turn the camera as well.
	Controller->SetIgnoreLookInput(true);
	MenuController = Controller;
	MenuCharacter = Character;
	Pointer = FVector2D::ZeroVector;
	AimDegrees = 0.0;
	// Selected from the first frame, so a release without touching the mouse picks the top wedge.
	SelectedIndex = WedgeAt(AimDegrees, Menu->GetNumEntries());
	Menu->SetSelected(SelectedIndex);
}

void UBackSlotRadialSubsystem::UpdateSelection(AFGPlayerController* Controller)
{
	float deltaX = 0.0f;
	float deltaY = 0.0f;
	Controller->GetInputMouseDelta(deltaX, deltaY);
	Pointer = (Pointer + FVector2D(deltaX, deltaY)).GetClampedToMaxSize(PointerMaxLength);

	const int32 previousIndex = SelectedIndex;
	// Clockwise from straight up, matching how the widget lays out its wedges. Until the mouse
	// moves the aim stays at its last value, which starts straight up, so something is always
	// selected: the wedge under the arrow, as in the build-mode radial.
	if (!Pointer.IsNearlyZero(1.0))
	{
		AimDegrees = FMath::Fmod(FMath::RadiansToDegrees(FMath::Atan2(Pointer.X, Pointer.Y)) + 360.0, 360.0);
		Menu->SetPointer(static_cast<float>(AimDegrees));
	}
	SelectedIndex = WedgeAt(AimDegrees, Menu->GetNumEntries());
	Menu->SetSelected(SelectedIndex);

	// Tick once per wedge entered, not every frame.
	if (SelectedIndex != INDEX_NONE && SelectedIndex != previousIndex)
	{
		PlayUISound(HoverSound, MenuCharacter.Get());
	}
}

void UBackSlotRadialSubsystem::LoadSounds()
{
	if (bSoundsLoaded)
	{
		return;
	}
	bSoundsLoaded = true;
	HoverSound = LoadObject<UAkAudioEvent>(nullptr, HoverSoundPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	SelectSound = LoadObject<UAkAudioEvent>(nullptr, SelectSoundPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (HoverSound == nullptr || SelectSound == nullptr)
	{
		UE_LOG(LogBackSlotRadial, Log, TEXT("Menu sounds not found (expected outside the shipped game); the menu stays silent."));
	}
}

void UBackSlotRadialSubsystem::PlayUISound(UAkAudioEvent* Sound, AActor* Listener)
{
	if (Sound != nullptr && Listener != nullptr)
	{
		// The game's Wwise build compiles out UAkGameplayStatics::PostEvent, so post at the player's
		// position instead. UI events are 2D in Wwise, so where they play makes no difference.
		UAkGameplayStatics::PostAkEventAtLocation(Listener, Sound, Listener->GetActorLocation(), FRotator::ZeroRotator);
	}
}

void UBackSlotRadialSubsystem::CloseMenu(bool bConfirm)
{
	if (Menu == nullptr)
	{
		return;
	}

	const TSubclassOf<UFGItemDescriptor> item = bConfirm ? Menu->GetEntryItem(SelectedIndex) : nullptr;
	const bool bClear = bConfirm && Menu->IsClearEntry(SelectedIndex);
	Menu->RemoveFromParent();
	Menu = nullptr;
	HeldTime = 0.0f;

	AFGPlayerController* controller = MenuController.Get();
	AFGCharacterPlayer* character = MenuCharacter.Get();
	if (controller == nullptr)
	{
		return;
	}
	controller->SetIgnoreLookInput(false);

	if (character == nullptr)
	{
		return;
	}

	if (bClear || item != nullptr)
	{
		PlayUISound(SelectSound, character);
	}

	if (bClear)
	{
		if (controller->HasAuthority())
		{
			UBackSlotRadialRCO::ClearBackSlot(character);
		}
		else if (UBackSlotRadialRCO* rco = controller->GetRemoteCallObjectOfClass<UBackSlotRadialRCO>())
		{
			rco->Server_ClearBackSlot();
		}
		return;
	}

	if (item == nullptr)
	{
		return;
	}

	if (controller->HasAuthority())
	{
		UBackSlotRadialRCO::EquipBackItem(character, item);
	}
	else if (UBackSlotRadialRCO* rco = controller->GetRemoteCallObjectOfClass<UBackSlotRadialRCO>())
	{
		rco->Server_EquipBackItem(item);
	}
}
