#include "BackSlotRadialRCO.h"

#include "BackSlotRadial.h"
#include "BackSlotRadialModule.h"
#include "FGCharacterPlayer.h"
#include "FGInventoryComponentEquipment.h"
#include "FGInventoryLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Resources/FGItemDescriptor.h"

UBackSlotRadialGameInstanceModule::UBackSlotRadialGameInstanceModule()
{
	// Without this, SML never discovers this class.
	bRootModule = true;
	RemoteCallObjects.Add(UBackSlotRadialRCO::StaticClass());
}

int32 UBackSlotRadialRCO::GetBackIndex(UFGInventoryComponentEquipment* BackSlot)
{
	const int32 active = BackSlot->GetActiveIndex();
	if (BackSlot->IsValidIndex(active))
	{
		return active;
	}
	const int32 empty = BackSlot->FindEmptyIndex();
	return BackSlot->IsValidIndex(empty) ? empty : 0;
}

bool UBackSlotRadialRCO::EquipBackItem(AFGCharacterPlayer* Character, TSubclassOf<UFGItemDescriptor> ItemClass)
{
	if (Character == nullptr || ItemClass == nullptr)
	{
		return false;
	}

	UFGInventoryComponent* inventory = Character->GetInventory();
	UFGInventoryComponentEquipment* backSlot = Character->GetEquipmentSlot(EEquipmentSlot::ES_BACK);
	if (inventory == nullptr || backSlot == nullptr)
	{
		return false;
	}

	const int32 backIndex = GetBackIndex(backSlot);
	if (!backSlot->IsItemAllowed(ItemClass, backIndex))
	{
		UE_LOG(LogBackSlotRadial, Warning, TEXT("%s does not fit the back slot."), *ItemClass->GetName());
		return false;
	}

	for (int32 index = 0; index < inventory->GetSizeLinear(); ++index)
	{
		FInventoryStack stack;
		if (inventory->GetStackFromIndex(index, stack) && stack.HasItems() && stack.Item.GetItemClass() == ItemClass)
		{
			if (backSlot->IsIndexEmpty(backIndex))
			{
				// MoveInventoryItem refuses an empty equipment slot: it asks the slot's size without
				// naming the incoming item, and an empty equipment slot has no size of its own
				// ("Can't find a slot size!"). AddStackToIndex names the item, so add one directly,
				// keeping its state (fuel and so on), then take it out of the inventory.
				const int32 added = backSlot->AddStackToIndex(backIndex, FInventoryStack(stack.Item));
				if (added > 0)
				{
					inventory->RemoveFromIndex(index, added);
					// With no active index the slot holds the item but nothing is worn; make it active,
					// as the game's own handheld radial does, so it is actually equipped.
					if (backSlot->GetActiveIndex() != backIndex)
					{
						backSlot->SetActiveEquipmentIndex(backIndex);
					}
				}
				UE_LOG(LogBackSlotRadial, Log, TEXT("Equipping %s into the empty back slot: %s"), *ItemClass->GetName(),
					added > 0 ? TEXT("done") : TEXT("refused"));
				return added > 0;
			}

			const bool bMoved = UFGInventoryLibrary::MoveInventoryItem(inventory, index, backSlot, backIndex);
			UE_LOG(LogBackSlotRadial, Log, TEXT("Swapping %s into the back slot: %s"), *ItemClass->GetName(),
				bMoved ? TEXT("done") : TEXT("refused"));
			return bMoved;
		}
	}

	UE_LOG(LogBackSlotRadial, Warning, TEXT("%s is no longer in the inventory."), *ItemClass->GetName());
	return false;
}

void UBackSlotRadialRCO::Server_EquipBackItem_Implementation(TSubclassOf<UFGItemDescriptor> ItemClass)
{
	EquipBackItem(GetOwnerPlayerCharacter(), ItemClass);
}

bool UBackSlotRadialRCO::ClearBackSlot(AFGCharacterPlayer* Character)
{
	UFGInventoryComponent* inventory = Character != nullptr ? Character->GetInventory() : nullptr;
	UFGInventoryComponentEquipment* backSlot = Character != nullptr ? Character->GetEquipmentSlot(EEquipmentSlot::ES_BACK) : nullptr;
	if (inventory == nullptr || backSlot == nullptr)
	{
		return false;
	}

	const int32 backIndex = GetBackIndex(backSlot);
	if (!backSlot->IsValidIndex(backIndex) || backSlot->IsIndexEmpty(backIndex))
	{
		return false;
	}

	const int32 emptyIndex = inventory->FindEmptyIndex();
	if (!inventory->IsValidIndex(emptyIndex))
	{
		UE_LOG(LogBackSlotRadial, Warning, TEXT("No empty inventory slot to unequip the back item into."));
		return false;
	}

	// The same move an equip makes, run the other way, so the game unequips it the usual way.
	const bool bMoved = UFGInventoryLibrary::MoveInventoryItem(backSlot, backIndex, inventory, emptyIndex);
	UE_LOG(LogBackSlotRadial, Log, TEXT("Clearing the back slot: %s"), bMoved ? TEXT("done") : TEXT("refused"));
	return bMoved;
}

void UBackSlotRadialRCO::Server_ClearBackSlot_Implementation()
{
	ClearBackSlot(GetOwnerPlayerCharacter());
}

void UBackSlotRadialRCO::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UBackSlotRadialRCO, bDummyReplicated);
}
