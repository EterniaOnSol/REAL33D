#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Real33DPlayerController.generated.h"

class AReal33DWorld;
class SReal33DChatPanel;
class STextBlock;
class SEditableTextBox;

/**
 * Turns key presses into intents and nothing else.
 *
 * There is deliberately no path from a key to an Actor's position here. A press
 * becomes a walk request on the bridge, the bridge hands it to Protocol772Core,
 * Protocol772Core asks Fusion32, and only when Fusion32 answers does WorldState
 * change and the creature actor follow. If Fusion32 refuses the step nothing at
 * all happens on screen, which is the correct outcome: the player did not move.
 *
 * Bindings are made in code with BindKey so the project carries no binary input
 * assets. Direction values are the server's own, from enums.hh.
 */
UCLASS()
class REAL33D_API AReal33DPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AReal33DPlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void WalkNorth();
	void WalkEast();
	void WalkSouth();
	void WalkWest();
	void WalkForward();
	void WalkRight();
	void WalkBackward();
	void WalkLeft();
	void ReleaseForward();
	void ReleaseRight();
	void ReleaseBackward();
	void ReleaseLeft();
	void SetMovementHeld(uint8 RelativeDirection, bool bHeld);
	void DumpEvidence();
	void InspectUnderCursor();
	void SaveInspectorNote(const FString& Verdict);

	void Request(uint8 Direction);
	void RequestRelative(uint8 RelativeDirection);

	// ----------------------------------------------------------- camera orbit
	//
	// Held right mouse button plus mouse movement swings the view around the
	// player; the wheel moves it in and out. The controller decides nothing
	// about where the camera ends up: it converts a gesture into a delta in
	// degrees and hands it to the world actor, which owns the limits.
	//
	// The axis handlers fire on every mouse move whether or not the button is
	// down, so each one leaves immediately unless a drag is in progress. No
	// input mode is changed and no focus is taken: pressing a mouse button over
	// the viewport already moves focus off the chat box, and the panel reports
	// that through the same signal it always did.

	void BeginOrbit();
	void EndOrbit();
	void OrbitYaw(float Value);
	void OrbitPitch(float Value);
	void ZoomCamera(float Value);

	/**
	 * The world actor, found once and remembered.
	 *
	 * Deliberately not a TActorIterator per call: mouse axes fire several times
	 * a frame and the world holds thousands of tile actors, so searching it on
	 * every mouse move would cost more than everything else this class does.
	 */
	AReal33DWorld* GetWorldActor();

	TWeakObjectPtr<AReal33DWorld> WorldActor;

	/** True between right button down and up. */
	bool bOrbiting = false;
	bool bHeldMovement[4] = { false, false, false, false };
	uint8 ActiveHeldDirection = 0;
	double LastWalkIntentTime = 0.0;
	static constexpr double HeldWalkIntervalSeconds = 0.45;

	// ------------------------------------------------------------ chat input
	//
	// The chat area owns typing; this class owns movement. They meet at one
	// boolean.
	//
	// While the player is typing the walk keys are inert, checked at the top of
	// Request rather than by unbinding and rebinding: one place holds the rule
	// and there is no window in which the bindings are half-swapped. Typing
	// "was" must not walk the player west, north and south.
	//
	// The flag is set from an explicit signal the panel sends when its text box
	// takes or releases focus, never inferred by asking Slate what has focus
	// this frame. An inferred gate would be a question with a different answer
	// depending on when it was asked; this one is a fact with a single owner,
	// which is what makes "movement is suppressed while typing" provable rather
	// than merely observed.

	/** Puts the caret in the chat box. Bound to Enter, as the classic client. */
	void FocusChatInput();

	/** Abandons the line and gives movement back. Bound to Escape. */
	void CloseChatInput();

	/** The panel's explicit signal. True means the player is typing. */
	void HandleTypingChanged(bool bTyping);

	TSharedPtr<SReal33DChatPanel> ChatPanel;

	/**
	 * Exactly the widget handed to AddViewportWidgetContent.
	 *
	 * Kept because removal matches on identity: passing the panel instead of
	 * the box it was wrapped in would silently remove nothing and leave the
	 * previous session's chat area on screen.
	 */
	TSharedPtr<SWidget> ChatRoot;

	/** The gate. Nothing else may write it. */
	bool bTypingActive = false;

	// The V08-only in-world identifier and note panel.
	TSharedPtr<SWidget> InspectorRoot;
	TSharedPtr<STextBlock> InspectorLabel;
	TSharedPtr<SEditableTextBox> InspectorNote;
	bool bInspectorEnabled = false;
	bool bInspectorSelection = false;
	uint16 InspectorTypeId = 0;
	FString InspectorName;
	FString InspectorStatus;
	FString InspectorPosition;
	FString InspectorNotesPath;
};
