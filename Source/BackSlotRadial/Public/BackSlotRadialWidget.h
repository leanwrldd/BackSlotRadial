#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "BackSlotRadialWidget.generated.h"

class UFGItemDescriptor;
class UTextBlock;

USTRUCT()
struct FBackSlotRadialEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<UFGItemDescriptor> ItemClass;

	UPROPERTY()
	FText Name;

	/// The "Unequip" wedge: no item, drawn as a big X, empties the back slot.
	UPROPERTY()
	bool bClear = false;

	/// A UPROPERTY so GC sees the icon it holds.
	UPROPERTY()
	FSlateBrush Icon;
};

/// The radial menu, built in C++ so no widget Blueprint is needed. It never takes input: the
/// subsystem polls the mouse and tells it which wedge is selected, so the game keeps every key.
UCLASS()
class BACKSLOTRADIAL_API UBackSlotRadialWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetContents(const TArray<TSubclassOf<UFGItemDescriptor>>& Items, TSubclassOf<UFGItemDescriptor> Equipped);

	/// INDEX_NONE selects nothing.
	void SetSelected(int32 Index);

	/// Where the mouse points, clockwise from straight up. Drives the chevron, which follows the
	/// mouse freely rather than snapping to wedges. Starts pointing up; the caller leaves it where it
	/// was while the mouse is inside the dead zone.
	void SetPointer(float Degrees);

	int32 GetNumEntries() const { return Entries.Num(); }
	TSubclassOf<UFGItemDescriptor> GetEntryItem(int32 Index) const;
	bool IsClearEntry(int32 Index) const { return Entries.IsValidIndex(Index) && Entries[Index].bClear; }

protected:
	virtual void NativeOnInitialized() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	void UpdateLabels();

	UPROPERTY(Transient)
	TArray<FBackSlotRadialEntry> Entries;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CaptionLabel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameLabel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailLabel;

	FText EquippedName;
	int32 Selected = INDEX_NONE;
	float PointerDegrees = 0.0f;
};
