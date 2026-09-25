-- Correlate two retained REAL33D2D live traces with the Session A memory file.
-- Run from the fusion32 root with REAL33D2D's bundled LuaJIT and three paths:
--   luajit tests/real33d_agent_memory_live_test.lua A.jsonl B.jsonl memory_A.json
dofile('agent/real33d2d/modules/real33d_agent/agent_schema.lua')
dofile('agent/real33d2d/modules/real33d_agent/agent_memory.lua')
dofile('agent/real33d2d/modules/real33d_agent/agent_core.lua')
local S, M, A = AgentSchema, AgentMemory, AgentCore

local function read(path)
  local handle = assert(io.open(path, 'rb'))
  local value = handle:read('*a')
  handle:close()
  return value
end

local function events(path)
  local list = {}
  for line in io.lines(path) do
    local event, reason = S.decode(line)
    assert(event, path .. ': ' .. tostring(reason))
    list[#list + 1] = event
  end
  return list
end

local function matching(list, eventName)
  local result = {}
  for _, event in ipairs(list) do
    if event.event == eventName then result[#result + 1] = event end
  end
  return result
end

local function currentView(projected)
  local creatures, tiles, items = {}, {}, {}
  for _, creature in ipairs(projected.visible.creatures) do
    creatures[creature.id] = {
      id = creature.id, monster = creature.kind == 'monster',
      position = { x = creature.x, y = creature.y, z = creature.z },
    }
  end
  for _, tile in ipairs(projected.visible.tiles) do
    tiles[tile.x .. ':' .. tile.y .. ':' .. tile.z] = {
      walkable = tile.walkable, position = { x = tile.x, y = tile.y, z = tile.z },
    }
    for _, thing in ipairs(tile.things) do items[thing.key] = thing end
  end
  for _, thing in ipairs(projected.owned.equipment) do items[thing.key] = thing end
  for _, container in ipairs(projected.owned.containers) do
    for _, thing in ipairs(container.items) do items[thing.key] = thing end
  end
  return {
    online = projected.online,
    player = { id = projected.self.id,
      position = { x = projected.self.x, y = projected.self.y, z = projected.self.z } },
    creatures = creatures, tiles = tiles, items = items,
  }
end

assert(arg[1] and arg[2] and arg[3], 'A trace, B trace and A memory required')
local a, b, memoryText = events(arg[1]), events(arg[2]), read(arg[3])
local aStart, aEnd = matching(a, 'session_start'), matching(a, 'session_end')
local bStart, bEnd = matching(b, 'session_start'), matching(b, 'session_end')
assert(#aStart == 1 and #aEnd == 1 and #bStart == 1 and #bEnd == 1)
assert(aStart[1].session_id ~= bStart[1].session_id)
assert(aStart[1].memory.state == 'new' and aStart[1].memory.records == 0)
assert(aEnd[1].memory_saved and aEnd[1].memory_records > 0)
assert(bStart[1].memory.state == 'loaded' and bStart[1].memory.records == aEnd[1].memory_records)
assert(bStart[1].memory.sessions == 2 and bEnd[1].memory_saved)

local identity = { agent = 'mock', world = '127.0.0.1_7171', character = 'Test Player A' }
local memory = assert(M.load(memoryText, identity))
assert(memory.sessions == 1 and memory:total() == aEnd[1].memory_records)
assert(memory:count('places') > 1 and memory:count('routes') > 0)
assert(memory:count('creature_sightings') > 0)
assert(select(2, M.load(memoryText, {
  agent = identity.agent, world = identity.world, character = 'Test Player B',
})) == 'memory_identity_mismatch')
assert(M.identityKey(identity) ~= M.identityKey({
  agent = identity.agent, world = identity.world, character = 'Test Player B',
}))

local aObservations, bObservations = {}, {}
for _, event in ipairs(matching(a, 'observation')) do
  aObservations[event.observation_id] = event.observation
end
for _, event in ipairs(matching(b, 'observation')) do
  bObservations[event.observation_id] = event.observation
  assert(event.observation.memory == nil)
  for field in pairs(event.observation) do
    assert(({ schema = true, online = true, self = true, visible = true,
              owned = true, social = true })[field], 'unexpected published field: ' .. field)
  end
end
for _, category in ipairs(M.categoryNames()) do
  for _, record in ipairs(memory:recall(category)) do
    assert(record.source == 'direct_observation')
    assert(aObservations[record.first_observation_id])
    assert(aObservations[record.last_observation_id])
    assert(record.creatureId == nil and record.itemKey == nil)
  end
end

local remembered = memory:recall('creature_sightings')[1]
local staleId
for _, event in ipairs(matching(a, 'observation')) do
  for _, creature in ipairs(event.observation.visible.creatures) do
    if creature.name == remembered.name then staleId = creature.id end
  end
end
assert(staleId and staleId ~= 1001)

local first = matching(b, 'observation')[1].observation
assert(first.self.x == 32097 and first.self.y == 32209 and first.self.z == 7)
local initial = currentView(first)
assert(initial.creatures[staleId] == nil)
local goal = memory:get('learned_facts', 'hunting_ground:32095:32215:7')
assert(goal and goal.value == '32095:32215:7')
for _, tile in ipairs(first.visible.tiles) do
  assert(not (tile.x >= goal.x and tile.x < goal.x + M.BUCKET
    and tile.y >= goal.y and tile.y < goal.y + M.BUCKET and tile.z == goal.z),
    'remembered location was initially visible')
end
assert(select(2, A.validate({ action = 'attack', creatureId = staleId }, initial))
  == 'creature_not_visible')
assert(select(2, A.validate({ action = 'follow', creatureId = staleId }, initial))
  == 'creature_not_visible')
assert(select(2, A.validate({ action = 'use', item = 'i:99' }, initial))
  == 'item_not_visible')
assert(select(2, A.validate({ action = 'move', direction = 2 },
  { online = true, player = initial.player, tiles = {} })) == 'tile_not_visible_walkable')

local fresh
for _, event in ipairs(matching(b, 'observation')) do
  if currentView(event.observation).creatures[staleId] then
    fresh = event.observation
    break
  end
end
assert(fresh and fresh.self.y > first.self.y)
assert(A.validate({ action = 'attack', creatureId = staleId }, currentView(fresh)))

local guided
for _, event in ipairs(matching(b, 'intent')) do
  if event.plan and event.plan.memory_guided then guided = event; break end
end
assert(guided and guided.intent.action == 'move' and guided.intent.direction == 2)
assert(guided.plan.navigating_to == goal.value)
local before = assert(bObservations[guided.observation_id])
assert(before.self.y == 32209)
local view = currentView(before)
assert(A.validate(guided.intent, view))
local target = view.tiles[before.self.x .. ':' .. (before.self.y + 1) .. ':' .. before.self.z]
assert(target and target.walkable)

local stages, dispatch, result = {}, nil, nil
for _, event in ipairs(b) do
  if event.correlation_id == guided.correlation_id then
    if event.event == 'validation' then stages[event.stage] = event.accepted end
    if event.event == 'dispatch' then dispatch = event end
    if event.event == 'result' then result = event end
  end
end
assert(stages.schema and stages.state and stages.budget)
assert(dispatch and dispatch.accepted and dispatch.path == 'g_game.move')
assert(result and result.authoritative_change and result.observed_in)
local after = assert(bObservations[result.observed_in])
assert(after.self.y == before.self.y + 1)
assert(#matching(b, 'protocol_error') == 0)
print('REAL33D_AGENT_MEMORY_LIVE two sessions, persistence, navigation, stale guard, isolation PASS')
