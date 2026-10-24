#pragma once

#include "CoreMinimal.h"
#include "Real33DBridge.h"

class AReal33DWorld;
class SReal33DHUD;
class SReal33DLoginScreen;

/**
 * The opt-in live driver for UNREAL-LOGIN-CHARACTER-SELECT-UI-001's QA.
 *
 * WHAT IT IS. A scripted operator. It types into the account form's own text
 * boxes and presses the screen's own buttons with `SButton::SimulateClick`,
 * then presses the HUD's own Exit button, against a live Fusion32. It does not
 * call the bridge behind the UI, and it does not have a second path into the
 * protocol: every command that reaches the wire during a QA run reaches it
 * because a real control was pressed.
 *
 * WHY IT EXISTS. The claim this milestone has to support is a sequence of
 * player actions -- log in, pick a character, play, log out, come back --
 * against a server that is actually running. A unit test cannot make that
 * claim, because the thing under test is the sequence and the server's answers
 * to it; and a human at the GUI cannot make it reproducibly, because the next
 * reviewer has to be able to repeat the run. This is how the two are
 * reconciled, and it is the same shape as the `-real33d-minimap-qa` and
 * `-real33d-presentation-qa` captures already in this client.
 *
 * WHAT IT IS NOT. It is not part of the shipping flow and nothing reaches it
 * unless `-real33d-login-qa` is on the command line. It does not relax, skip
 * or shortcut any rule the UI enforces: a step that needs the Enter Game
 * button finds it disabled exactly when a player would.
 *
 * NO SLEEPS AND NO RETRY HACKS. Every step advances on an OBSERVED condition
 * -- a phase, a counter, an actor count -- never on elapsed time. The
 * per-step deadline exists only to end a run that is stuck, and reaching one
 * FAILS the run and says which step; it never lets the sequence continue as
 * though the step had passed.
 *
 * NO CREDENTIAL IS LOGGED. The password is read from the sanitized runtime
 * file, typed into the password box, and never written to the transcript, the
 * log or the evidence file. The wrong-password step uses a literal that is
 * deliberately not a credential.
 */
class REAL33D_API FReal33DLoginQA
{
public:
	/** True when `-real33d-login-qa` is on the command line. */
	static bool Requested();

	/**
	 * How many logout-and-return cycles to run after the first entry.
	 * `-real33d-login-qa-cycles=N`, default 3.
	 */
	static int32 RequestedCycles();

	/** Reads the runtime account and opens the transcript. */
	void Start(const FReal33DConnectionConfig& Config);

	/** Everything one step of the script needs to see. */
	struct FContext
	{
		UReal33DBridge* Bridge = nullptr;
		SReal33DLoginScreen* Screen = nullptr;
		SReal33DHUD* Hud = nullptr;
		AReal33DWorld* World = nullptr;
		double Now = 0.0;
	};

	/** Advances the script by at most one step. Game thread only. */
	void Tick(const FContext& Context);

	bool IsRunning() const { return bRunning; }
	bool HasFailed() const { return bFailed; }

private:
	/** One scripted action and the observed condition that completes it. */
	struct FStep
	{
		FString Name;
		/** Performed once, on entry. Returning false fails the run. */
		TFunction<bool(FReal33DLoginQA&, const FContext&)> Act;
		/** Polled every tick. True when the step's claim is observed. */
		TFunction<bool(FReal33DLoginQA&, const FContext&)> Done;
		double DeadlineSeconds = 20.0;
	};

	void BuildScript();
	void Record(const FString& Event, const FString& Detail);
	void Fail(const FString& Why);
	void Finish();

	/** Types into a box the way a player does, through its own SetText. */
	static bool Type(const TSharedPtr<class SEditableTextBox>& Box, const FString& Text);
	/** Presses a button the way a player does. False when it is disabled. */
	static bool Press(const TSharedPtr<class SButton>& Button);

	TArray<FStep> Script;
	int32 StepIndex = INDEX_NONE;
	bool bStepEntered = false;
	double StepStartedAt = 0.0;
	bool bRunning = false;
	bool bFailed = false;

	FReal33DConnectionConfig Config;
	uint32 AccountId = 0;
	/** Held for the length of the run, never logged, overwritten at the end. */
	FString Password;
	int32 Cycles = 3;
	int32 CycleIndex = 0;

	/** Counters captured on step entry, so a step proves a CHANGE. */
	int32 WalksAcceptedBefore = 0;
	int32 TilesBefore = 0;
	double LastWalkSentAt = 0.0;
	uint8 NextWalkDirection = 0;
	/**
	 * How many times Enter Game has been pressed for the current entry.
	 *
	 * More than one means Fusion32 turned an entry away -- it refuses a
	 * character whose logout has not finished -- and the player pressed
	 * again. Counted so the transcript distinguishes "the server said no" from
	 * "the client never asked".
	 */
	int32 EntryPresses = 0;
	/** INIT_GAME count read while the last session was still up. */
	int32 LastInitGames = 0;
	/** Sessions that actually reached the world. */
	int32 SessionsEntered = 0;

	FString TranscriptPath;
};
