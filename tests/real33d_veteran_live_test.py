#!/usr/bin/env python3
"""Replay a complete VETERAN-PLAY-002 trace against the progression gate."""
import collections
import json
import sys
from pathlib import Path

ORIGIN = (32096, 32208, 7)


def carried_items(observation):
    counts = collections.Counter()
    owned = observation.get("owned", {})
    for item in owned.get("equipment", []):
        counts[item["id"]] += item.get("count", 1)
    for container in owned.get("containers", []):
        if container.get("source") == "carried":
            for item in container.get("items", []):
                counts[item["id"]] += item.get("count", 1)
    return counts


def evaluate(path):
    events = [json.loads(line) for line in Path(path).read_text(encoding="utf-8").splitlines()]
    kinds = collections.Counter(event["event"] for event in events)
    starts = [e for e in events if e["event"] == "session_start"]
    ends = [e for e in events if e["event"] == "session_end"]
    observations = {e["observation_id"]: e["observation"] for e in events
                    if e["event"] == "observation"}
    chronological = [e["observation"] for e in events if e["event"] == "observation"]
    decisions = [e for e in events if e["event"] == "brain_decision"]
    intents = {e["action_id"]: e for e in events if e["event"] == "intent"}
    dispatches = [e for e in events if e["event"] == "dispatch" and e.get("accepted")]
    validations = collections.defaultdict(dict)
    for event in events:
        if event["event"] == "validation":
            validations[event.get("action_id")][event["stage"]] = event
    failures = []
    if len(starts) != 1 or len(ends) != 1:
        failures.append("one complete session required")
    start = starts[0] if starts else {}
    if start.get("mode") != "aldric" or start.get("model") != "qwen3:4b" \
       or start.get("mock_disabled") is not True:
        failures.append("real qwen3:4b with mock disabled required")
    if not start.get("local_knowledge_records") or start.get("local_knowledge_error"):
        failures.append("REAL33D 7.72 local knowledge must load")
    if not chronological:
        failures.append("current client observations required")
    else:
        first = chronological[0]["self"]
        if (first["x"], first["y"], first["z"]) != ORIGIN:
            failures.append("first observed position must equal START_POS")
    if len(decisions) < 2:
        failures.append("multiple model decisions required")
    goals = []
    for decision in decisions:
        planner = decision.get("planner") or {}
        if decision.get("provider") != "ollama" or decision.get("model") != "qwen3:4b" \
           or not planner.get("current_goal") or not planner.get("current_plan") \
           or not planner.get("current_subgoal"):
            failures.append("model goal, plan and subgoal required on every decision")
            break
        goals.append(planner["current_goal"])
    persisted = any(a == b for a, b in zip(goals, goals[1:]))
    if not persisted:
        failures.append("goal did not persist across model decisions")
    for dispatch in dispatches:
        stages = validations[dispatch["action_id"]]
        if any(not stages.get(stage, {}).get("accepted") for stage in
               ("schema", "state", "budget")):
            failures.append("dispatch without three accepted gates")
            break
        if not str(dispatch.get("path", "")).startswith("g_game."):
            failures.append("dispatch bypassed normal client API")
            break
    if kinds["protocol_error"]:
        failures.append("protocol error in trace")
    attack_targets = {}
    for dispatch in dispatches:
        if dispatch.get("action") != "attack":
            continue
        intent = intents.get(dispatch["action_id"], {}).get("intent", {})
        target = intent.get("creatureId")
        before = observations.get(dispatch["observation_id"], {})
        for creature in before.get("visible", {}).get("creatures", []):
            if creature.get("id") == target and creature.get("kind") == "monster":
                attack_targets[target] = (creature.get("hpPercent", 100),
                                          dispatch["seq"])
    combat_hit = any(creature.get("id") in attack_targets and
                     event["seq"] > attack_targets[creature["id"]][1] and
                     creature.get("hpPercent", 100) < attack_targets[creature["id"]][0]
                     for event in events if event["event"] == "observation"
                     for creature in event["observation"].get("visible", {}).get("creatures", []))
    loot_gain = False
    economy_gain = False
    for result in (e for e in events if e["event"] == "result" and
                   e.get("authoritative_change")):
        before = observations.get(result.get("observation_id"))
        after = observations.get(result.get("observed_in"))
        if not before or not after:
            continue
        action = result.get("action")
        if action == "move_item":
            intent = intents.get(result["action_id"], {}).get("intent", {})
            source = intent.get("item")
            dest = intent.get("destination")
            source_on_ground = any(c.get("source") == "tile" and
                                   any(i.get("key") == source for i in c.get("items", []))
                                   for c in before.get("owned", {}).get("containers", []))
            dest_carried = any(c.get("source") == "carried" and
                               str(dest).startswith("c:" + str(c.get("id")) + ":")
                               for c in before.get("owned", {}).get("containers", []))
            gained = any(count > carried_items(before)[item]
                         for item, count in carried_items(after).items())
            loot_gain |= source_on_ground and dest_carried and gained
        if action in ("buy", "sell"):
            prior_money = before.get("shop", {}).get("money")
            later_money = after.get("shop", {}).get("money")
            economy_gain |= (prior_money is not None and later_money is not None
                             and prior_money != later_money and
                             carried_items(before) != carried_items(after))
    level_gain = bool(chronological and
                      chronological[-1]["self"].get("level", 0) >
                      chronological[0]["self"].get("level", 0))
    progression = level_gain or economy_gain or (combat_hit and loot_gain)
    if not progression:
        failures.append("no observed level, economy, or combat-plus-loot progression")
    replans = sum((e.get("planner") or {}).get("last_goal_status") == "replan"
                  for e in decisions)
    triggers = collections.Counter((e.get("planner") or {}).get("last_replan_trigger")
                                   for e in decisions)
    summary = dict(state="PASS" if not failures else "FAILED",
                   events=len(events), decisions=len(decisions),
                   dispatches=len(dispatches), goal_persisted=persisted,
                   replans=replans, replan_triggers={k: v for k, v in triggers.items() if k},
                   attack_targets=len(attack_targets), combat_hit=combat_hit,
                   loot_gain=loot_gain, economy_gain=economy_gain,
                   level_gain=level_gain, progression=progression,
                   first_position=tuple(chronological[0]["self"][k] for k in
                                        ("x", "y", "z")) if chronological else None,
                   failures=failures)
    return summary


if __name__ == "__main__":
    report = evaluate(sys.argv[1])
    print(json.dumps(report, sort_keys=True))
    if report["state"] != "PASS":
        raise SystemExit(1)
