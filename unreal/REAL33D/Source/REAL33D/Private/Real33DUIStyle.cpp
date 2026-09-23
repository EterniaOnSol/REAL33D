#include "Real33DUIStyle.h"

#include "Brushes/SlateDynamicImageBrush.h"
#include "Misc/Paths.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"

TSharedPtr<FSlateStyleSet> FReal33DUIStyle::Instance = nullptr;
TMap<uint16, TSharedPtr<FSlateDynamicImageBrush>> FReal33DUIStyle::ItemBrushes;

namespace
{
	// healthinfo.otui gives the 9-slice borders in pixels; FSlateBoxBrush wants
	// them as fractions of the image. Both bar images are 94 wide.
	constexpr float BarPixels = 94.0f;

	FMargin BorderMargin(float LeftPixels, float RightPixels)
	{
		return FMargin(LeftPixels / BarPixels, 0.0f, RightPixels / BarPixels, 0.0f);
	}

	/**
	 * A 9-slice margin from one pixel inset, as the `.otui` `image-border` key.
	 *
	 * That key is a single number applied to all four edges, and FSlateBoxBrush
	 * wants each edge as a fraction of the image it is cutting up, so the same
	 * inset divides by width on the horizontal edges and by height on the
	 * vertical ones. Passing the pixel value straight through would stretch the
	 * corners by the image's aspect ratio.
	 */
	FMargin UniformBorder(float InsetPixels, float Width, float Height)
	{
		return FMargin(InsetPixels / Width, InsetPixels / Height,
			InsetPixels / Width, InsetPixels / Height);
	}

	/**
	 * One icon, from `Resources/UI/Sliced/`.
	 *
	 * The 2D client keeps every state of a button in one PNG and picks a
	 * rectangle out of it with `image-clip`. The obvious transcription is a UV
	 * region on a single brush, and it does not work here: Slate packs
	 * file-backed brushes into a shared atlas, so a UV region set against the
	 * source image samples the atlas instead and every clipped icon came out as
	 * static from neighbouring textures. Declaring the brush at the sheet's own
	 * size rather than the cell's did not fix it either.
	 *
	 * So the cells are cut out on disk, by `scripts/client/slice_ui_cells.ps1`,
	 * and each is its own PNG at its native size. That is exactly the shape the
	 * brushes which always rendered correctly already had -- the health bars,
	 * the equipment placeholders -- and it needs nothing from the atlas beyond
	 * what those need. The cost is that re-cutting is a script run rather than
	 * a file copy when the source art changes; the script records every
	 * rectangle, so that remains one command.
	 */
	FSlateImageBrush* Sliced(FSlateStyleSet& Style, const TCHAR* Name,
		float Width, float Height)
	{
		return new FSlateImageBrush(
			Style.RootToContentDir(*FString::Printf(TEXT("Sliced/%s"), Name), TEXT(".png")),
			FVector2D(Width, Height));
	}
}

void FReal33DUIStyle::Initialize()
{
	if (Instance.IsValid())
	{
		return;
	}
	Instance = Create();
	FSlateStyleRegistry::RegisterSlateStyle(*Instance);
}

void FReal33DUIStyle::Shutdown()
{
	if (!Instance.IsValid())
	{
		return;
	}
	FSlateStyleRegistry::UnRegisterSlateStyle(*Instance);
	Instance.Reset();
	// The item pictures point into the style's content root, so they go with it.
	ItemBrushes.Reset();
}

const FSlateBrush* FReal33DUIStyle::ItemBrush(uint16 TypeId)
{
	if (TypeId == 0 || !Instance.IsValid())
	{
		return nullptr;
	}
	if (const TSharedPtr<FSlateDynamicImageBrush>* Found = ItemBrushes.Find(TypeId))
	{
		return Found->Get();
	}

	const FString Path = Instance->RootToContentDir(
		*FString::Printf(TEXT("Items/%u"), TypeId), TEXT(".png"));
	if (!FPaths::FileExists(Path))
	{
		// Remembered as absent so a missing picture is one file check per id
		// per session rather than one per frame.
		ItemBrushes.Add(TypeId, nullptr);
		return nullptr;
	}

	// Every extracted picture is 32x32, which is why this size is a constant
	// and not read from the file: see extract_item_sprites.py.
	TSharedPtr<FSlateDynamicImageBrush> Brush = MakeShareable(
		new FSlateDynamicImageBrush(FName(*Path), FVector2D(SlotIconSize, SlotIconSize)));
	ItemBrushes.Add(TypeId, Brush);
	return Brush.Get();
}

const ISlateStyle& FReal33DUIStyle::Get()
{
	check(Instance.IsValid());
	return *Instance;
}

FName FReal33DUIStyle::GetStyleSetName()
{
	static const FName Name(TEXT("Real33DUIStyle"));
	return Name;
}

TSharedRef<FSlateStyleSet> FReal33DUIStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShared<FSlateStyleSet>(GetStyleSetName());
	Style->SetContentRoot(FPaths::ProjectDir() / TEXT("Resources/UI"));

	const FVector2D SymbolSize(SymbolWidth, SymbolHeight);
	const FVector2D BarSize(BarWidth, BarHeight);

	// ------------------------------------------------------------ health/mana

	// The two icons. Plain image brushes: they are never stretched.
	Style->Set("Real33D.HealthMana.HitpointsSymbol", new FSlateImageBrush(
		Style->RootToContentDir(TEXT("HealthMana/hitpoints_symbol"), TEXT(".png")),
		SymbolSize));
	Style->Set("Real33D.HealthMana.ManaSymbol", new FSlateImageBrush(
		Style->RootToContentDir(TEXT("HealthMana/mana_symbol"), TEXT(".png")),
		SymbolSize));

	// The empty trough both bars sit in. 9-slice 6/6, per healthinfo.otui.
	Style->Set("Real33D.HealthMana.BarBorder", new FSlateBoxBrush(
		Style->RootToContentDir(TEXT("HealthMana/hitpoints_manapoints_bar_border"), TEXT(".png")),
		BarSize, BorderMargin(6.0f, 6.0f)));

	// The fills. 9-slice 5/7: these are stretched to the current value, so the
	// margins are what keeps the rounded caps from smearing.
	Style->Set("Real33D.HealthMana.HitpointsFill", new FSlateBoxBrush(
		Style->RootToContentDir(TEXT("HealthMana/hitpoints_bar_filled"), TEXT(".png")),
		BarSize, BorderMargin(5.0f, 7.0f)));
	Style->Set("Real33D.HealthMana.ManaFill", new FSlateBoxBrush(
		Style->RootToContentDir(TEXT("HealthMana/mana_bar_filled"), TEXT(".png")),
		BarSize, BorderMargin(5.0f, 7.0f)));

	// #c0c0c0ff, the label colour healthinfo.otui uses for both readouts.
	Style->Set("Real33D.HealthMana.TextColor",
		FSlateColor(FLinearColor(FColor(0xC0, 0xC0, 0xC0, 0xFF))));

	// The bars as progress-bar styles rather than hand-placed boxes.
	//
	// healthinfo.otui anchors `current` to `total.left` and lets the trough
	// stretch between the icon and the numbers, so the bar is whatever width
	// the column leaves it. Transcribing that as a fixed 94px box was wrong:
	// at the 176px side-panel width the row came to 184 and the numbers were
	// clipped off the edge. A progress bar takes its width from the layout and
	// scales the 9-sliced fill inside it, which is what the .otui describes.
	{
		const auto Bar = [](const FSlateBrush* Background, const FSlateBrush* Fill)
		{
			FProgressBarStyle Out;
			Out.SetBackgroundImage(*Background);
			Out.SetFillImage(*Fill);
			Out.SetMarqueeImage(*Fill);
			return Out;
		};
		Style->Set("Real33D.Bar.Hitpoints",
			Bar(Style->GetBrush("Real33D.HealthMana.BarBorder"),
				Style->GetBrush("Real33D.HealthMana.HitpointsFill")));
		Style->Set("Real33D.Bar.Mana",
			Bar(Style->GetBrush("Real33D.HealthMana.BarBorder"),
				Style->GetBrush("Real33D.HealthMana.ManaFill")));

		// The thin skill and battle bars, which the 2D draws as flat colour
		// rather than art: a dark trough with a tinted fill the widget sets.
		FSlateColorBrush* Trough = new FSlateColorBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
		FSlateColorBrush* Plain = new FSlateColorBrush(FLinearColor::White);
		FProgressBarStyle Thin;
		Thin.SetBackgroundImage(*Trough);
		Thin.SetFillImage(*Plain);
		Thin.SetMarqueeImage(*Plain);
		Style->Set("Real33D.Bar.Thin", Thin);
	}

	// ---------------------------------------------------------- window chrome
	//
	// 30-miniwindow.otui. The body and header are 256px-wide strips stretched
	// to whatever width a panel ends up at, so both are box brushes with the
	// 4px border that file declares.

	Style->Set("Real33D.Chrome.WindowBody", new FSlateBoxBrush(
		Style->RootToContentDir(TEXT("Chrome/miniwindow_body"), TEXT(".png")),
		FVector2D(256.0f, 239.0f), UniformBorder(4.0f, 256.0f, 239.0f)));
	Style->Set("Real33D.Chrome.WindowHeader", new FSlateBoxBrush(
		Style->RootToContentDir(TEXT("Chrome/miniwindow_header"), TEXT(".png")),
		FVector2D(256.0f, 17.0f), UniformBorder(4.0f, 256.0f, 17.0f)));

	// GameSidePanel, gameinterface.otui: image-border 4 on a 100x100 tile.
	Style->Set("Real33D.Chrome.SidePanel", new FSlateBoxBrush(
		Style->RootToContentDir(TEXT("Chrome/2pixel_up_frame_borderimage"), TEXT(".png")),
		FVector2D(100.0f, 100.0f), UniformBorder(4.0f, 100.0f, 100.0f)));

	// GameMapPanel framing, and the sunken frame the minimap sits in.
	//
	// A BORDER brush, not a box one. A box brush stretches the image's middle
	// region across the whole widget, which for the map panel means painting
	// over the 3D scene the frame is supposed to be framing -- the centre has to
	// be a hole, not a fill. FSlateBorderBrush draws the eight edge and corner
	// regions and leaves the middle untouched, which is what a frame is.
	Style->Set("Real33D.Chrome.MapPanel", new FSlateBorderBrush(
		Style->RootToContentDir(TEXT("Chrome/panel_map"), TEXT(".png")),
		UniformBorder(4.0f, 96.0f, 96.0f)));
	Style->Set("Real33D.Chrome.SunkenFrame", new FSlateBoxBrush(
		Style->RootToContentDir(TEXT("Chrome/1pixel_down_frame"), TEXT(".png")),
		FVector2D(96.0f, 96.0f), UniformBorder(4.0f, 96.0f, 96.0f)));
	// The console buffer's own frame, console.otui: image-border 4.
	Style->Set("Real33D.Chrome.ConsoleFrame", new FSlateBoxBrush(
		Style->RootToContentDir(TEXT("Chrome/3pixel_frame_borderimage"), TEXT(".png")),
		FVector2D(102.0f, 102.0f), UniformBorder(4.0f, 102.0f, 102.0f)));

	// The two repeating backgrounds: the bottom panel and the stats bars.
	Style->Set("Real33D.Chrome.BackgroundDark", new FSlateBoxBrush(
		Style->RootToContentDir(TEXT("Chrome/background_dark"), TEXT(".png")),
		FVector2D(96.0f, 96.0f), FMargin(0.25f)));
	Style->Set("Real33D.Chrome.Background", new FSlateBoxBrush(
		Style->RootToContentDir(TEXT("Chrome/background"), TEXT(".png")),
		FVector2D(96.0f, 96.0f), FMargin(0.25f)));

	// The dark inset both the status-icon strip and the soul/cap boxes use.
	Style->Set("Real33D.Chrome.ContainerSlot", new FSlateBoxBrush(
		Style->RootToContentDir(TEXT("Chrome/containerslot"), TEXT(".png")),
		FVector2D(32.0f, 33.0f), UniformBorder(3.0f, 32.0f, 33.0f)));

	// An empty item slot, 10-items.otui.
	Style->Set("Real33D.Chrome.ItemSlot", new FSlateBoxBrush(
		Style->RootToContentDir(TEXT("Chrome/item"), TEXT(".png")),
		FVector2D(SlotSize, SlotSize), UniformBorder(1.0f, SlotSize, SlotSize)));

	// The dither the 2D lays over anything it has greyed out. Used here to mark
	// a control inert, which is exactly what it means in the 2D too.
	Style->Set("Real33D.Chrome.Dither", new FSlateBoxBrush(
		Style->RootToContentDir(TEXT("Chrome/ditherpattern"), TEXT(".png")),
		FVector2D(32.0f, 32.0f), FMargin(0.25f)));
	Style->Set("Real33D.Chrome.MiniBorder", new FSlateImageBrush(
		Style->RootToContentDir(TEXT("Chrome/miniborder"), TEXT(".png")),
		FVector2D(14.0f, 14.0f)));

	// The header buttons. 14x14 cells the 2D draws at 12x12, which is its
	// `size` key against `image-size: 14 14`.
	Style->Set("Real33D.Chrome.Minimize", Sliced(*Style, TEXT("chrome_minimize"), 14, 14));
	Style->Set("Real33D.Chrome.Restore", Sliced(*Style, TEXT("chrome_restore"), 14, 14));
	Style->Set("Real33D.Chrome.Close", Sliced(*Style, TEXT("chrome_close"), 14, 14));
	Style->Set("Real33D.Chrome.ClosePressed", Sliced(*Style, TEXT("chrome_close_pressed"), 14, 14));
	Style->Set("Real33D.Chrome.ContainerUp", Sliced(*Style, TEXT("chrome_container_up"), 14, 14));
	Style->Set("Real33D.Chrome.Filter", Sliced(*Style, TEXT("chrome_filter"), 14, 14));
	Style->Set("Real33D.Chrome.LockOpen", Sliced(*Style, TEXT("chrome_lock_open"), 14, 14));
	Style->Set("Real33D.Chrome.LockShut", Sliced(*Style, TEXT("chrome_lock_shut"), 14, 14));
	Style->Set("Real33D.Chrome.ContextMenu", Sliced(*Style, TEXT("chrome_context_menu"), 14, 14));

	// Console channel tabs, console.otui: 96x18, checked on top, idle below.
	Style->Set("Real33D.Chrome.TabSelected", Sliced(*Style, TEXT("tab_selected"), 96, 18));
	Style->Set("Real33D.Chrome.TabIdle", Sliced(*Style, TEXT("tab_idle"), 96, 18));

	// The side-panel grow/shrink arrows, gameinterface.otui: 9x27 cells.
	Style->Set("Real33D.Chrome.PanelRightMore", Sliced(*Style, TEXT("panel_right_more"), 9, 27));
	Style->Set("Real33D.Chrome.PanelRightLess", Sliced(*Style, TEXT("panel_right_less"), 9, 27));
	Style->Set("Real33D.Chrome.PanelLeftMore", Sliced(*Style, TEXT("panel_left_more"), 9, 27));
	Style->Set("Real33D.Chrome.PanelLeftLess", Sliced(*Style, TEXT("panel_left_less"), 9, 27));

	// Container paging, container.otui.
	Style->Set("Real33D.Chrome.PageLeft", new FSlateImageBrush(
		Style->RootToContentDir(TEXT("Chrome/container-browse-left-up"), TEXT(".png")),
		FVector2D(18.0f, 18.0f)));
	Style->Set("Real33D.Chrome.PageRight", new FSlateImageBrush(
		Style->RootToContentDir(TEXT("Chrome/container-browse-right-up"), TEXT(".png")),
		FVector2D(18.0f, 18.0f)));

	// ------------------------------------------------------ inventory the 2D's
	//
	// inventory.otui. The equipment slots are containerslot with a 3px border
	// and a 32x32 placeholder centred in each.

	Style->Set("Real33D.Inventory.Slot", new FSlateBoxBrush(
		Style->RootToContentDir(TEXT("Inventory/containerslot"), TEXT(".png")),
		FVector2D(34.0f, 44.0f), UniformBorder(3.0f, 34.0f, 44.0f)));

	static const TCHAR* const SlotArt[] = {
		TEXT("head"), TEXT("neck"), TEXT("back"), TEXT("torso"), TEXT("right_hand"),
		TEXT("left_hand"), TEXT("legs"), TEXT("feet"), TEXT("finger"), TEXT("hip") };
	for (const TCHAR* const Each : SlotArt)
	{
		Style->Set(FName(*FString::Printf(TEXT("Real33D.Inventory.Slot.%s"), Each)),
			new FSlateImageBrush(
				Style->RootToContentDir(
					*FString::Printf(TEXT("Inventory/inventory_%s"), Each), TEXT(".png")),
				FVector2D(SlotIconSize, SlotIconSize)));
	}

	// The combat and posture toggles: 20x20 cells, idle and engaged.
	Style->Set("Real33D.Inventory.Stand", Sliced(*Style, TEXT("inv_stand"), 20, 20));
	Style->Set("Real33D.Inventory.StandOn", Sliced(*Style, TEXT("inv_stand_on"), 20, 20));
	Style->Set("Real33D.Inventory.Follow", Sliced(*Style, TEXT("inv_follow"), 20, 20));
	Style->Set("Real33D.Inventory.FollowOn", Sliced(*Style, TEXT("inv_follow_on"), 20, 20));
	Style->Set("Real33D.Inventory.Attack", Sliced(*Style, TEXT("inv_attack"), 20, 20));
	Style->Set("Real33D.Inventory.AttackOn", Sliced(*Style, TEXT("inv_attack_on"), 20, 20));
	Style->Set("Real33D.Inventory.Defend", Sliced(*Style, TEXT("inv_defend"), 20, 20));
	Style->Set("Real33D.Inventory.DefendOn", Sliced(*Style, TEXT("inv_defend_on"), 20, 20));
	Style->Set("Real33D.Inventory.Balanced", Sliced(*Style, TEXT("inv_balanced"), 20, 20));
	Style->Set("Real33D.Inventory.BalancedOn", Sliced(*Style, TEXT("inv_balanced_on"), 20, 20));

	Style->Set("Real33D.Inventory.Shrink", Sliced(*Style, TEXT("inv_shrink"), 12, 12));
	Style->Set("Real33D.Inventory.Grow", Sliced(*Style, TEXT("inv_grow"), 12, 12));
	Style->Set("Real33D.Inventory.Purse", Sliced(*Style, TEXT("inv_purse"), 34, 12));

	// ------------------------------------------------------------- conditions
	//
	// player-state-flags.png is a 369x9 strip of 9x9 icons. The eight cells
	// below are the eight flags SV_CMD_PLAYER_STATE can actually raise on
	// Fusion32, at the clip indices gamelib/player.lua gives them. Nothing
	// later in the strip belongs to 7.72 and none of it is registered.
	{
		static const TCHAR* const Keys[] = {
			TEXT("Poisoned"), TEXT("Burning"), TEXT("Electrified"), TEXT("Drunk"),
			TEXT("ManaShield"), TEXT("Slowed"), TEXT("Hasted"), TEXT("LogoutBlocked") };
		static const TCHAR* const Files[] = {
			TEXT("poisoned"), TEXT("burning"), TEXT("electrified"), TEXT("drunk"),
			TEXT("manashield"), TEXT("slowed"), TEXT("hasted"), TEXT("logoutblocked") };
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Keys); ++Index)
		{
			Style->Set(FName(*FString::Printf(TEXT("Real33D.Condition.%s"), Keys[Index])),
				Sliced(*Style, *FString::Printf(TEXT("condition_%s"), Files[Index]),
					ConditionIconSize, ConditionIconSize));
		}
	}

	// ----------------------------------------------------------- action bars
	//
	// game_actionbar. The column background is a 37x19 tile with a 1px border;
	// the slot is a flat 34x34.

	Style->Set("Real33D.ActionBar.Background", new FSlateBoxBrush(
		Style->RootToContentDir(TEXT("ActionBar/actionbar_background-light"), TEXT(".png")),
		FVector2D(37.0f, 19.0f), UniformBorder(1.0f, 37.0f, 19.0f)));
	Style->Set("Real33D.ActionBar.BackgroundDark", new FSlateBoxBrush(
		Style->RootToContentDir(TEXT("ActionBar/background-dark"), TEXT(".png")),
		FVector2D(96.0f, 96.0f), FMargin(0.25f)));
	Style->Set("Real33D.ActionBar.Slot", new FSlateImageBrush(
		Style->RootToContentDir(TEXT("ActionBar/actionbarslot"), TEXT(".png")),
		FVector2D(ActionSlotSize, ActionSlotSize)));
	Style->Set("Real33D.ActionBar.Splitter", new FSlateBoxBrush(
		Style->RootToContentDir(TEXT("ActionBar/splitterActBottom"), TEXT(".png")),
		FVector2D(1744.0f, 6.0f), FMargin(0.02f, 0.0f)));
	Style->Set("Real33D.ActionBar.Locked", new FSlateImageBrush(
		Style->RootToContentDir(TEXT("ActionBar/locked"), TEXT(".png")),
		FVector2D(8.0f, 12.0f)));

	// The four paging arrows, each cut from the disabled row of its own 17x51
	// column, because nothing here can page anything yet.
	Style->Set("Real33D.ActionBar.Previous", Sliced(*Style, TEXT("arrow_previous"), 17, 17));
	Style->Set("Real33D.ActionBar.Next", Sliced(*Style, TEXT("arrow_next"), 17, 17));
	Style->Set("Real33D.ActionBar.First", Sliced(*Style, TEXT("arrow_first"), 17, 17));
	Style->Set("Real33D.ActionBar.Last", Sliced(*Style, TEXT("arrow_last"), 17, 17));

	// ------------------------------------------------------------ battle list

	Style->Set("Real33D.Battle.Icon", new FSlateImageBrush(
		Style->RootToContentDir(TEXT("Battle/icon-battlelist"), TEXT(".png")),
		FVector2D(12.0f, 12.0f)));
	Style->Set("Real33D.Battle.Players", new FSlateImageBrush(
		Style->RootToContentDir(TEXT("Battle/icon-battlelist-players"), TEXT(".png")),
		FVector2D(15.0f, 16.0f)));
	Style->Set("Real33D.Battle.Monsters", new FSlateImageBrush(
		Style->RootToContentDir(TEXT("Battle/icon-battlelist-monster"), TEXT(".png")),
		FVector2D(15.0f, 14.0f)));
	Style->Set("Real33D.Battle.NPCs", new FSlateImageBrush(
		Style->RootToContentDir(TEXT("Battle/icon-battlelist-npc"), TEXT(".png")),
		FVector2D(16.0f, 14.0f)));
	Style->Set("Real33D.Battle.Skulls", new FSlateImageBrush(
		Style->RootToContentDir(TEXT("Battle/icon-battlelist-skull"), TEXT(".png")),
		FVector2D(16.0f, 16.0f)));
	Style->Set("Real33D.Battle.Party", new FSlateImageBrush(
		Style->RootToContentDir(TEXT("Battle/icon-battlelist-party"), TEXT(".png")),
		FVector2D(15.0f, 16.0f)));
	// battle.otui draws every filter on `button_empty`: idle, then engaged.
	Style->Set("Real33D.Battle.FilterIdle", Sliced(*Style, TEXT("battle_filter_idle"), 20, 20));
	Style->Set("Real33D.Battle.FilterOn", Sliced(*Style, TEXT("battle_filter_on"), 20, 20));

	// -------------------------------------------------------------- minimap

	Style->Set("Real33D.Automap.Rose", Sliced(*Style, TEXT("automap_rose"), 43, 43));
	Style->Set("Real33D.Automap.Layers", Sliced(*Style, TEXT("automap_layers"), 14, 67));
	Style->Set("Real33D.Automap.LayerMark", new FSlateImageBrush(
		Style->RootToContentDir(TEXT("Automap/automap_indicator_slider_left"), TEXT(".png")),
		FVector2D(6.0f, 7.0f)));
	Style->Set("Real33D.Automap.FullMap", Sliced(*Style, TEXT("automap_fullmap"), 20, 20));
	Style->Set("Real33D.Automap.ZoomOut", Sliced(*Style, TEXT("automap_zoomout"), 20, 20));
	Style->Set("Real33D.Automap.ZoomIn", Sliced(*Style, TEXT("automap_zoomin"), 20, 20));

	// -------------------------------------------------- main panel controls
	//
	// Every control button is a 40x20 sheet of two 20x20 cells: idle, engaged.

	{
		static const TCHAR* const Buttons[] = {
			TEXT("skills"), TEXT("battlelist"), TEXT("vip"),
			TEXT("control"), TEXT("options"), TEXT("logout") };
		static const TCHAR* const Keys[] = {
			TEXT("Skills"), TEXT("Battle"), TEXT("Vip"),
			TEXT("Control"), TEXT("Options"), TEXT("Logout") };
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Buttons); ++Index)
		{
			Style->Set(FName(*FString::Printf(TEXT("Real33D.Control.%s"), Keys[Index])),
				Sliced(*Style, *FString::Printf(TEXT("control_%s"), Buttons[Index]),
					ControlButtonSize, ControlButtonSize));
			Style->Set(FName(*FString::Printf(TEXT("Real33D.Control.%sOn"), Keys[Index])),
				Sliced(*Style, *FString::Printf(TEXT("control_%s_on"), Buttons[Index]),
					ControlButtonSize, ControlButtonSize));
		}
	}
	// The skills miniwindow's own 12x12 title icon.
	Style->Set("Real33D.Control.SkillsIcon", new FSlateImageBrush(
		Style->RootToContentDir(TEXT("Options/skills"), TEXT(".png")),
		FVector2D(12.0f, 12.0f)));

	// ------------------------------------------------------------- colours
	//
	// Taken from the `.otui` that owns each surface, so nothing here is a new
	// choice: #c0c0c0 is the readout grey, #9d9d9d the miniwindow title, and
	// #dfdfdf88 what the 2D dims a disabled control to.

	Style->Set("Real33D.Text.Readout",
		FSlateColor(FLinearColor(FColor(0xC0, 0xC0, 0xC0, 0xFF))));
	Style->Set("Real33D.Text.Title",
		FSlateColor(FLinearColor(FColor(0x9D, 0x9D, 0x9D, 0xFF))));
	Style->Set("Real33D.Text.Disabled",
		FSlateColor(FLinearColor(FColor(0xDF, 0xDF, 0xDF, 0x88))));

	return Style;
}
