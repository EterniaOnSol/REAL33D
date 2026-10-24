#include "Real33DLoginScreen.h"

#include "Framework/Application/SlateApplication.h"
#include "REAL33D.h"
#include "Real33DUIStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/ISlateStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FLinearColor BodyColour(0.92f, 0.90f, 0.82f, 1.0f);
	const FLinearColor NoteColour(0.62f, 0.68f, 0.78f, 1.0f);
	const FLinearColor MessageColour(0.92f, 0.66f, 0.44f, 1.0f);
	const FLinearColor PanelColour(0.03f, 0.03f, 0.04f, 0.94f);
	const FLinearColor ScrimColour(0.0f, 0.0f, 0.0f, 0.72f);
	const FLinearColor RowColour(0.10f, 0.11f, 0.13f, 1.0f);
	const FLinearColor ChosenRowColour(0.18f, 0.26f, 0.34f, 1.0f);

	constexpr float kWindowWidth = 380.0f;
	constexpr float kListHeight = 190.0f;
	constexpr int32 kTextSize = 9;

	/** The outfit milestone's lesson: UE 5.8's default button padding is big. */
	const FMargin kButtonPadding(10.0f, 3.0f);
}

FText SReal33DLoginScreen::Title()
{
	return NSLOCTEXT("Real33D", "LoginTitle", "REAL33D - Fusion32 7.72");
}

FText SReal33DLoginScreen::CharacterRowText(const FReal33DCharacterEntry& Character)
{
	// Both, always, and both the server's. The world matters even on a
	// single-world runtime: it is the only thing on the row that says which
	// game service the entry will dial, and a list that showed only names
	// would be unreadable the first time an account spans two worlds.
	if (Character.WorldName.IsEmpty())
	{
		return FText::FromString(Character.Name);
	}
	return FText::FromString(FString::Printf(TEXT("%s   -   %s"),
		*Character.Name, *Character.WorldName));
}

FText SReal33DLoginScreen::PhaseText(Real33D::SessionPhase Phase)
{
	switch (Phase)
	{
	case Real33D::SessionPhase::LoggingIn:
		return NSLOCTEXT("Real33D", "LoginPhaseLoggingIn",
			"Asking the login service...");
	case Real33D::SessionPhase::CharacterSelect:
		return NSLOCTEXT("Real33D", "LoginPhaseSelect",
			"Choose a character.");
	case Real33D::SessionPhase::EnteringGame:
		return NSLOCTEXT("Real33D", "LoginPhaseEntering",
			"Entering the game...");
	default:
		return FText::GetEmpty();
	}
}

FText SReal33DLoginScreen::NoteText()
{
	return NSLOCTEXT("Real33D", "LoginNote",
		"Login asks Fusion32. The characters below are the ones the server "
		"returned for this account; this client has no list of its own. Your "
		"password is sent to the login service and to the game service and is "
		"never written to a log or a file.");
}

void SReal33DLoginScreen::Construct(const FArguments& InArgs)
{
	Bridge = InArgs._Bridge;
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	const TSharedRef<SWidget> AccountForm = MakeAccountForm();
	const TSharedRef<SWidget> CharacterList = MakeCharacterList();
	Panels.Register(Real33D::PanelIds::Account, AccountForm);
	Panels.Register(Real33D::PanelIds::CharacterSelect, CharacterList);

	ChildSlot
	[
		SNew(SOverlay)

		// The scrim. The 3D scene is behind this -- an empty one before the
		// first login, the torn-down one after a session ends -- and a form
		// typed into over a live-looking world reads as a window that can be
		// dismissed. It cannot: there is no session behind it.
		+ SOverlay::Slot()
		[
			SNew(SImage)
			.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.ColorAndOpacity(ScrimColour)
		]

		+ SOverlay::Slot()
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

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBorder)
							.BorderImage(Style.GetBrush("Real33D.Chrome.WindowHeader"))
							.Padding(FMargin(6.0f, 3.0f))
							[
								SNew(STextBlock)
								.Text(Title())
								.ColorAndOpacity(FSlateColor(BodyColour))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", kTextSize))
							]
						]

						// The two surfaces, one above the other. Only one is
						// ever visible; the registry owns which.
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							AccountForm
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							CharacterList
						]

						// The message line. The server's own words whenever
						// Fusion32 produced any; see NoteSession.
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(10.0f, 2.0f, 10.0f, 0.0f))
						[
							SAssignNew(MessageLabel, STextBlock)
							.ColorAndOpacity(FSlateColor(MessageColour))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
							.AutoWrapText(true)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(10.0f, 6.0f, 10.0f, 8.0f))
						[
							SNew(STextBlock)
							.Text(NoteText())
							.ColorAndOpacity(FSlateColor(NoteColour))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
							.AutoWrapText(true)
						]
					]
				]
			]
		]
	];

	ShowSurface(Real33D::PanelIds::Account);
	RefreshControls();
}

TSharedRef<SWidget> SReal33DLoginScreen::MakeAccountForm()
{
	return SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(10.0f, 8.0f, 10.0f, 0.0f))
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("Real33D", "LoginAccount", "Account number"))
			.ColorAndOpacity(FSlateColor(NoteColour))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(10.0f, 2.0f, 10.0f, 0.0f))
		[
			SAssignNew(AccountBox, SEditableTextBox)
			.HintText(NSLOCTEXT("Real33D", "LoginAccountHint", "Account number"))
			.OnTextCommitted(this, &SReal33DLoginScreen::HandleAccountCommitted)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(10.0f, 6.0f, 10.0f, 0.0f))
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("Real33D", "LoginPassword", "Password"))
			.ColorAndOpacity(FSlateColor(NoteColour))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(10.0f, 2.0f, 10.0f, 0.0f))
		[
			SAssignNew(PasswordBox, SEditableTextBox)
			// Not echoed. The one presentation rule here that is a security
			// rule as well, and the reason this is a dedicated box rather
			// than a second plain field.
			.IsPassword(true)
			.HintText(NSLOCTEXT("Real33D", "LoginPasswordHint", "Password"))
			.OnTextCommitted(this, &SReal33DLoginScreen::HandlePasswordCommitted)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(10.0f, 8.0f, 10.0f, 0.0f))
		.HAlign(HAlign_Right)
		[
			SAssignNew(LoginButton, SButton)
			.ContentPadding(kButtonPadding)
			.Text(NSLOCTEXT("Real33D", "LoginSubmit", "Login"))
			.ToolTipText(NSLOCTEXT("Real33D", "LoginSubmitTip",
				"Asks the Fusion32 login service for this account's characters."))
			.OnClicked(this, &SReal33DLoginScreen::HandleLogin)
		];
}

TSharedRef<SWidget> SReal33DLoginScreen::MakeCharacterList()
{
	return SNew(SVerticalBox)

		// The server's message of the day, when it sent one. Nothing stands in
		// for it when it did not: an empty MOTD is a fact about the server.
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(10.0f, 8.0f, 10.0f, 0.0f))
		[
			SAssignNew(MotdLabel, STextBlock)
			.ColorAndOpacity(FSlateColor(NoteColour))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			.AutoWrapText(true)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(10.0f, 6.0f, 10.0f, 2.0f))
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("Real33D", "LoginCharacters", "Characters"))
			.ColorAndOpacity(FSlateColor(BodyColour))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(10.0f, 0.0f, 10.0f, 0.0f))
		[
			SNew(SBox)
			.HeightOverride(kListHeight)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SAssignNew(CharacterRows, SVerticalBox)
				]
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(10.0f, 4.0f, 10.0f, 0.0f))
		[
			SAssignNew(PremiumLabel, STextBlock)
			.ColorAndOpacity(FSlateColor(NoteColour))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(10.0f, 8.0f, 10.0f, 0.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.ContentPadding(kButtonPadding)
				.Text(NSLOCTEXT("Real33D", "LoginBack", "Back"))
				.ToolTipText(NSLOCTEXT("Real33D", "LoginBackTip",
					"Forgets this account and returns to the account form."))
				.OnClicked(this, &SReal33DLoginScreen::HandleBack)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNullWidget::NullWidget
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(EnterButton, SButton)
				.ContentPadding(kButtonPadding)
				.Text(NSLOCTEXT("Real33D", "LoginEnter", "Enter Game"))
				.ToolTipText(NSLOCTEXT("Real33D", "LoginEnterTip",
					"Presents this character to the Fusion32 game service."))
				.OnClicked(this, &SReal33DLoginScreen::HandleEnterGame)
			]
		];
}

void SReal33DLoginScreen::ShowSurface(FName Surface)
{
	// One owner, both ways: the surface that is not named is closed in the
	// same call that opens the one that is, so there is no frame in which
	// both are up and none in which neither is.
	Panels.SetOpen(Real33D::PanelIds::Account, Surface == Real33D::PanelIds::Account);
	Panels.SetOpen(Real33D::PanelIds::CharacterSelect,
		Surface == Real33D::PanelIds::CharacterSelect);
}

void SReal33DLoginScreen::ShowAccountForm(const FText& Message)
{
	ShowSurface(Real33D::PanelIds::Account);
	Characters.Reset();
	ListedCharacters = 0;
	SelectedCharacter = INDEX_NONE;
	if (CharacterRows.IsValid())
	{
		CharacterRows->ClearChildren();
	}
	if (MessageLabel.IsValid())
	{
		MessageLabel->SetText(Message);
	}
	RefreshControls();
}

void SReal33DLoginScreen::ShowCharacterList(
	const TArray<FReal33DCharacterEntry>& InCharacters, const FString& Motd,
	int32 PremiumDays, const FText& Message)
{
	Characters = InCharacters;
	ShowSurface(Real33D::PanelIds::CharacterSelect);
	RebuildCharacterRows();

	if (MotdLabel.IsValid())
	{
		MotdLabel->SetText(FText::FromString(Motd));
		// Collapsed rather than left as an empty line: a gap where a message
		// would be reads as a message that failed to load.
		MotdLabel->SetVisibility(
			Motd.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible);
	}
	if (PremiumLabel.IsValid())
	{
		// The server's number, stated plainly. Zero is a fact about the
		// account and is shown as one rather than hidden.
		PremiumLabel->SetText(FText::FromString(
			FString::Printf(TEXT("Premium days: %d"), PremiumDays)));
	}
	if (MessageLabel.IsValid())
	{
		MessageLabel->SetText(Message);
	}

	// One character is picked for them. A list of one is the ordinary case on
	// this runtime, and making the player click it before Enter Game would be
	// a step that answers a question with only one answer.
	if (!Characters.IsEmpty())
	{
		SelectCharacter(0);
	}
	RefreshControls();
}

void SReal33DLoginScreen::RebuildCharacterRows()
{
	SelectedCharacter = INDEX_NONE;
	ListedCharacters = 0;
	if (!CharacterRows.IsValid())
	{
		return;
	}
	CharacterRows->ClearChildren();

	for (int32 Index = 0; Index < Characters.Num(); ++Index)
	{
		CharacterRows->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 0.0f, 0.0f, 2.0f))
		[
			SNew(SButton)
			.ContentPadding(FMargin(6.0f, 4.0f))
			.HAlign(HAlign_Left)
			.ButtonColorAndOpacity_Lambda([this, Index]() -> FSlateColor
			{
				return FSlateColor(
					SelectedCharacter == Index ? ChosenRowColour : RowColour);
			})
			.OnClicked_Lambda([this, Index]()
			{
				SelectCharacter(Index);
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(CharacterRowText(Characters[Index]))
				.ColorAndOpacity(FSlateColor(BodyColour))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			]
		];
		++ListedCharacters;
	}

	if (Characters.IsEmpty())
	{
		// An account with no characters. Said plainly, because it is a fact
		// about the account and not a failure of the client.
		CharacterRows->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("Real33D", "LoginNoCharacters",
				"This account has no characters on this world."))
			.ColorAndOpacity(FSlateColor(NoteColour))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			.AutoWrapText(true)
		];
	}
}

bool SReal33DLoginScreen::SelectCharacter(int32 Index)
{
	// Refused rather than clamped. Clamping an index nobody asked for would
	// enter the game as a character the player never picked.
	if (!Characters.IsValidIndex(Index))
	{
		return false;
	}
	SelectedCharacter = Index;
	RefreshControls();
	return true;
}

FText SReal33DLoginScreen::GetMessage() const
{
	return MessageLabel.IsValid() ? MessageLabel->GetText() : FText::GetEmpty();
}

void SReal33DLoginScreen::RefreshControls()
{
	const bool bBusy = DrawnPhase == Real33D::SessionPhase::LoggingIn
		|| DrawnPhase == Real33D::SessionPhase::EnteringGame;
	if (LoginButton.IsValid())
	{
		// Dithered out while a request is in flight. This is the control that
		// a double press turns into two login connections for one account.
		LoginButton->SetEnabled(!bBusy);
	}
	if (EnterButton.IsValid())
	{
		// And this is the one a double press turns into two game connections,
		// which Fusion32 answers by displacing the first -- two INIT_GAMEs for
		// one player-visible session. The bridge refuses it as well; both, so
		// neither is the only guard.
		EnterButton->SetEnabled(!bBusy && Characters.IsValidIndex(SelectedCharacter));
	}
}

FReply SReal33DLoginScreen::HandleLogin()
{
	UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr || !AccountBox.IsValid() || !PasswordBox.IsValid())
	{
		return FReply::Handled();
	}

	const FString Typed = AccountBox->GetText().ToString().TrimStartAndEnd();
	const uint32 AccountNumber =
		static_cast<uint32>(FCString::Strtoui64(*Typed, nullptr, 10));

	// Handed over and immediately forgotten here. The box is cleared whether
	// or not the bridge accepted it: a password left in a widget outlives the
	// request it belonged to, and there is no reason for this screen to be
	// able to answer what it was.
	const FString Password = PasswordBox->GetText().ToString();
	Live->BeginAccountLogin(AccountNumber, Password);
	PasswordBox->SetText(FText::GetEmpty());

	// Refresh picks up whatever the bridge made of it -- a phase change, or a
	// refusal with a message -- on this frame rather than the next.
	Refresh();
	return FReply::Handled();
}

FReply SReal33DLoginScreen::HandleEnterGame()
{
	UReal33DBridge* Live = Bridge.Get();
	if (Live != nullptr && Characters.IsValidIndex(SelectedCharacter))
	{
		Live->EnterGame(SelectedCharacter);
		Refresh();
	}
	return FReply::Handled();
}

FReply SReal33DLoginScreen::HandleBack()
{
	if (UReal33DBridge* Live = Bridge.Get())
	{
		Live->ReturnToAccountScreen();
		Refresh();
	}
	else
	{
		ShowAccountForm(FText::GetEmpty());
	}
	return FReply::Handled();
}

void SReal33DLoginScreen::HandleAccountCommitted(const FText& /*Text*/,
	ETextCommit::Type Commit)
{
	// Enter moves on rather than submitting: an account number with no
	// password is a login this client knows will be refused, and sending it
	// would cost a round trip to say so.
	if (Commit == ETextCommit::OnEnter && PasswordBox.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(PasswordBox);
	}
}

void SReal33DLoginScreen::HandlePasswordCommitted(const FText& /*Text*/,
	ETextCommit::Type Commit)
{
	if (Commit == ETextCommit::OnEnter)
	{
		HandleLogin();
	}
}

void SReal33DLoginScreen::Refresh()
{
	UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr)
	{
		return;
	}

	const Real33D::SessionPhase Phase = Live->GetSessionPhase();
	const uint64 Revision = Live->GetSessionRevision();

	// The whole screen follows one rule, so it cannot be up over a live game
	// or absent over a dead one.
	SetVisibility(Real33D::ShowsLoginSurface(Phase)
		? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);

	if (Revision == DrawnRevision && Phase == DrawnPhase)
	{
		return;
	}
	DrawnRevision = Revision;
	DrawnPhase = Phase;

	// The server's own words when there are any; this client's description of
	// what it is doing only when there are not.
	const FString& ServerMessage = Live->GetSessionMessage();
	const FText Message = ServerMessage.IsEmpty()
		? PhaseText(Phase) : FText::FromString(ServerMessage);

	switch (Phase)
	{
	case Real33D::SessionPhase::CharacterSelect:
		ShowCharacterList(Live->GetCharacters(), Live->GetMotd(),
			Live->GetPremiumDays(), Message);
		break;

	case Real33D::SessionPhase::Offline:
	case Real33D::SessionPhase::LoggingIn:
		ShowAccountForm(Message);
		break;

	default:
		// EnteringGame, InGame and Reconnecting: the screen is collapsed and
		// there is nothing to lay out. Deliberately not cleared -- a logout
		// returns to the list that is already built.
		break;
	}

	RefreshControls();
}
