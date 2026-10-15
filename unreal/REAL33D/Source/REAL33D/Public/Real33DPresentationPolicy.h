#pragma once

#include "CoreMinimal.h"

// Render policy only. These functions never describe server walkability.
namespace Real33D::Presentation
{
// Thin terrain/border meshes need distinct planes even at the first overlay.
// Use rendered thickness, not gameplay blocking flags (static context has none).
// Both live tiles and WideWorld call this with the transformed local bounds.
inline bool IsFlatSurface(const FBox& Bounds)
{
	return Bounds.IsValid && Bounds.GetSize().Z <= 10.0;
}

inline double SurfaceLayerLift(int32 ItemIndex)
{
	return 2.0 * FMath::Max(0, ItemIndex);
}

inline bool HideUpperFloors(int32 PlayerFloor, bool bCovered)
{
	return bCovered || PlayerFloor > 7;
}

// A room may span several floors: an empty immediate upper tile does not mean
// outdoors. The caller supplies only already-received live tile content.
inline bool HasCoverAbove(int32 PlayerFloor, TFunctionRef<bool(int32)> HasContent)
{
	for (int32 Floor = PlayerFloor - 1; Floor >= 0; --Floor)
		if (HasContent(Floor)) return true;
	return false;
}

// Slab intersection also handles a camera starting inside a mesh's bounds.
// Padding reserves camera/near-plane clearance without requiring asset collision.
inline bool SegmentEntry(const FVector& Start, const FVector& End,
	const FBox& Bounds, double Padding, double& Entry)
{
	if (!Bounds.IsValid) return false;
	const FBox Box = Bounds.ExpandBy(Padding);
	const FVector Delta = End - Start;
	double First = 0.0, Last = 1.0;
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		if (FMath::Abs(Delta[Axis]) < 1.e-8)
		{
			if (Start[Axis] < Box.Min[Axis] || Start[Axis] > Box.Max[Axis]) return false;
			continue;
		}
		double Near = (Box.Min[Axis] - Start[Axis]) / Delta[Axis];
		double Far = (Box.Max[Axis] - Start[Axis]) / Delta[Axis];
		if (Near > Far) Swap(Near, Far);
		First = FMath::Max(First, Near);
		Last = FMath::Min(Last, Far);
		if (First > Last) return false;
	}
	Entry = First;
	return true;
}

inline void LimitCamera(const FVector& Focus, const FVector& Eye,
	const FBox& Bounds, double& Distance)
{
	double Entry = 0.0;
	if (SegmentEntry(Focus, Eye, Bounds, 18.0, Entry))
		Distance = FMath::Min(Distance, FMath::Max(0.0, (Eye - Focus).Size() * Entry - 2.0));
}
}
