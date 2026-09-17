# Art Direction

## There is no fidelity percentage

There is no 50/50 rule, and no other ratio. Any number of that kind would be
arbitrary and would turn a judgement into arithmetic.

What the original data provides is **identity and context**: what a thing is,
where it sits, how large it is on the grid, what it does when used, and what the
world around it looks like. That is binding. A tree is a tree, a Rookgaard
bridge is that bridge, and a bear is a bear.

What the original data does not provide is a target for how closely the 3D asset
should resemble the 32-pixel sprite. Sprites are a 2D top-down abstraction; some
read literally in 3D, some do not. Deciding that is the artist's job and the
director's call, asset by asset.

## Who decides

- The **artist** proposes. One or more mockups per asset, each a distinct
  version.
- The **project director** decides `APPROVED` or `REJECTED`. That decision is
  not delegated to a rule, a metric or this document.

Nothing advances past `REVIEW` without an explicit decision recorded in the
tracker.

## Rejected work is kept

Rejected proposals and superseded versions go to `visual/rejected/` and stay
there. They are not deleted and not overwritten.

Two reasons. A rejected direction is often revived later for a different asset
or a different region. And a decision record without the thing it rejected is
not a record.

Each asset keeps its own directory under `mockups/`, `approved/` and
`rejected/`, named after the tracker `ID`, so the history of a decision is
readable without consulting anyone.

## What the inventory contributes, and what it does not

The manifests say what exists, what it does mechanically, where it occurs and
which ids are likely to share an appearance. They say nothing about style.

In particular the `art_class` column is a keyword match on the object's own
name. It is a starting filter for an artist browsing 5,003 rows, never a
description of how the thing should look. It is marked `INFERRED` precisely so
it is not mistaken for direction.

## Reuse is an art decision, not only a data one

The inventory proposes reuse: 3,964 object ids are flagged
`SHARED_CANDIDATE` because they share a name and category with another id, and
21 are `ALIAS_OF` because the data demonstrates a disguise relationship.

`ALIAS_OF` is settled: `reference/game/src/objects.hh::getDisguise` shows the
client is told to draw the target type, so those ids cannot look different.

`SHARED_CANDIDATE` is a proposal. Two ids sharing the name "wall" may be the
same wall seen from two angles, or two different walls. The artist confirms or
splits the group, and records the outcome in `Representation`.
