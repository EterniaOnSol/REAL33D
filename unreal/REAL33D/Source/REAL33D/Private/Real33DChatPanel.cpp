#include "Real33DChatPanel.h"

#include "REAL33D.h"
#include "Real33DUIStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/ISlateStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	/** Speech. Corroborated as yellow by a third-party client, not by source. */
	const FLinearColor SpeechColour(1.0f, 0.93f, 0.40f, 1.0f);
	/** The server talking to this player. Distinct so the two never blur. */
	const FLinearColor ServerColour(0.55f, 0.85f, 1.0f, 1.0f);
	/** Dark enough to read pale text against the lit scene behind it. */
	const FLinearColor PanelColour(0.02f, 0.02f, 0.03f, 0.82f);

	constexpr float kPanelWidth = 560.0f;
	constexpr float kHistoryHeight = 190.0f;
}

// ----------------------------------------------------------------- the input

void SReal33DChatInput::Construct(const FArguments& InArgs)
{
	Panel = InArgs._OwningPanel;

	SEditableTextBox::Construct(
		SEditableTextBox::FArguments()
		.HintText(NSLOCTEXT("Real33D", "ChatHint", "Click here or press Enter to talk"))
		// CTalk reads into char Text[256] and readString truncates rather than
		// refusing, so an over-long line would arrive silently cut. Stopped at
		// the keyboard instead.
		.MaximumLength(255)
		// Escape restores what the box held when focus arrived, and that
		// restoration is itself a commit, which is how an abandoned line
		// reaches HandleTextCommitted and closes the gate.
		.RevertTextOnEscape(true)
		.ClearKeyboardFocusOnCommit(true)
		.OnTextCommitted(InArgs._OnTextCommitted));
}

FReply SReal33DChatInput::OnFocusReceived(const FGeometry& MyGeometry,
	const FFocusEvent& InFocusEvent)
{
	if (Panel != nullptr && InFocusEvent.GetCause() != EFocusCause::Cleared)
	{
		// Before the base class forwards focus inward, so the gate is already
		// open by the time the first character can possibly be typed.
		Panel->BeginTyping();
	}
	return SEditableTextBox::OnFocusReceived(MyGeometry, InFocusEvent);
}

// ----------------------------------------------------------------- the panel

FText SReal33DChatPanel::TabLabel(const FReal33DChatView& View)
{
	switch (View.Kind)
	{
	case EReal33DChatViewKind::Channel:
	case EReal33DChatViewKind::Private:
		// The server's own name in both cases. A name this client made up
		// would eventually be the wrong one: channel 0 is named after the
		// player's guild.
		return FText::FromString(View.Name);
	default:
		return NSLOCTEXT("Real33D", "ChatLocal", "Local Chat");
	}
}

FText SReal33DChatPanel::DestinationLabel(const FReal33DChatView& View)
{
	switch (View.Kind)
	{
	case EReal33DChatViewKind::Channel:
		return FText::FromString(FString::Printf(
			TEXT("to %s (channel %d)"), *View.Name, View.Channel));
	case EReal33DChatViewKind::Private:
		return FText::FromString(FString::Printf(TEXT("to %s, privately"), *View.Name));
	default:
		return FText::GetEmpty();
	}
}

bool SReal33DChatPanel::IsClosable(const FReal33DChatView& View)
{
	// Local Chat is where every unaddressed line goes. A console with nowhere
	// to put ordinary speech would discard what the server said.
	return View.Kind != EReal33DChatViewKind::Local;
}

TSharedRef<SWidget> SReal33DChatPanel::MakeTab(const FReal33DChatView& View, bool bSelected)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();
	const bool bClosable = IsClosable(View);

	const FText Tip = View.Kind == EReal33DChatViewKind::Local
		? NSLOCTEXT("Real33D", "ChatLocalTip",
			"Speech, the server's own messages, and this client's notices")
		: DestinationLabel(View);

	TSharedRef<SOverlay> Cell = SNew(SOverlay)
		+ SOverlay::Slot()
		[
			// console.otui: the selected cell sits on top of the sheet, the
			// idle one behind it.
			SNew(SImage).Image(Style.GetBrush(bSelected
				? "Real33D.Chrome.TabSelected" : "Real33D.Chrome.TabIdle"))
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(FMargin(2.0f, 0.0f, bClosable ? 14.0f : 2.0f, 0.0f))
		[
			SNew(STextBlock)
			.Text(TabLabel(View))
			.ColorAndOpacity(FSlateColor(FLinearColor(FColor(0xDF, 0xDF, 0xDF, 0xFF))))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		];

	if (bClosable)
	{
		// The 2D's own close box, in the corner of the cell. Closing a channel
		// tab leaves the channel; closing a private one just puts the
		// conversation away.
		Cell->AddSlot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.0f, 0.0f, 2.0f, 0.0f))
			[
				SNew(SButton)
				.ButtonStyle(FCoreStyle::Get(), "NoBorder")
				.ContentPadding(FMargin(0.0f))
				.ToolTipText(View.Kind == EReal33DChatViewKind::Channel
					? NSLOCTEXT("Real33D", "ChatCloseChannelTip",
						"Leaves this channel and closes the tab")
					: NSLOCTEXT("Real33D", "ChatClosePrivateTip",
						"Closes this conversation"))
				.OnClicked(FOnClicked::CreateSP(
					this, &SReal33DChatPanel::HandleTabClosed, View.ViewId))
				[
					SNew(SBox)
					.WidthOverride(10.0f)
					.HeightOverride(10.0f)
					[
						SNew(SImage).Image(Style.GetBrush("Real33D.Chrome.Close"))
					]
				]
			];
	}

	return SNew(SBox)
		.WidthOverride(96.0f)
		.HeightOverride(18.0f)
		.ToolTipText(Tip)
		[
			SNew(SButton)
			.ButtonStyle(FCoreStyle::Get(), "NoBorder")
			.ContentPadding(FMargin(0.0f))
			.OnClicked(FOnClicked::CreateSP(
				this, &SReal33DChatPanel::HandleTabClicked, View.ViewId))
			[
				Cell
			]
		];
}

TSharedRef<SWidget> SReal33DChatPanel::BuildChannelTabs()
{
	SAssignNew(Tabs, SHorizontalBox);
	RebuildTabs();
	return Tabs.ToSharedRef();
}

void SReal33DChatPanel::RebuildTabs()
{
	if (!Tabs.IsValid())
	{
		return;
	}
	Tabs->ClearChildren();

	const UReal33DBridge* Live = Bridge.Get();
	TArray<FReal33DChatView> Views;
	if (Live != nullptr)
	{
		Live->GetChatViews(Views);
	}
	if (Views.Num() == 0)
	{
		// No bridge at all: the gallery and headless modes still put this panel
		// on screen. One Local Chat tab, as before channels existed.
		FReal33DChatView Local;
		Views.Add(Local);
	}

	const uint32 Selected = Live != nullptr ? Live->GetSelectedChatView() : 0;
	for (const FReal33DChatView& View : Views)
	{
		Tabs->AddSlot()
			.AutoWidth()
			.Padding(FMargin(0.0f, 0.0f, 1.0f, 0.0f))
			[
				MakeTab(View, View.ViewId == Selected)
			];
	}

	// The input row's left-hand slot follows the tab: the mode choice belongs
	// to Local Chat alone.
	if (Destination.IsValid())
	{
		const FReal33DChatView Current = CurrentView();
		if (Current.Kind == EReal33DChatViewKind::Local)
		{
			Destination->SetContent(BuildModeSelector());
		}
		else
		{
			Destination->SetContent(
				SNew(STextBlock)
				.Text(DestinationLabel(Current))
				.ColorAndOpacity(FSlateColor(ServerColour))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8)));
		}
	}
}

FReal33DChatView SReal33DChatPanel::CurrentView() const
{
	const UReal33DBridge* Live = Bridge.Get();
	if (Live != nullptr)
	{
		TArray<FReal33DChatView> Views;
		Live->GetChatViews(Views);
		const uint32 Selected = Live->GetSelectedChatView();
		for (const FReal33DChatView& View : Views)
		{
			if (View.ViewId == Selected)
			{
				return View;
			}
		}
	}
	// A default-constructed view is Local Chat, which is the right answer both
	// when there is no bridge and when the selection has just been closed.
	return FReal33DChatView();
}

FReply SReal33DChatPanel::HandleTabClicked(uint32 ViewId)
{
	UReal33DBridge* Live = Bridge.Get();
	if (Live != nullptr && Live->SelectChatView(ViewId))
	{
		// Redrawn now rather than on the next revision change: selecting a tab
		// changes nothing in any transcript, so nothing else would redraw it.
		RebuildTabs();
		RebuildTranscript();
	}
	return FReply::Handled();
}

FReply SReal33DChatPanel::HandleTabClosed(uint32 ViewId)
{
	UReal33DBridge* Live = Bridge.Get();
	if (Live != nullptr && Live->CloseChatView(ViewId))
	{
		RebuildTabs();
		RebuildTranscript();
	}
	return FReply::Handled();
}

void SReal33DChatPanel::Construct(const FArguments& InArgs)
{
	Bridge = InArgs._Bridge;
	OnTypingChanged = InArgs._OnTypingChanged;
	bEmbedded = InArgs._Embedded;

	const ISlateStyle& Style = FReal33DUIStyle::Get();

	TSharedRef<SVerticalBox> Column = SNew(SVerticalBox);

	if (bEmbedded)
	{
		Column->AddSlot()
			.AutoHeight()
			.Padding(FMargin(3.0f, 0.0f, 0.0f, 0.0f))
			[
				BuildChannelTabs()
			];
	}

	// The transcript. Embedded it fills the bottom panel and wears the 2D's own
	// console frame; standalone it keeps its fixed height.
	{
		TSharedRef<SWidget> Transcript =
			SNew(SBorder)
			.BorderImage(Style.GetBrush(bEmbedded
				? "Real33D.Chrome.ConsoleFrame" : "Real33D.Chrome.WindowBody"))
			// Darker and more solid than the rest of the chrome. Everywhere
			// else a faded panel just shows scenery through it; here it shows
			// scenery through the text, and speech has to stay readable over
			// whatever the camera happens to be pointing at.
			.BorderBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.92f))
			.Padding(FMargin(4.0f))
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SImage)
					.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.ColorAndOpacity(FLinearColor(0.02f, 0.02f, 0.03f, 0.82f))
				]
				+ SOverlay::Slot()
				[
					SNew(SBox)
					.HeightOverride(bEmbedded ? FOptionalSize() : FOptionalSize(kHistoryHeight))
					[
						SAssignNew(History, SScrollBox)
						+ SScrollBox::Slot()
						[
							SAssignNew(Lines, SVerticalBox)
						]
					]
				]
			];

		// Fill and auto are different slot kinds, not one slot with a flag, so
		// the branch is here rather than inside the slot.
		if (bEmbedded)
		{
			Column->AddSlot().FillHeight(1.0f)[ Transcript ];
		}
		else
		{
			Column->AddSlot().AutoHeight()[ Transcript ];
		}
	}

	Column->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 4.0f, 0.0f, 0.0f))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				// Filled by RebuildTabs: the mode selector on Local Chat, the
				// destination on any other tab.
				SAssignNew(Destination, SBox)
				[
					BuildModeSelector()
				]
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(FMargin(6.0f, 0.0f))
			[
				SAssignNew(Input, SReal33DChatInput)
				.OwningPanel(this)
				.OnTextCommitted(FOnTextCommitted::CreateSP(
					this, &SReal33DChatPanel::HandleTextCommitted))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(NSLOCTEXT("Real33D", "ChatSend", "Send"))
				.OnClicked(FOnClicked::CreateSP(
					this, &SReal33DChatPanel::HandleSendClicked))
			]
		];

	if (bEmbedded)
	{
		// No dark box of its own: the HUD's bottom panel already draws the
		// repeating background console.otui puts behind this.
		ChildSlot
		.Padding(FMargin(4.0f, 2.0f, 4.0f, 4.0f))
		[
			Column
		];
		return;
	}

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(kPanelWidth)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(PanelColour)
			.Padding(FMargin(8.0f))
			[
				Column
			]
		]
	];
}

TSharedRef<SWidget> SReal33DChatPanel::BuildModeSelector()
{
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);

	// The three positional modes, and only those. The addressed and channel
	// modes CTalk also accepts need an addressee or a channel number this
	// client has no field for, and a command missing its required tail is one
	// the server refuses.
	static const EReal33DTalkMode Modes[] = {
		EReal33DTalkMode::Say, EReal33DTalkMode::Whisper, EReal33DTalkMode::Yell };

	for (const EReal33DTalkMode Each : Modes)
	{
		Row->AddSlot()
			.AutoWidth()
			.Padding(FMargin(0.0f, 0.0f, 4.0f, 0.0f))
			[
				SNew(SCheckBox)
				.Style(&FCoreStyle::Get().GetWidgetStyle<FCheckBoxStyle>(
					"RadioButton"))
				.IsChecked(this, &SReal33DChatPanel::IsModeChosen, Each)
				.OnCheckStateChanged(this, &SReal33DChatPanel::ChooseMode, Each)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Real33DTalkModeLabel(Each)))
				]
			];
	}
	return Row;
}

ECheckBoxState SReal33DChatPanel::IsModeChosen(EReal33DTalkMode Which) const
{
	return Mode == Which ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SReal33DChatPanel::ChooseMode(ECheckBoxState State, EReal33DTalkMode Which)
{
	if (State != ECheckBoxState::Checked)
	{
		// A radio group's other members report themselves unchecked as the
		// selection moves. Only the new selection carries information.
		return;
	}
	Mode = Which;
	UE_LOG(LogReal33D, Log, TEXT("talk mode is now %s"), Real33DTalkModeLabel(Mode));
}

TSharedPtr<SWidget> SReal33DChatPanel::GetInputWidget() const
{
	return Input;
}

void SReal33DChatPanel::BeginTyping()
{
	if (bTyping)
	{
		return;
	}
	bTyping = true;
	UE_LOG(LogReal33D, Log, TEXT("chat input active; movement keys are inert"));
	OnTypingChanged.ExecuteIfBound(true);
}

void SReal33DChatPanel::EndTyping()
{
	if (!bTyping)
	{
		return;
	}
	bTyping = false;
	UE_LOG(LogReal33D, Log, TEXT("chat input released; movement keys are live"));
	OnTypingChanged.ExecuteIfBound(false);
}

void SReal33DChatPanel::HandleTextCommitted(const FText& Text, ETextCommit::Type Cause)
{
	if (Cause == ETextCommit::OnEnter)
	{
		Send();
	}
	// Every other cause is the player leaving the box: focus moved, focus was
	// cleared, or Escape restored the original text. None of them sends, and
	// all of them must release the gate, or movement would stay dead with
	// nothing on screen explaining why.
	EndTyping();
}

FReply SReal33DChatPanel::HandleSendClicked()
{
	// Clicking the button takes focus from the box, which commits and closes the
	// gate before this runs. The text is still in the box: only Send clears it.
	Send();
	// Closed again here rather than assumed closed. If a future style made the
	// button non-focusable the commit would never fire, and the player would be
	// left unable to walk with nothing on screen explaining why.
	EndTyping();
	return FReply::Handled();
}

void SReal33DChatPanel::Send()
{
	if (!Input.IsValid())
	{
		return;
	}
	const FString Text = Input->GetText().ToString();
	if (Text.IsEmpty())
	{
		// CTalk refuses empty text, so an empty line is simply not a message.
		return;
	}

	UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr || !Live->IsRunning())
	{
		UE_LOG(LogReal33D, Warning, TEXT("not connected; \"%s\" not sent"), *Text);
		return;
	}

	// An intent, exactly like a walk. Nothing is drawn here: Fusion32 decides
	// who hears it, and the authoritative talk it broadcasts is what fills the
	// transcript. Rendering it locally as well would show the player their own
	// line twice, once for a message the server may never have accepted.
	//
	// Where it goes is the tab, not the mode selector. A channel line carries
	// the channel number of the tab it was typed in and a private line the
	// character of its conversation, so the command cannot leave here without
	// the field its mode requires.
	const FReal33DChatView View = CurrentView();
	switch (View.Kind)
	{
	case EReal33DChatViewKind::Channel:
	{
		const uint32 TalkId = Live->RequestChannelTalk(View.Channel, Text);
		UE_LOG(LogReal33D, Log,
			TEXT("channel %d talk %u: \"%s\" handed to Fusion32"),
			View.Channel, TalkId, *Text);
		break;
	}
	case EReal33DChatViewKind::Private:
	{
		const uint32 TalkId = Live->RequestPrivateTalk(View.Name, Text);
		// The sender gets no copy of their own private line: Fusion32 sends it
		// to the addressee and answers the sender "Message sent to X." That
		// confirmation lands in Local Chat, which is where SV_CMD_MESSAGE
		// always lands.
		UE_LOG(LogReal33D, Log,
			TEXT("private talk %u to %s: \"%s\" handed to Fusion32"),
			TalkId, *View.Name, *Text);
		break;
	}
	default:
	{
		const uint32 TalkId = Live->RequestTalk(Mode, Text);
		UE_LOG(LogReal33D, Log, TEXT("%s %u: \"%s\" handed to Fusion32"),
			Real33DTalkModeLabel(Mode), TalkId, *Text);
		break;
	}
	}

	Input->SetText(FText::GetEmpty());
}

void SReal33DChatPanel::Tick(const FGeometry& AllottedGeometry,
	const double CurrentTime, const float DeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	const UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr)
	{
		return;
	}
	const uint64 Revision = Live->GetChatRevision();
	const uint32 Selected = Live->GetSelectedChatView();
	if (bEverDrawn && Revision == DrawnRevision && Selected == DrawnViewId)
	{
		return;
	}
	DrawnRevision = Revision;
	DrawnViewId = Selected;
	bEverDrawn = true;
	// The strip too: the one revision covers both the transcripts and the set
	// of surfaces, so a channel that just opened gets its tab in the same frame
	// its first line could arrive.
	RebuildTabs();
	RebuildTranscript();
}

void SReal33DChatPanel::RebuildTranscript()
{
	if (!Lines.IsValid())
	{
		return;
	}

	const UReal33DBridge* Live = Bridge.Get();
	TArray<FReal33DChatLine> Transcript;
	if (Live != nullptr)
	{
		// The selected surface's own lines, never another's. One conversation's
		// transcript shown under another's name is the one mistake a multi-tab
		// console must not make.
		Live->GetChatTranscript(Live->GetSelectedChatView(), Transcript);
	}

	// Rebuilt whole rather than appended to, because the log evicts from the
	// front as well as growing at the back, and sixty text blocks is nothing.
	// This is also what makes a cleared transcript empty the panel on
	// disconnect instead of leaving the previous session's lines behind.
	Lines->ClearChildren();
	for (const FReal33DChatLine& Each : Transcript)
	{
		Lines->AddSlot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(Each.Line))
				.ColorAndOpacity(FSlateColor(
					Each.bSystemLine ? ServerColour : SpeechColour))
				.AutoWrapText(true)
			];
	}

	if (History.IsValid())
	{
		History->ScrollToEnd();
	}
}
