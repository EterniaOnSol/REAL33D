#include "Real33DPanelRegistry.h"

namespace Real33D
{
namespace PanelIds
{
	const FName Minimap(TEXT("Minimap"));
	const FName Health(TEXT("Health"));
	const FName Inventory(TEXT("Inventory"));
	// "Skills" and "Battle" are the two the control strip already sends; see
	// SReal33DControlPanel. They must keep these exact spellings.
	const FName Skills(TEXT("Skills"));
	const FName Battle(TEXT("Battle"));
	const FName Containers(TEXT("Containers"));
	const FName Channels(TEXT("Channels"));
	const FName TextEditor(TEXT("TextEditor"));
	const FName Trade(TEXT("Trade"));
	const FName Outfit(TEXT("Outfit"));
	const FName MoveAmount(TEXT("MoveAmount"));
}
}

bool FReal33DPanelRegistry::Register(FName Id, const TSharedRef<SWidget>& Window)
{
	if (Id.IsNone() || IsRegistered(Id))
	{
		return false;
	}
	Entries.Add(FEntry{Id, Window});
	return true;
}

bool FReal33DPanelRegistry::Unregister(FName Id)
{
	return Entries.RemoveAll([Id](const FEntry& Entry) { return Entry.Id == Id; }) > 0;
}

void FReal33DPanelRegistry::Reset()
{
	Entries.Reset();
}

void FReal33DPanelRegistry::GetIds(TArray<FName>& OutIds) const
{
	OutIds.Reset(Entries.Num());
	for (const FEntry& Entry : Entries)
	{
		OutIds.Add(Entry.Id);
	}
}

TSharedPtr<SWidget> FReal33DPanelRegistry::Find(FName Id) const
{
	for (const FEntry& Entry : Entries)
	{
		if (Entry.Id == Id)
		{
			return Entry.Window;
		}
	}
	return nullptr;
}

bool FReal33DPanelRegistry::IsOpen(FName Id) const
{
	const TSharedPtr<SWidget> Window = Find(Id);
	// Read from the widget, never from a cached flag: see the header.
	return Window.IsValid() && Window->GetVisibility() != EVisibility::Collapsed;
}

bool FReal33DPanelRegistry::SetOpen(FName Id, bool bOpen)
{
	const TSharedPtr<SWidget> Window = Find(Id);
	if (!Window.IsValid())
	{
		return false;
	}
	Window->SetVisibility(bOpen ? EVisibility::Visible : EVisibility::Collapsed);
	return true;
}

bool FReal33DPanelRegistry::Toggle(FName Id)
{
	const TSharedPtr<SWidget> Window = Find(Id);
	if (!Window.IsValid())
	{
		return false;
	}
	// Same flip the hand-written toggle performed, so a panel that was built
	// collapsed opens on its first toggle exactly as it used to.
	return SetOpen(Id, Window->GetVisibility() == EVisibility::Collapsed);
}
