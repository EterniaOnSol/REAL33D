#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class UReal33DBridge;

DECLARE_DELEGATE_OneParam(FReal33DOnPanelToggled, FName);

/**
 * The control button strip, from `game_mainpanel/mainoptionspanel.otui`.
 *
 * The 2D builds this strip by letting each module register its own 20x20
 * button; the ones registered by the modules actually in that client's load
 * chain are Skills, Battle List, VIP, Control Buttons, Options and Exit. Those
 * six are what this strip carries, in that order, on the 20px grid with 2px
 * gutters that file lays out.
 *
 * Skills and Battle toggle their panels, which is a local decision and live.
 *
 * Exit is live and is the real thing: Fusion32's logout is a single client
 * command, ClientCore already builds it in `BuildLogoutCommand`, and the bridge
 * already sends it on the way down. Pressing Exit therefore logs out properly
 * rather than dropping the socket, which is the difference between leaving the
 * game and appearing to freeze inside it.
 *
 * VIP, Control Buttons and Options are inert. The first needs a buddy list this
 * client decodes only for length; the other two are windows this milestone does
 * not build.
 */
class SReal33DControlPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DControlPanel) {}
		SLATE_ARGUMENT(UReal33DBridge*, Bridge)
		/** Fired with the panel's name when its button is pressed. */
		SLATE_EVENT(FReal33DOnPanelToggled, OnPanelToggled)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** mainoptionspanel.otui gives the strip a 28px body. */
	static constexpr float PanelHeight = 28.0f;

private:
	FReply HandleToggle(FName Panel);
	FReply HandleLogout();

	/** One 20x20 control button. `Panel` empty means the button is inert. */
	TSharedRef<SWidget> MakeButton(const FName& Brush, const FText& Tooltip,
		FName Panel, bool bLogout);

	TWeakObjectPtr<UReal33DBridge> Bridge;
	FReal33DOnPanelToggled OnPanelToggled;
};
