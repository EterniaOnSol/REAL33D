#pragma once

#include "CoreMinimal.h"
#include "Input/DragAndDrop.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SVerticalBox;

/**
 * Where a slot is, in the terms Fusion32 uses to address one.
 *
 * A body slot or a slot inside an open container. Not a map field: dropping an
 * object onto the ground means naming the field under the cursor, which is the
 * 3D scene's business and not this panel's.
 */
struct FReal33DSlotRef
{
	enum class EKind : uint8 { None, Inventory, Container };
	EKind Kind = EKind::None;
	/** Which open container, 0-based. Meaningless unless Kind is Container. */
	uint8 Container = 0;
	/** The body slot number, or the index within the container. */
	uint8 Slot = 0;
	/** What is being carried, so the request can name the object. */
	uint16 TypeId = 0;
	/** Stack size for a cumulative object; 1 for anything else. */
	uint8 Count = 1;

	bool IsValid() const { return Kind != EKind::None; }
};

/** Fired when an object is dropped on a slot. Both ends are server addresses. */
DECLARE_DELEGATE_TwoParams(FReal33DOnItemDropped, FReal33DSlotRef, FReal33DSlotRef);

/**
 * Fired when a slot is used.
 *
 * `bWithTarget` is the difference between the two 7.72 shapes of use: false
 * means CL_CMD_USE_OBJECT, which acts on the object where it stands and is
 * what opens a container; true means the player wants to use it on something
 * else, which puts the client into targeting until the next click names a
 * creature, an object or a field.
 */
DECLARE_DELEGATE_TwoParams(FReal33DOnSlotUsed, FReal33DSlotRef, bool);

/**
 * Offers a left-click on a slot to whoever is waiting for a target.
 *
 * Returns true when it was taken, which is how a click that completes a
 * use-with is stopped from also starting a drag of the object it landed on.
 */
DECLARE_DELEGATE_RetVal_OneParam(bool, FReal33DOnSlotPicked, FReal33DSlotRef);

/**
 * The object being dragged between slots.
 *
 * Carries only what CL_CMD_MOVE_OBJECT needs to name it: where it is, what it
 * is, and how many. Nothing is moved while the drag is in flight -- the panel
 * still shows the object where the server last said it was, because that is
 * still where it is.
 */
class FReal33DItemDrag : public FDragDropOperation
{
public:
	DRAG_DROP_OPERATOR_TYPE(FReal33DItemDrag, FDragDropOperation)

	explicit FReal33DItemDrag(const FReal33DSlotRef& InFrom) : From(InFrom) {}

	const FReal33DSlotRef& Origin() const { return From; }

private:
	FReal33DSlotRef From;
};

/**
 * The classic REAL33D2D window frame, transcribed from `30-miniwindow.otui`.
 *
 * Every panel in the side columns is one of these: a 9-sliced body, a 15px
 * header carrying a 12x12 icon, a title and the header buttons, and the
 * contents below. The geometry is that file's -- the 4px body border, the 2px
 * bevel under the header, the 3px inset the contents sit in -- so a panel
 * written against this frame lands where the 2D client puts it.
 *
 * The header buttons are drawn because the 2D draws them. They do nothing: this
 * client has no window manager to minimise or close into, and a button that
 * looked live and then did nothing would be worse than one that plainly is not.
 * Minimise is the exception and is wired, because collapsing a panel needs
 * nothing from the server.
 */
class SReal33DMiniWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DMiniWindow)
		: _Title()
		, _IconBrush(NAME_None)
		, _ContentHeight(150.0f)
		, _Inert(false)
	{}
		/** Shown in the header, in the 2D's own title colour. */
		SLATE_ARGUMENT(FText, Title)
		/** A 12x12 brush name from FReal33DUIStyle, or NAME_None for no icon. */
		SLATE_ARGUMENT(FName, IconBrush)
		/** Height of the contents area below the header, in pixels. */
		SLATE_ARGUMENT(float, ContentHeight)
		/**
		 * True when the panel shows a surface this client cannot fill yet.
		 *
		 * Draws the 2D's own dither over the contents and greys the title, which
		 * is exactly what that client does to a disabled widget. It is a
		 * statement that the data does not exist, not that the panel is broken.
		 */
		SLATE_ARGUMENT(bool, Inert)
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply ToggleCollapsed();

	/** One 12x12 header button. Inert unless it is the minimise one. */
	TSharedRef<SWidget> MakeHeaderButton(const FName& Brush, bool bWired);

	TSharedPtr<SWidget> Body;
	float ContentHeight = 150.0f;
	bool bCollapsed = false;
};

/**
 * One 34x34 slot, as `10-items.otui` draws an item and `inventory.otui` an
 * equipment square.
 *
 * When the slot holds something, that something is whatever Fusion32 last said
 * is there: a type id, and a stack count when the object's type is cumulative.
 * When it holds nothing the slot shows the 32x32 body-part placeholder the 2D
 * shows for an empty equipment square, or bare art for a container cell.
 *
 * The item is drawn as its type id rather than its sprite. This client has no
 * 7.72 sprite sheet -- the 3D presentation resolves meshes by type id through
 * the asset registry, which covers the objects that have been modelled and not
 * the rest -- so an id is what can be shown truthfully. It is the real id off
 * the wire, not a placeholder, and the tooltip carries the count.
 */
class SReal33DSlot : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DSlot)
		: _SlotBrush("Real33D.Chrome.ItemSlot")
		, _PlaceholderBrush(NAME_None)
		, _Tooltip()
		, _TypeId(0)
		, _Count(0)
	{}
		SLATE_ARGUMENT(FName, SlotBrush)
		/** The 32x32 art naming which body part this slot is, if it is one. */
		SLATE_ARGUMENT(FName, PlaceholderBrush)
		SLATE_ARGUMENT(FText, Tooltip)
		/** Non-zero when the server says an object is here. */
		SLATE_ARGUMENT(uint16, TypeId)
		/** Stack size, drawn only when the object's type is cumulative. */
		SLATE_ARGUMENT(uint8, Count)
		/** Where this slot is, in the terms a move request needs. */
		SLATE_ARGUMENT(FReal33DSlotRef, Location)
		/** Fired when something is dropped here. */
		SLATE_EVENT(FReal33DOnItemDropped, OnItemDropped)
		/** Fired on a right-click: use, or shift-right-click: use with. */
		SLATE_EVENT(FReal33DOnSlotUsed, OnSlotUsed)
		/** Offered a left-click, so a pending use-with can take it as its target. */
		SLATE_EVENT(FReal33DOnSlotPicked, OnSlotPicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// ------------------------------------------------------ drag and drop
	//
	// Dragging an object from one slot to another is a move, and a move is a
	// request: the object stays drawn where it is until Fusion32 says
	// otherwise. Nothing here touches the panel's contents. This is not item
	// *use* -- that is mouse-driven with a crosshair and belongs to a later
	// milestone; this only asks the server to relocate an object it owns.

	virtual FReply OnMouseButtonDown(const FGeometry& Geometry,
		const FPointerEvent& Event) override;
	virtual FReply OnDragDetected(const FGeometry& Geometry,
		const FPointerEvent& Event) override;
	virtual FReply OnDragOver(const FGeometry& Geometry,
		const FDragDropEvent& Event) override;
	virtual FReply OnDrop(const FGeometry& Geometry,
		const FDragDropEvent& Event) override;

private:
	FReal33DSlotRef Location;
	FReal33DOnItemDropped OnItemDropped;
	FReal33DOnSlotUsed OnSlotUsed;
	FReal33DOnSlotPicked OnSlotPicked;
};
