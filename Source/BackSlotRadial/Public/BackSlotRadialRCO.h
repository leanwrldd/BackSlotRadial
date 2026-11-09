#pragma once

#include "CoreMinimal.h"
#include "FGRemoteCallObject.h"
#include "BackSlotRadialRCO.generated.h"

class AFGCharacterPlayer;
class UFGItemDescriptor;

/// Inventory moves only run with authority, so a client's choice goes through here.
UCLASS(Within = FGPlayerController)
class BACKSLOTRADIAL_API UBackSlotRadialRCO : public UFGRemoteCallObject
{
	GENERATED_BODY()

public:
	/// The back slot's working index. The active index goes INDEX_NONE once the slot is emptied,
	/// which no inventory call accepts, so fall back to an empty index, then 0.
	static int32 GetBackIndex(class UFGInventoryComponentEquipment* BackSlot);

	/// Swaps the first stack of ItemClass in the character's inventory into their back slot.
	/// Authority only. The back slot's own filter decides whether the item fits.
	static bool EquipBackItem(AFGCharacterPlayer* Character, TSubclassOf<UFGItemDescriptor> ItemClass);

	UFUNCTION(Server, Reliable)
	void Server_EquipBackItem(TSubclassOf<UFGItemDescriptor> ItemClass);

	/// Moves whatever is in the back slot into the first empty inventory slot. Authority only.
	/// Refuses, leaving the item equipped, when the inventory has no empty slot.
	static bool ClearBackSlot(AFGCharacterPlayer* Character);

	UFUNCTION(Server, Reliable)
	void Server_ClearBackSlot();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/// An RCO needs at least one replicated property, or its RPCs are dropped.
	UPROPERTY(Replicated)
	bool bDummyReplicated = true;
};
