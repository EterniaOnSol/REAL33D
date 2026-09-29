#pragma once

#include "CoreMinimal.h"

// Render policy only. These functions never describe server walkability.
namespace Real33D::Presentation
{
inline bool HideUpperFloors(int32 PlayerFloor, bool bCovered)
{
	return bCovered || PlayerFloor > 7;
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

inline bool CutAway(const FVector& Focus, const FVector& Eye, const FBox& Bounds)
{
	if (!Bounds.IsValid || Bounds.GetSize().Z < 70.0) return false;
	const FVector Nearest = Bounds.GetClosestPointTo(Focus);
	if (FVector::DistSquaredXY(Nearest, Focus) > FMath::Square(300.0)) return false;
	double Entry = 0.0;
	return SegmentEntry(Focus, Eye, Bounds, 18.0, Entry);
}

inline void LimitCamera(const FVector& Focus, const FVector& Eye,
	const FBox& Bounds, double& Distance)
{
	double Entry = 0.0;
	if (SegmentEntry(Focus, Eye, Bounds, 18.0, Entry))
		Distance = FMath::Min(Distance, FMath::Max(0.0, (Eye - Focus).Size() * Entry - 2.0));
}
}
