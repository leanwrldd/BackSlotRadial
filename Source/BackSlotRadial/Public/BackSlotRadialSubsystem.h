#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BackSlotRadialSubsystem.generated.h"

class AFGCharacterPlayer;
class AFGPlayerController;
class UBackSlotRadialWidget;
class UFGItemDescriptor;
class UInputAction;

/// Watches the hold key for the local player, and runs the menu while it is open.
UCLASS()
class BACKSLOTRADIAL_API UBackSlotRadialSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	// So the menu still closes when something pauses the game while it's open.
	virtual bool IsTickableWhenPaused() const override { return true; }

private:
	/// False wherever the key belongs to something else: building, weapons, menus, vehicles.
	static bool CanUseMenu(AFGPlayerController* Controller, AFGCharacterPlayer* Character);

	void OpenMenu(AFGPlayerController* Controller, AFGCharacterPlayer* Character);
	void UpdateSelection(AFGPlayerController* Controller);
	void CloseMenu(bool bConfirm);

	/// Loads the vanilla build-menu sounds on first use. They only exist in the shipped game, so in
	/// the editor they stay null and the menu is silent.
	void LoadSounds();
	static void PlayUISound(class UAkAudioEvent* Sound, AActor* Listener);

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> OpenAction;

	UPROPERTY(Transient)
	TObjectPtr<class UAkAudioEvent> HoverSound;

	UPROPERTY(Transient)
	TObjectPtr<class UAkAudioEvent> SelectSound;

	bool bSoundsLoaded = false;

	UPROPERTY(Transient)
	TObjectPtr<UBackSlotRadialWidget> Menu;

	TWeakObjectPtr<AFGPlayerController> MenuController;
	TWeakObjectPtr<AFGCharacterPlayer> MenuCharacter;

	float HeldTime = 0.0f;
	/// Set once a press is ruled out, so the key must be released before the next try. Stops the
	/// menu popping up when the build gun is put away with the key still down.
	bool bWaitingForRelease = false;
	/// The input asset failed to load; the error is logged once.
	bool bActionMissing = false;
	/// Mouse movement since the menu opened, in mouse units with +Y up.
	FVector2D Pointer = FVector2D::ZeroVector;
	int32 SelectedIndex = INDEX_NONE;
	/// Where the arrow points, clockwise from straight up. Kept while the mouse is still.
	double AimDegrees = 0.0;
};
