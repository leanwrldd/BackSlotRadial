#include "BackSlotRadialWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Resources/FGItemDescriptor.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "BackSlotRadial"

// Styled after the game's own build-mode radial: one dark disc, the hovered wedge lit orange
// between the inner ring and the rim, a white inner ring with a chevron pointing at the
// selection, and a ruled title block in the middle.
namespace
{
// See-through black disc, flat fully opaque orange for the hovered wedge.
const FLinearColor DiscColor(0.0f, 0.0f, 0.0f, 0.6f);
const FLinearColor SelectedColor = FLinearColor::FromSRGBColor(FColor(233, 147, 64, 255));
const FLinearColor RingColor(1.0f, 1.0f, 1.0f, 0.95f);
const FLinearColor RuleColor(1.0f, 1.0f, 1.0f, 0.85f);
const FLinearColor WedgeLabelColor(1.0f, 1.0f, 1.0f, 0.9f);
const FLinearColor DetailColor(1.0f, 1.0f, 1.0f, 0.45f);

constexpr float InnerRadius = 200.0f;
constexpr float OuterRadius = 335.0f;
constexpr float RingWidth = 3.0f;
constexpr float IconSize = 72.0f;
constexpr float ArcStepDegrees = 2.0f;
constexpr float RuleHalfWidth = 125.0f;
constexpr float RuleTopY = -21.0f;
constexpr float RuleBottomY = 25.0f;
constexpr int32 WedgeLabelSize = 11;
constexpr float ChevronThickness = 8.0f;
constexpr float ClearCrossSize = 50.0f;
constexpr float ClearCrossThickness = 10.0f;

// Clockwise from the top of the screen.
FVector2f Direction(float Degrees)
{
	const float radians = FMath::DegreesToRadians(Degrees);
	return FVector2f(FMath::Sin(radians), -FMath::Cos(radians));
}

UTextBlock* AddCenteredLabel(UWidgetTree* Tree, UCanvasPanel* Canvas, FName Name, int32 FontSize, float OffsetY, const FLinearColor& Color)
{
	UTextBlock* label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	FSlateFontInfo font = label->GetFont();
	font.Size = FontSize;
	label->SetFont(font);
	label->SetColorAndOpacity(FSlateColor(Color));
	label->SetJustification(ETextJustify::Center);
	label->SetShadowOffset(FVector2D(1.0f, 1.0f));
	label->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));

	UCanvasPanelSlot* slot = Canvas->AddChildToCanvas(label);
	slot->SetAnchors(FAnchors(0.5f, 0.5f));
	slot->SetAlignment(FVector2D(0.5f, 0.5f));
	slot->SetAutoSize(true);
	slot->SetPosition(FVector2D(0.0f, OffsetY));
	return label;
}

void DrawCircle(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry, const FVector2f& Center,
	float Radius, const FSlateBrush& Brush)
{
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		Geometry.ToPaintGeometry(FVector2f(Radius * 2.0f), FSlateLayoutTransform(Center - FVector2f(Radius))),
		// MakeBox fills with the tint passed here, not the brush's own colour (the outline colour does come from the brush).
		&Brush, ESlateDrawEffect::None, Brush.TintColor.GetSpecifiedColor());
}

// A solid bar with round ends from A to B, drawn as a rotated rounded box. Opaque capsules that
// overlap leave no seam, unlike a MakeLines joint, so the chevron and the X are built from these.
void DrawCapsule(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry, const FVector2f& A,
	const FVector2f& B, float Thickness, const FLinearColor& Color)
{
	const FVector2f along = B - A;
	const FVector2f size(along.Size() + Thickness, Thickness);
	const FSlateRenderTransform rotation(FQuat2f(FMath::Atan2(along.Y, along.X)));
	const FSlateRoundedBoxBrush brush(Color, Thickness * 0.5f);
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		Geometry.ToPaintGeometry(size, FSlateLayoutTransform((A + B) * 0.5f - size * 0.5f), rotation),
		&brush, ESlateDrawEffect::None, Color);
}

void DrawCenteredText(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry, const FVector2f& Center,
	const FText& Text, const FSlateFontInfo& Font, const FLinearColor& Color)
{
	const FVector2f size = FVector2f(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text, Font));
	const FVector2f topLeft = Center - size * 0.5f;
	FSlateDrawElement::MakeText(OutDrawElements, LayerId,
		Geometry.ToPaintGeometry(size, FSlateLayoutTransform(topLeft + FVector2f(1.0f, 1.0f))),
		Text, Font, ESlateDrawEffect::None, FLinearColor(0.0f, 0.0f, 0.0f, 0.6f * Color.A));
	FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
		Geometry.ToPaintGeometry(size, FSlateLayoutTransform(topLeft)),
		Text, Font, ESlateDrawEffect::None, Color);
}
}

void UBackSlotRadialWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree == nullptr || WidgetTree->RootWidget != nullptr)
	{
		return;
	}

	UCanvasPanel* canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
	WidgetTree->RootWidget = canvas;
	CaptionLabel = AddCenteredLabel(WidgetTree, canvas, TEXT("Caption"), 13, -38.0f, FLinearColor::White);
	NameLabel = AddCenteredLabel(WidgetTree, canvas, TEXT("ItemName"), 28, 2.0f, FLinearColor::White);
	DetailLabel = AddCenteredLabel(WidgetTree, canvas, TEXT("Detail"), 13, 46.0f, DetailColor);

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UBackSlotRadialWidget::SetContents(const TArray<TSubclassOf<UFGItemDescriptor>>& Items, TSubclassOf<UFGItemDescriptor> Equipped)
{
	Entries.Reset(Items.Num() + 1);

	// First, so it takes the top wedge (wedges run clockwise from the top).
	if (Equipped != nullptr)
	{
		FBackSlotRadialEntry& entry = Entries.AddDefaulted_GetRef();
		entry.bClear = true;
		entry.Name = LOCTEXT("Unequip", "Unequip");
	}

	for (const TSubclassOf<UFGItemDescriptor>& item : Items)
	{
		FBackSlotRadialEntry& entry = Entries.AddDefaulted_GetRef();
		entry.ItemClass = item;
		entry.Name = UFGItemDescriptor::GetItemName(item);
		entry.Icon.DrawAs = ESlateBrushDrawType::Image;
		entry.Icon.ImageSize = FVector2D(IconSize, IconSize);
		entry.Icon.SetResourceObject(UFGItemDescriptor::GetBigIcon(item));
	}

	EquippedName = Equipped != nullptr ? UFGItemDescriptor::GetItemName(Equipped) : LOCTEXT("EmptySlot", "Nothing");
	Selected = INDEX_NONE;
	UpdateLabels();
}

void UBackSlotRadialWidget::SetSelected(int32 Index)
{
	if (Selected != Index)
	{
		Selected = Index;
		UpdateLabels();
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void UBackSlotRadialWidget::SetPointer(float Degrees)
{
	if (FMath::IsNearlyEqual(PointerDegrees, Degrees, 0.1f))
	{
		return;
	}
	PointerDegrees = Degrees;
	// Slate caches painted output; nothing else changes when only the pointer moves, so ask for a repaint.
	Invalidate(EInvalidateWidgetReason::Paint);
}

TSubclassOf<UFGItemDescriptor> UBackSlotRadialWidget::GetEntryItem(int32 Index) const
{
	return Entries.IsValidIndex(Index) ? Entries[Index].ItemClass : TSubclassOf<UFGItemDescriptor>();
}

void UBackSlotRadialWidget::UpdateLabels()
{
	if (CaptionLabel == nullptr || NameLabel == nullptr || DetailLabel == nullptr)
	{
		return;
	}

	if (Entries.IsValidIndex(Selected))
	{
		CaptionLabel->SetText(Entries[Selected].bClear ? LOCTEXT("ClearSlot", "Clear Back Slot") : LOCTEXT("SwapTo", "Swap Back Slot"));
		NameLabel->SetText(Entries[Selected].Name);
		DetailLabel->SetText(FText::Format(LOCTEXT("CurrentlyEquipped", "Currently Equipped: {0}"), EquippedName));
	}
	else
	{
		CaptionLabel->SetText(Entries.IsEmpty() ? LOCTEXT("NothingToSwap", "Nothing to swap with") : LOCTEXT("Equipped", "Currently Equipped"));
		NameLabel->SetText(EquippedName);
		DetailLabel->SetText(FText::GetEmpty());
	}
}

int32 UBackSlotRadialWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2f center = FVector2f(AllottedGeometry.GetLocalSize()) * 0.5f;
	const FPaintGeometry paintGeometry = AllottedGeometry.ToPaintGeometry();
	const int32 count = Entries.Num();
	const float wedgeDegrees = count > 0 ? 360.0f / count : 360.0f;

	// The whole menu sits on one dark disc, centre included.
	const FSlateRoundedBoxBrush discBrush(DiscColor, OuterRadius);
	DrawCircle(OutDrawElements, LayerId, AllottedGeometry, center, OuterRadius, discBrush);

	// Only the hovered wedge is coloured, from the inner ring out to the rim.
	if (Entries.IsValidIndex(Selected))
	{
		const float startDegrees = Selected * wedgeDegrees - wedgeDegrees * 0.5f;
		const int32 segments = FMath::Max(FMath::CeilToInt(wedgeDegrees / ArcStepDegrees), 1);
		const float middleRadius = (InnerRadius + OuterRadius) * 0.5f;

		TArray<FVector2f> arc;
		arc.Reserve(segments + 1);
		for (int32 point = 0; point <= segments; ++point)
		{
			arc.Add(center + Direction(startDegrees + wedgeDegrees * point / segments) * middleRadius);
		}
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, paintGeometry, arc, ESlateDrawEffect::None,
			SelectedColor, true, OuterRadius - InnerRadius);
	}

	// White inner ring, with a chevron just inside it pointing at the selection.
	const FSlateRoundedBoxBrush ringBrush(FLinearColor::Transparent, InnerRadius, RingColor, RingWidth);
	DrawCircle(OutDrawElements, LayerId + 2, AllottedGeometry, center, InnerRadius, ringBrush);

	{
		// Always shown, pointing up until the mouse moves.
		const FVector2f direction = Direction(PointerDegrees);
		const FVector2f side(-direction.Y, direction.X);
		const FVector2f tip = center + direction * (InnerRadius - 22.0f);
		for (const float sign : { -1.0f, 1.0f })
		{
			DrawCapsule(OutDrawElements, LayerId + 4, AllottedGeometry, tip - direction * 24.0f + side * (30.0f * sign), tip,
				ChevronThickness, FLinearColor::White);
		}
	}

	// Rules above and below the item name, as on the build-mode title block.
	for (const float y : { RuleTopY, RuleBottomY })
	{
		const TArray<FVector2f> rule = {
			center + FVector2f(-RuleHalfWidth, y),
			center + FVector2f(RuleHalfWidth, y),
		};
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, paintGeometry, rule, ESlateDrawEffect::None,
			RuleColor, true, 1.5f);
	}

	// Each wedge shows its icon with the item name beneath it.
	const FSlateFontInfo wedgeFont = FCoreStyle::GetDefaultFontStyle("Regular", WedgeLabelSize);
	const float iconRadius = (InnerRadius + OuterRadius) * 0.5f;
	for (int32 index = 0; index < count; ++index)
	{
		const FVector2f iconCenter = center + Direction(index * wedgeDegrees) * iconRadius - FVector2f(0.0f, 10.0f);
		if (Entries[index].bClear)
		{
			const float half = ClearCrossSize * 0.5f;
			DrawCapsule(OutDrawElements, LayerId + 3, AllottedGeometry, iconCenter + FVector2f(-half, -half), iconCenter + FVector2f(half, half),
				ClearCrossThickness, FLinearColor::White);
			DrawCapsule(OutDrawElements, LayerId + 3, AllottedGeometry, iconCenter + FVector2f(-half, half), iconCenter + FVector2f(half, -half),
				ClearCrossThickness, FLinearColor::White);
		}
		else
		{
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
				AllottedGeometry.ToPaintGeometry(FVector2f(IconSize), FSlateLayoutTransform(iconCenter - FVector2f(IconSize * 0.5f))),
				&Entries[index].Icon, ESlateDrawEffect::None, FLinearColor::White);
		}
		DrawCenteredText(OutDrawElements, LayerId + 3, AllottedGeometry, iconCenter + FVector2f(0.0f, IconSize * 0.5f + 10.0f),
			Entries[index].Name, wedgeFont, WedgeLabelColor);
	}

	return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 5, InWidgetStyle, bParentEnabled);
}

#undef LOCTEXT_NAMESPACE
