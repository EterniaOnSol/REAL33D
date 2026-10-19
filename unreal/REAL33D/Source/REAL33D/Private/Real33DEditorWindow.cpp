#include "Real33DEditorWindow.h"

#include "REAL33D.h"
#include "Real33DUIStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/ISlateStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	/** The parchment-ish body text of the 2D's book window. */
	const FLinearColor BodyColour(0.92f, 0.90f, 0.82f, 1.0f);
	/** Secondary lines: attribution, limits, and what the server said. */
	const FLinearColor NoteColour(0.62f, 0.68f, 0.78f, 1.0f);
	const FLinearColor PanelColour(0.03f, 0.03f, 0.04f, 0.94f);

	constexpr float kWindowWidth = 460.0f;
	constexpr float kBodyHeight = 220.0f;
	constexpr int32 kTextSize = 9;
}

FText SReal33DEditorWindow::Title(const FReal33DEditor& Editor)
{
	if (!Editor.bOpen)
	{
		return FText::GetEmpty();
	}
	if (!Editor.bIsText)
	{
		// The server's own type byte names this one.
		return FText::FromString(Real33DEditListTypeLabel(Editor.ListType));
	}
	// No name is on the wire for a text object, so none is invented.
	return NSLOCTEXT("Real33D", "EditorTextTitle", "Text");
}

FText SReal33DEditorWindow::Attribution(const FReal33DEditor& Editor)
{
	if (!Editor.bOpen || !Editor.bIsText)
	{
		// SendEditList writes no editor field at all, so there is nothing to
		// attribute and nothing is claimed.
		return FText::GetEmpty();
	}
	if (Editor.Editor.IsEmpty())
	{
		// The server sent an empty string, which is a fact about the object
		// and not a missing value: EDITOR is unset until someone writes.
		return NSLOCTEXT("Real33D", "EditorNobody", "Nobody has written on this.");
	}
	return FText::FromString(
		FString::Printf(TEXT("Written by %s."), *Editor.Editor));
}

FText SReal33DEditorWindow::LengthNote(const FReal33DEditor& Editor)
{
	if (!Editor.bOpen || !Editor.bIsText)
	{
		return FText::GetEmpty();
	}
	return FText::FromString(FString::Printf(
		TEXT("Fusion32 reported a limit of %u character(s) for this object."),
		Editor.MaxLength));
}

bool SReal33DEditorWindow::CanSubmit(const FReal33DEditor& Editor)
{
	if (!Editor.bOpen)
	{
		return false;
	}
	if (Editor.bIsText)
	{
		// CEditText answers NOTACCESSIBLE for an object that does not exist,
		// and zero is never a real object id.
		return Editor.ObjectId != 0;
	}
	// CEditList logs "Unbekannter Typ" and drops anything that is not one of
	// the three, so a list the server did not name has nowhere to go.
	return Editor.ListType != EReal33DEditListType::Unknown;
}

void SReal33DEditorWindow::Construct(const FArguments& InArgs)
{
	Bridge = InArgs._Bridge;
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	ChildSlot
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(SBox)
		.WidthOverride(kWindowWidth)
		[
			SNew(SBorder)
			.BorderImage(Style.GetBrush("Real33D.Chrome.WindowBody"))
			.BorderBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.96f))
			.Padding(FMargin(0.0f))
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SImage)
					.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.ColorAndOpacity(PanelColour)
				]
				+ SOverlay::Slot()
				[
					SNew(SVerticalBox)

					// Heading, on the 2D's own window header art.
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBorder)
						.BorderImage(Style.GetBrush("Real33D.Chrome.WindowHeader"))
						.Padding(FMargin(6.0f, 3.0f))
						[
							SAssignNew(Heading, STextBlock)
							.ColorAndOpacity(FSlateColor(BodyColour))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", kTextSize))
						]
					]

					// Which object or list this is. Ids rather than a name,
					// because the wire carries no name.
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(8.0f, 4.0f, 8.0f, 0.0f))
					[
						SAssignNew(DetailText, STextBlock)
						.ColorAndOpacity(FSlateColor(NoteColour))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					]

					// The body. Always editable: whether an edit is accepted
					// is Fusion32's ruling, not this window's guess.
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(8.0f, 4.0f))
					[
						SNew(SBox)
						.HeightOverride(kBodyHeight)
						[
							SAssignNew(Body, SMultiLineEditableTextBox)
							.AutoWrapText(true)
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", kTextSize))
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(8.0f, 0.0f))
					[
						SAssignNew(AttributionText, STextBlock)
						.ColorAndOpacity(FSlateColor(NoteColour))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.AutoWrapText(true)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(8.0f, 2.0f))
					[
						SAssignNew(LengthText, STextBlock)
						.ColorAndOpacity(FSlateColor(NoteColour))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.AutoWrapText(true)
					]

					// What became of the last submission, in the server's own
					// terms. Never "saved": nothing on the wire says that.
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(8.0f, 2.0f))
					[
						SAssignNew(StatusText, STextBlock)
						.ColorAndOpacity(FSlateColor(NoteColour))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.AutoWrapText(true)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(8.0f, 4.0f, 8.0f, 8.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						[
							SNew(SSpacer)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(FMargin(0.0f, 0.0f, 6.0f, 0.0f))
						[
							SAssignNew(SubmitButton, SButton)
							.Text(NSLOCTEXT("Real33D", "EditorSubmit", "Submit"))
							.ToolTipText(NSLOCTEXT("Real33D", "EditorSubmitTip",
								"Hands the text to Fusion32. Acceptance is silent: "
								"only a refusal answers, and the only proof it was "
								"kept is reading the object again."))
							.OnClicked(FOnClicked::CreateSP(
								this, &SReal33DEditorWindow::HandleSubmit))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.Text(NSLOCTEXT("Real33D", "EditorClose", "Close"))
							.ToolTipText(NSLOCTEXT("Real33D", "EditorCloseTip",
								"Closes the window and discards anything unsent. "
								"Fusion32 keeps no record that it was open."))
							.OnClicked(FOnClicked::CreateSP(
								this, &SReal33DEditorWindow::HandleClose))
						]
					]
				]
			]
		]
	];

	// Built collapsed. Fusion32 opens this window, never the client.
	SetVisibility(EVisibility::Collapsed);
}

void SReal33DEditorWindow::Refresh()
{
	const UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr)
	{
		return;
	}
	const uint64 Revision = Live->GetEditorRevision();
	if (bEverDrawn && Revision == DrawnRevision)
	{
		// Deliberately not rebuilt every frame: the body is an editable box
		// and refilling it under the player's caret would discard what they
		// are typing.
		return;
	}
	DrawnRevision = Revision;
	bEverDrawn = true;
	Rebuild();
}

void SReal33DEditorWindow::Rebuild()
{
	const UReal33DBridge* Live = Bridge.Get();
	const FReal33DEditor Editor = Live != nullptr
		? Live->GetEditor() : FReal33DEditor();

	bShowing = Editor.bOpen;
	// SelfHitTestInvisible, not Visible. This widget fills the whole overlay
	// slot so its box can be centred, and a plain Visible would make that
	// whole rectangle hit-testable -- swallowing every click on the panels
	// behind it for as long as a book was open, which is a regression in the
	// rest of the HUD. Self-invisible keeps the children clickable and lets
	// everything outside the box through. Collapsed still means closed, which
	// is what the panel registry reads.
	SetVisibility(Editor.bOpen
		? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);
	if (!Editor.bOpen)
	{
		return;
	}

	if (Heading.IsValid())      { Heading->SetText(Title(Editor)); }
	if (AttributionText.IsValid())
	{
		const FText Line = Attribution(Editor);
		AttributionText->SetText(Line);
		AttributionText->SetVisibility(
			Line.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible);
	}
	if (LengthText.IsValid())
	{
		const FText Line = LengthNote(Editor);
		LengthText->SetText(Line);
		LengthText->SetVisibility(
			Line.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible);
	}
	if (DetailText.IsValid())
	{
		DetailText->SetText(FText::FromString(Editor.bIsText
			? FString::Printf(TEXT("object %u, type %u"),
				Editor.ObjectId, Editor.TypeId)
			: FString::Printf(TEXT("%s, id %u"),
				Real33DEditListTypeLabel(Editor.ListType), Editor.ListId)));
	}
	if (StatusText.IsValid())
	{
		// Cleared on every new window: a note about the previous object must
		// not appear to describe this one.
		StatusText->SetText(FText::GetEmpty());
	}
	if (Body.IsValid())
	{
		Body->SetText(FText::FromString(Editor.Text));
	}
	if (SubmitButton.IsValid())
	{
		SubmitButton->SetEnabled(CanSubmit(Editor));
	}

	UE_LOG(LogReal33D, Log, TEXT("editor window shown: %s"),
		*Title(Editor).ToString());
}

FReply SReal33DEditorWindow::HandleSubmit()
{
	UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr || !Body.IsValid())
	{
		return FReply::Handled();
	}
	const FReal33DEditor Editor = Live->GetEditor();
	if (!CanSubmit(Editor))
	{
		return FReply::Handled();
	}

	const FString Text = Body->GetText().ToString();
	const uint32 ActionId = Editor.bIsText
		? Live->RequestEditText(Editor.ObjectId, Text)
		: Live->RequestEditList(Editor.ListType, Editor.ListId, Text);

	if (StatusText.IsValid())
	{
		// The exact claim this client is entitled to make, and no more.
		StatusText->SetText(ActionId != 0
			? NSLOCTEXT("Real33D", "EditorHandedOver",
				"Handed to Fusion32. It does not acknowledge an accepted edit; "
				"a refusal appears in Local Chat, and reading the object again "
				"is what shows whether it was kept.")
			: NSLOCTEXT("Real33D", "EditorNotSent",
				"Not sent: this client is not connected."));
	}
	UE_LOG(LogReal33D, Log, TEXT("editor submit %u: %d char(s)"),
		ActionId, Text.Len());

	// The window stays open on purpose. Closing it here would look like the
	// confirmation Fusion32 never sends.
	return FReply::Handled();
}

FReply SReal33DEditorWindow::HandleClose()
{
	UReal33DBridge* Live = Bridge.Get();
	if (Live != nullptr)
	{
		Live->CloseEditor();
	}
	bShowing = false;
	SetVisibility(EVisibility::Collapsed);
	return FReply::Handled();
}
