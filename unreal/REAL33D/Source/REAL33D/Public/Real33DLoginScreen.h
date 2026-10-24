#pragma once

#include "CoreMinimal.h"
#include "Real33DBridge.h"
#include "Real33DPanelRegistry.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SButton;
class SEditableTextBox;
class STextBlock;
class SVerticalBox;

/**
 * Everything the player sees before a character is in the world.
 *
 * Two surfaces on one widget: the account form, and the character list the
 * login service returned. Exactly one is visible at a time, and which one is
 * `FReal33DPanelRegistry`'s answer rather than a flag kept here -- the same
 * rule, and the same owner, the HUD's panels use. Registered under
 * `Real33D::PanelIds::Account` and `Real33D::PanelIds::CharacterSelect`.
 *
 * WHAT THIS WIDGET DOES NOT DO:
 *
 *   * it does not authenticate anything. Pressing Login asks the login
 *     service; a wrong password is Fusion32's ruling and arrives as the
 *     server's own text, which is what the message line shows. This client
 *     never decides that a password is wrong and never checks one locally;
 *   * it does not name a character. Every row comes from SV_CMD_CHARACTER_LIST
 *     and nothing is drawn from a cache, a previous run or a settings file.
 *     An account with no characters shows no rows, because that is the truth
 *     about the account;
 *   * it does not keep the password. The box holds it while it is being typed
 *     and it is cleared the instant it has been handed to the bridge, so it is
 *     not sitting in a widget for the rest of the session, and it is never put
 *     in a log line, an event or an evidence file.
 *
 * The character list is NOT re-queried when a session ends. 7.72's game login
 * re-authenticates from scratch -- the account, the character and the password
 * travel in the game service's own RSA block -- so the list is not a ticket
 * that expires, and a second trip to the login service before every relogin
 * would be a round trip that proves nothing and one more way for a relogin to
 * fail. `Back` returns to the account form, which does re-query.
 *
 * Pure Slate, built in C++, so the project still carries no .uasset.
 */
class REAL33D_API SReal33DLoginScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DLoginScreen) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UReal33DBridge>, Bridge)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/**
	 * Matches the screen to the bridge's session, once a frame.
	 *
	 * Reads `GetSessionRevision` first and does nothing when it has not moved,
	 * so the list is rebuilt when the session changes rather than every frame.
	 * Game thread only, like the rest of the UI.
	 */
	void Refresh();

	// ------------------------------------------------------- what is showing
	//
	// Read from the registry, which reads the widgets, so these cannot
	// disagree with what is on screen. Used by the player controller to decide
	// whether the HUD is drawn, and by the automation tests.

	bool IsAccountFormShowing() const
	{
		return Panels.IsOpen(Real33D::PanelIds::Account);
	}
	bool IsCharacterListShowing() const
	{
		return Panels.IsOpen(Real33D::PanelIds::CharacterSelect);
	}

	/** How many character rows are drawn. The server's count, never padded. */
	int32 GetListedCharacters() const { return ListedCharacters; }

	/** The row the player has picked, or INDEX_NONE before they pick one. */
	int32 GetSelectedCharacter() const { return SelectedCharacter; }

	/** The message line, as the player reads it. */
	FText GetMessage() const;

	/** The screen's panels, for the controller and the tests. */
	FReal33DPanelRegistry& GetPanels() { return Panels; }
	const FReal33DPanelRegistry& GetPanels() const { return Panels; }

	// ------------------------------------------------------- the controls
	//
	// The actual widgets, so the opt-in live QA driver can press the buttons
	// a player presses rather than calling the bridge behind them. Exposed,
	// not duplicated: a driver with its own path to the bridge would prove
	// the bridge and leave the screen -- the enable rules, the selection, the
	// password handling -- untested in the live run, which is the half that
	// only exists here.
	//
	// Nothing in the shipping flow uses these. See Real33DLoginQA.h.

	TSharedPtr<SEditableTextBox> GetAccountBox() const { return AccountBox; }
	TSharedPtr<SEditableTextBox> GetPasswordBox() const { return PasswordBox; }
	TSharedPtr<SButton> GetLoginButton() const { return LoginButton; }
	TSharedPtr<SButton> GetEnterButton() const { return EnterButton; }

	// -------------------------------------------------- presentation, direct
	//
	// The entry points `Refresh` drives, exposed so the flow can be exercised
	// without a bridge, a server or a human: an automation test shows a list
	// of its own and checks what the screen does with it.

	/** Shows the account form, with an optional message above the fields. */
	void ShowAccountForm(const FText& Message);

	/**
	 * Shows a character list. `Motd` and `PremiumDays` are the login
	 * service's; neither is invented when the server sent none.
	 */
	void ShowCharacterList(const TArray<FReal33DCharacterEntry>& InCharacters,
		const FString& Motd, int32 PremiumDays, const FText& Message);

	/** Picks a row. Out-of-range is refused, not clamped. */
	bool SelectCharacter(int32 Index);

	// ---------------------------------------------------------- display rules
	//
	// Static and free of Slate, so each is testable without a renderer.

	/** The screen's title. */
	static FText Title();

	/** One row: the character and the world the server named for it. */
	static FText CharacterRowText(const FReal33DCharacterEntry& Character);

	/**
	 * What the client is doing, in words, for a phase that has no message of
	 * its own. Never a substitute for the server's text: when Fusion32 said
	 * something, that is what the message line shows.
	 */
	static FText PhaseText(Real33D::SessionPhase Phase);

	/** What this screen is entitled to claim, which is never "logged in". */
	static FText NoteText();

private:
	FReply HandleLogin();
	FReply HandleEnterGame();
	FReply HandleBack();
	void HandleAccountCommitted(const FText& Text, ETextCommit::Type Commit);
	void HandlePasswordCommitted(const FText& Text, ETextCommit::Type Commit);

	/** Rebuilds the rows from `Characters`. Clears the selection. */
	void RebuildCharacterRows();

	/** Enables and disables the buttons from the phase and the selection. */
	void RefreshControls();

	/** Shows one of the two surfaces and hides the other. One owner. */
	void ShowSurface(FName Surface);

	TSharedRef<SWidget> MakeAccountForm();
	TSharedRef<SWidget> MakeCharacterList();

	TWeakObjectPtr<UReal33DBridge> Bridge;

	TSharedPtr<SEditableTextBox> AccountBox;
	TSharedPtr<SEditableTextBox> PasswordBox;
	TSharedPtr<SButton> LoginButton;
	TSharedPtr<SButton> EnterButton;
	TSharedPtr<STextBlock> MessageLabel;
	TSharedPtr<STextBlock> MotdLabel;
	TSharedPtr<STextBlock> PremiumLabel;
	TSharedPtr<SVerticalBox> CharacterRows;

	/** The two surfaces, so visibility keeps one owner. */
	FReal33DPanelRegistry Panels;

	/** The list as it was last drawn. A copy, never a pointer into the bridge. */
	TArray<FReal33DCharacterEntry> Characters;
	int32 ListedCharacters = 0;
	int32 SelectedCharacter = INDEX_NONE;

	/**
	 * The bridge revision this screen was last built from.
	 *
	 * Starts at a value the bridge can never report, so the first Refresh
	 * always builds: zero is the revision a bridge has before anything has
	 * happened, and a screen that skipped that frame would come up blank.
	 */
	uint64 DrawnRevision = TNumericLimits<uint64>::Max();

	/** The phase last drawn, so the buttons follow it without a rebuild. */
	Real33D::SessionPhase DrawnPhase = Real33D::SessionPhase::Offline;
};
