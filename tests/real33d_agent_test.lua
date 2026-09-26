-- Run with REAL33D2D's built LuaJIT: luajit tests/real33d_agent_test.lua
--
-- Sections:
--   1. REAL33D-AGENT-MVP-001 policy, budget, brain, loot/eat, viewport (preserved)
--   2. REAL33D-AGENT-BRIDGE-001 schemas, correlation ids, JSONL, opt-in default
dofile('agent/real33d2d/modules/real33d_agent/agent_schema.lua')
dofile('agent/real33d2d/modules/real33d_agent/agent_memory.lua')
dofile('agent/real33d2d/modules/real33d_agent/agent_core.lua')
dofile('agent/real33d2d/modules/real33d_agent/agent_bridge.lua')
local A = AgentCore
local S = AgentSchema
local function eq(a, b) assert(a == b, tostring(a) .. ' ~= ' .. tostring(b)) end
local function contains(haystack, needle)
  assert(haystack:find(needle, 1, true), needle .. ' not found in ' .. haystack)
end

local obs = {
  online = true,
  player = { id = 1, position = { x = 100, y = 200, z = 7 }, hp = 100, maxHp = 100 },
  tiles = { ['101:200:7'] = { walkable = true }, ['100:199:7'] = { walkable = false } },
  creatures = { [2] = { id = 2, monster = true, position = { x = 101, y = 200 } },
                [3] = { id = 3, monster = false } },
  creatureList = { { id = 2, name = 'Rat', monster = true, hpPercent = 100,
                     position = { x = 101, y = 200, z = 7 } } },
  items = { ['i:3'] = { item = true, count = 2, container = true } },
  destinations = { ['c:0:0'] = { place = 'container' } },
  inventoryKeys = { 'i:3' }, containers = {}, combat = { fight = 2, chase = 0, safe = true }
}
eq(A.validate({ action = 'move', direction = 1 }, obs).action, 'move')
eq(select(2, A.validate({ action = 'move', direction = 0 }, obs)), 'tile_not_visible_walkable')
eq(select(2, A.validate({ action = 'move', direction = 4 }, obs)), 'direction')
eq(select(2, A.validate({ action = 'attack', creatureId = 999 }, obs)), 'creature_not_visible')
eq(select(2, A.validate({ action = 'attack', creatureId = 3 }, obs)), 'attack_monster_only')
eq(A.validate({ action = 'follow', creatureId = 3 }, obs).action, 'follow')
eq(select(2, A.validate({ action = 'say', text = 'a\nb' }, obs)), 'chat_text')
eq(select(2, A.validate({ action = 'teleport', x = 2 }, obs)), 'unknown_action')
eq(select(2, A.validate({ action = 'use', item = 'i:99' }, obs)), 'item_not_visible')
eq(A.validate({ action = 'open_container', item = 'i:3' }, obs).action, 'open_container')
obs.items['i:4'] = { item = true, count = 1, container = false }
eq(select(2, A.validate({ action = 'open_container', item = 'i:4' }, obs)), 'item_not_container')
eq(select(2, A.validate({ action = 'move_item', item = 'i:3', destination = 'c:0:0', count = 3 }, obs)), 'item_count')
eq(A.validate({ action = 'move_item', item = 'i:3', destination = 'c:0:0', count = 1 }, obs).action, 'move_item')
eq(select(2, A.validate({ action = 'combat_mode', fight = 4, chase = 0, safe = true }, obs)), 'combat_mode')
eq(select(2, A.validate({ action = 'combat_mode', fight = 1, chase = 1, safe = true }, obs)), 'combat_mode_one_change')
eq(A.validate({ action = 'combat_mode', fight = 1, chase = 0, safe = true }, obs).action, 'combat_mode')

local budget = A.newBudget()
eq(budget:allow('move', 1000), true)
eq(select(2, budget:allow('move', 1100)), 'action_cooldown')
eq(budget:allow('move', 1800), true)
for i = 3, 24 do eq(budget:allow('move', 1800 + (i - 2) * 800), true) end
eq(select(2, budget:allow('move', 21000)), 'minute_budget')
eq(budget:allow('move', 62000), true)

local brain = A.mockBrain()
local intent
local function decide(now) brain:decide(obs, function(v) intent = v end, now) return intent end
eq(decide(0).action, 'attack')
obs.attackId = 2
eq(decide(3000).action, 'combat_mode')
obs.combat.chase = 1
eq(decide(6000).action, 'say')
eq(decide(9000).action, 'open_container')
eq(decide(20000), nil) -- visible damaged target is never cut off by elapsed time
obs.creatures[2] = nil
eq(decide(39000).action, 'cancel_attack')
obs.attackId = nil
obs.creatures[2] = { id = 2, monster = true, position = { x = 101, y = 200 } }
eq(decide(42000).action, 'move')
eq(decide(45000).action, 'attack')
local followBrain = A.mockBrain()
obs.followId = 2
local followIntent
followBrain:decide(obs, function(v) followIntent = v end, 0)
eq(followIntent.action, 'attack')
obs.followId = nil
local escapeBrain = A.mockBrain()
obs.player.hp = 40
obs.tiles['99:200:7'] = { walkable = true }
local escapeIntent
escapeBrain:decide(obs, function(v) escapeIntent = v end, 0)
eq(escapeIntent.action, 'move')
eq(escapeIntent.direction, 3)
obs.player.hp = 100
local manualBrain = A.mockBrain()
obs.attackId = 3
local manualIntent = 'unexpected'
manualBrain:decide(obs, function(v) manualIntent = v end, 0)
eq(manualIntent, nil)
obs.attackId = nil

local lootObs = {
  online = true,
  player = { id = 1, position = { x = 100, y = 200, z = 7 }, hp = 100, maxHp = 100 },
  tiles = { ['101:200:7'] = { position = { x = 101, y = 200, z = 7 },
    walkable = true, things = { { key = 't:101:200:7:1', id = 4173, container = true } } } },
  creatures = {}, creatureList = {}, attackId = nil, followId = nil,
  inventoryKeys = { 'i:3' },
  items = { ['i:3'] = { item = true, container = true, count = 1 },
            ['t:101:200:7:1'] = { item = true, container = true, count = 1 } },
  containers = { { id = 0, name = 'bag', capacity = 8, itemCount = 2, items = {} } },
  destinations = { ['c:0:2'] = { place = 'container' } },
  combat = { fight = 2, chase = 1, safe = true }
}
local lootBrain = A.mockBrain()
lootBrain.greeted, lootBrain.inventoryTried, lootBrain.inventoryUsed = true, true, true
lootBrain.bagReopened, lootBrain.bagId = true, 0
lootBrain.pendingLoot, lootBrain.lootStartedAt = true, 0
lootBrain.lootPosition = { x = 101, y = 200, z = 7 }
local lootIntent
local function lootDecide(now)
  lootBrain:decide(lootObs, function(v) lootIntent = v end, now)
  return lootIntent
end
eq(lootDecide(0).action, 'open_container')
lootObs.items['c:1:0'] = { id = 3585, item = true, usable = false, count = 1 }
lootObs.containers[2] = { id = 1, name = 'dead rabbit', capacity = 8, itemCount = 1,
  items = { { key = 'c:1:0', id = 3585, count = 1, container = false } } }
local move = lootDecide(3000)
eq(move.action, 'move_item')
eq(move.destination, 'c:0:2')
lootObs.items['c:0:2'] = { id = 3585, item = true, usable = false, count = 1 }
lootObs.containers[1].itemCount = 3
lootObs.containers[1].items[1] = { key = 'c:0:2', id = 3585, count = 1, container = false }
lootObs.containers[2].itemCount, lootObs.containers[2].items = 0, {}
eq(lootDecide(6000).action, 'use')
lootObs.items['c:0:2'] = nil
lootObs.containers[1].itemCount, lootObs.containers[1].items = 2, {}
lootDecide(9000)
eq(lootBrain.looted, true)
eq(lootBrain.ate, true)
-- A completed loot must not leave the brain permanently in "looting" state.
-- pendingLoot gates target selection, so if the success path leaks it the
-- agent never attacks anything again for the rest of the session.
eq(lootBrain.pendingLoot, false)
lootObs.creatures[7] = { id = 7, monster = true, hpPercent = 100,
                         position = { x = 101, y = 200, z = 7 } }
lootObs.creatureList[1] = { id = 7, name = 'Rat', monster = true, hpPercent = 100,
                            position = { x = 101, y = 200, z = 7 } }
eq(lootDecide(12000).action, 'attack')
local foodBrain = A.mockBrain()
foodBrain.greeted, foodBrain.inventoryTried, foodBrain.inventoryUsed = true, true, true
foodBrain.bagReopened, foodBrain.bagId = true, 0
local foodObs = {
  online = true, player = { id = 1, position = { x = 100, y = 200, z = 7 }, hp = 100, maxHp = 100 },
  tiles = {}, creatures = {}, creatureList = {}, inventoryKeys = { 'i:3' },
  items = { ['i:3'] = { item = true, container = true, count = 1 },
            ['c:0:0'] = { id = 3577, item = true, count = 1 } },
  containers = { { id = 0, name = 'bag', itemCount = 1, capacity = 8,
    items = { { key = 'c:0:0', id = 3577, count = 1 } } } },
  combat = { fight = 2, chase = 1, safe = true }
}
local foodIntent
foodBrain:decide(foodObs, function(v) foodIntent = v end, 0)
eq(foodIntent.action, 'use')
eq(foodIntent.item, 'c:0:0')
foodObs.items['c:0:0'] = nil
foodObs.containers[1].items, foodObs.containers[1].itemCount = {}, 0
foodBrain:decide(foodObs, function() end, 3000)
eq(foodBrain.ate, true)

json = { encode = function() return '{}' end,
         decode = function() return { action = 'move', direction = 1 } end }
local called
local ollama = A.ollamaBrain(function(url, request, callback)
  eq(url, 'http://127.0.0.1:11434/api/chat')
  eq(request.model, 'test-model')
  eq(request.stream, false)
  callback({ message = { content = '{"action":"move","direction":1}' } }, nil)
end, 'test-model')
ollama:decide(obs, function(v) called = v end)
eq(called.action, 'move')

local item = { getId = function() return 870 end, getCount = function() return 1 end,
  getName = function() return 'backpack' end, isUsable = function() return false end,
  isItem = function() return true end, isContainer = function() return false end,
  getPosition = function() return { x = 65535, y = 3, z = 0 } end }
-- Mirrors Tile::isCovered: a tile sitting on the floor it is asked about is
-- never covered, so the answer depends on the argument. Passing a constant 0
-- (the MVP behaviour) reports every tile as covered here, which is what made
-- the agent observe an empty map indoors and underground.
local tile = { isCovered = function(_, firstFloor) return firstFloor ~= 7 end,
  isWalkable = function() return true end,
  getThings = function() return {} end }
local player = { getId = function() return 1 end, getName = function() return 'Test' end,
  getPosition = function() return { x = 100, y = 200, z = 7 } end,
  getHealth = function() return 100 end, getMaxHealth = function() return 100 end,
  getMana = function() return 20 end, getMaxMana = function() return 20 end,
  getLevel = function() return 1 end, getMagicLevel = function() return 0 end,
  getSkillLevel = function() return 10 end,
  getInventoryItem = function(_, slot) return slot == 3 and item or nil end }
local seenCreature = { getId = function() return 2 end, getName = function() return 'Rat' end,
  getPosition = function() return { x = 101, y = 200, z = 7 } end,
  getHealthPercent = function() return 100 end,
  isMonster = function() return true end, isPlayer = function() return false end,
  isNpc = function() return false end, canBeSeen = function() return true end }
local hiddenCreature = { getId = function() return 9 end, getName = function() return 'Hidden' end,
  getPosition = function() return { x = 130, y = 200, z = 7 } end,
  getHealthPercent = function() return 100 end, isMonster = function() return true end,
  isPlayer = function() return false end, isNpc = function() return false end,
  canBeSeen = function() return true end }
local panel = { isInRange = function(_, p) return math.abs(p.x - 100) <= 1 and math.abs(p.y - 200) <= 1 end,
  getSightSpectators = function() return { seenCreature, hiddenCreature } end }
modules = { game_interface = { getMapPanel = function() return panel end } }
g_game = { isOnline = function() return true end, getLocalPlayer = function() return player end,
  getAttackingCreature = function() return nil end, getFollowingCreature = function() return nil end,
  getFightMode = function() return 2 end, getChaseMode = function() return 0 end,
  isSafeFight = function() return true end, getContainers = function() return {} end }
g_map = { getTile = function(p) if p.x == 130 then error('hidden map queried') end return tile end }
function item:getStackPos() return 0 end
function item:isItem() return true end
function item:isContainer() return false end
function tile:getThings() return {} end
dofile('agent/real33d2d/modules/real33d_agent/agent_runtime.lua')
local observed = Real33DAgentRuntime.observe()
eq(observed.player.hp, 100)
eq(observed.creatures[2].name, 'Rat')
eq(observed.creatures[9], nil)
eq(observed.inventory[3].id, 870)
eq(observed.tiles['105:200:7'], nil)
-- Tiles on the player's own floor inside the viewport must survive the covered
-- filter, or the agent is blind wherever anything is drawn above it.
assert(observed.tiles['100:200:7'], 'own tile filtered out by the covered check')
assert(observed.tiles['101:200:7'], 'adjacent visible tile filtered out')
eq(observed.tiles['101:200:7'].walkable, true)
print('REAL33D_AGENT_MVP core and viewport tests PASS')

-- ===========================================================================
-- REAL33D-AGENT-BRIDGE-001
-- ===========================================================================

local function validObservation()
  return {
    online = true,
    player = { id = 1, name = 'QA', position = { x = 100, y = 200, z = 7 },
               hp = 155, maxHp = 155, mana = 40, maxMana = 40, level = 8,
               magicLevel = 0, skills = { [0] = 10, [1] = 11 } },
    tiles = { ['101:200:7'] = { position = { x = 101, y = 200, z = 7 },
                                walkable = true, things = {} } },
    creatures = { [2] = { id = 2, name = 'Rat', monster = true, hpPercent = 100,
                          position = { x = 101, y = 200, z = 7 } } },
    creatureList = { { id = 2, name = 'Rat', monster = true, hpPercent = 100,
                       position = { x = 101, y = 200, z = 7 } } },
    items = { ['i:3'] = { id = 870, item = true, count = 1, container = true,
                          name = '', usable = false } },
    destinations = { ['c:0:0'] = { place = 'container', id = 0, slot = 0 } },
    inventory = { [3] = { id = 870, count = 1, container = true } },
    inventoryKeys = { 'i:3' },
    containers = { { id = 0, name = 'bag', capacity = 8, itemCount = 0,
                     sourcePlace = 'carried', items = {} } },
    chat = { { name = 'QA', mode = 1, text = 'hello' } },
    combat = { fight = 2, chase = 0, safe = true },
  }
end

-- --- schema acceptance -----------------------------------------------------
eq(S.checkObservation(validObservation()) ~= nil, true)
eq(S.checkObservation({ online = false }) ~= nil, true)
eq(#S.actions(), 13)
local accepted = {
  { action = 'move', direction = 0 },
  { action = 'say', text = 'hello' },
  { action = 'attack', creatureId = 2 },
  { action = 'follow', creatureId = 2 },
  { action = 'cancel_attack' },
  { action = 'cancel_follow' },
  { action = 'use', item = 'i:3' },
  { action = 'open_container', item = 'i:3' },
  { action = 'use_with', item = 'i:3', targetCreatureId = 2 },
  { action = 'use_with', item = 'i:3', targetItem = 'c:0:0' },
  { action = 'move_item', item = 'i:3', destination = 'c:0:0', count = 1 },
  { action = 'combat_mode', fight = 2, chase = 1, safe = true },
}
for _, intent in ipairs(accepted) do
  local normalized, reason = S.checkIntent(intent)
  assert(normalized, (intent.action or '?') .. ' rejected: ' .. tostring(reason))
  eq(normalized.action, intent.action)
end

-- --- malformed observation rejection ---------------------------------------
eq(select(2, S.checkObservation('nope')), 'schema_observation_not_table')
eq(select(2, S.checkObservation({})), 'schema_missing_field:online')
local noTiles = validObservation() noTiles.tiles = nil
eq(select(2, S.checkObservation(noTiles)), 'schema_missing_field:tiles')
local badHp = validObservation() badHp.player.maxHp = 0
eq(select(2, S.checkObservation(badHp)), 'schema_player_health')
local badPos = validObservation() badPos.player.position = { x = 1, y = 2 }
eq(select(2, S.checkObservation(badPos)), 'schema_player_identity')
local badCombat = validObservation() badCombat.combat.safe = 1
eq(select(2, S.checkObservation(badCombat)), 'schema_combat_mode')
local badAttack = validObservation() badAttack.attackId = 'x'
eq(select(2, S.checkObservation(badAttack)), 'schema_type:attackId')

-- The fair-play boundary as an executable check: an observation that grew a
-- field outside the whitelist is refused before any Brain can read it.
for _, forbidden in ipairs({ 'fullMap', 'minimap', 'spawns', 'database',
                            'hiddenCreatures', 'otherPlayers' }) do
  local leaky = validObservation()
  leaky[forbidden] = { anything = true }
  eq(select(2, S.checkObservation(leaky)), 'schema_forbidden_field:' .. forbidden)
end

-- --- malformed intent rejection --------------------------------------------
eq(select(2, S.checkIntent('nope')), 'schema_intent_not_table')
eq(select(2, S.checkIntent({})), 'schema_missing_action')
eq(select(2, S.checkIntent({ action = 'teleport' })), 'schema_unknown_action')
eq(select(2, S.checkIntent({ action = 'move' })), 'schema_missing_field:direction')
eq(select(2, S.checkIntent({ action = 'move', direction = '1' })), 'schema_type:direction')
eq(select(2, S.checkIntent({ action = 'move', direction = 1.5 })), 'schema_type:direction')
eq(select(2, S.checkIntent({ action = 'move', direction = 9 })), 'schema_range:direction')
eq(select(2, S.checkIntent({ action = 'move', direction = 1, extra = 1 })),
   'schema_unknown_field:extra')
eq(select(2, S.checkIntent({ action = 'say', text = '' })), 'schema_length:text')
eq(select(2, S.checkIntent({ action = 'say', text = string.rep('a', 101) })),
   'schema_length:text')
eq(select(2, S.checkIntent({ action = 'say', text = 'a\nb' })),
   'schema_control_character:text')
eq(select(2, S.checkIntent({ action = 'combat_mode', fight = 2, chase = 0, safe = 'yes' })),
   'schema_type:safe')
eq(select(2, S.checkIntent({ action = 'combat_mode', fight = 9, chase = 0, safe = true })),
   'schema_range:fight')
eq(select(2, S.checkIntent({ action = 'move_item', item = 'i:3',
                             destination = 'c:0:0', count = 0 })), 'schema_range:count')
eq(select(2, S.checkIntent({ action = 'use_with', item = 'i:3' })),
   'schema_use_with_one_target')
eq(select(2, S.checkIntent({ action = 'use_with', item = 'i:3', targetItem = 'c:0:0',
                             targetCreatureId = 2 })), 'schema_use_with_one_target')

-- --- invisible and stale target rejection ----------------------------------
local live = validObservation()
eq(A.validate({ action = 'attack', creatureId = 2 }, live).action, 'attack')
eq(select(2, A.validate({ action = 'attack', creatureId = 4242 }, live)),
   'creature_not_visible')

-- Staleness: the intent was formed against `live`, but dispatch revalidates
-- against the observation read back after the Brain answered. A target that
-- left the viewport in between must not be actionable.
local stale = validObservation()
stale.creatures, stale.creatureList = {}, {}
eq(select(2, A.validate({ action = 'attack', creatureId = 2 }, stale)),
   'creature_not_visible')
eq(select(2, A.validate({ action = 'follow', creatureId = 2 }, stale)),
   'creature_not_visible')
local movedAway = validObservation()
movedAway.tiles = {}
eq(select(2, A.validate({ action = 'move', direction = 1 }, movedAway)),
   'tile_not_visible_walkable')

-- --- invalid item / container reference rejection ---------------------------
eq(select(2, A.validate({ action = 'use', item = 'i:99' }, live)), 'item_not_visible')
eq(select(2, A.validate({ action = 'open_container', item = 'i:99' }, live)),
   'item_not_visible')
local plain = validObservation()
plain.items['i:4'] = { item = true, count = 1, container = false }
eq(select(2, A.validate({ action = 'open_container', item = 'i:4' }, plain)),
   'item_not_container')
eq(select(2, A.validate({ action = 'move_item', item = 'i:3',
                          destination = 'c:9:9', count = 1 }, live)),
   'item_or_destination_not_visible')
eq(select(2, A.validate({ action = 'move_item', item = 'i:3',
                          destination = 'c:0:0', count = 5 }, live)), 'item_count')
eq(select(2, A.validate({ action = 'use_with', item = 'i:99',
                          targetItem = 'i:3' }, live)), 'source_not_visible')
eq(select(2, A.validate({ action = 'use_with', item = 'i:3',
                          targetCreatureId = 4242 }, live)), 'target_not_visible')

-- --- action budget and per-action cooldown ---------------------------------
local b = A.newBudget()
eq(b:allow('move', 1000), true)
eq(select(2, b:allow('move', 1200)), 'action_cooldown')      -- move gap is 750ms
eq(select(2, b:allow('say', 1300)), 'global_cooldown')        -- any action, 600ms
eq(b:allow('say', 2000), true)
eq(select(2, b:allow('say', 10000)), 'action_cooldown')       -- chat gap is 15s
eq(b:allow('say', 17001), true)
local budget2 = A.newBudget()
local t = 0
for _ = 1, 24 do t = t + 800 eq(budget2:allow('move', t), true) end
eq(select(2, budget2:allow('move', t + 800)), 'minute_budget')
eq(budget2:allow('move', t + 61000), true)                    -- window rolls off

-- --- ids and correlation ---------------------------------------------------
local lines = {}
local clock = 0
local bridge = Real33DAgentBridge.new({
  sessionId = 'TEST',
  sink = function(line) lines[#lines + 1] = line end,
  clock = function() return clock end,
})

local obs1 = validObservation()
local cycle = bridge:beginCycle(obs1, S.projectObservation(obs1))
eq(cycle.correlationId, 'c-TEST-000001')
eq(cycle.observationId, 'o-TEST-000001')
eq(#lines, 1)
contains(lines[1], '"event":"observation"')
contains(lines[1], '"observation_id":"o-TEST-000001"')

local intent = { action = 'move', direction = 1 }
local actionId = bridge:emitIntent(cycle, intent, 'mock')
eq(actionId, 'a-TEST-000001')
contains(lines[2], '"event":"intent"')
contains(lines[2], '"action_id":"a-TEST-000001"')
contains(lines[2], '"correlation_id":"c-TEST-000001"')

bridge:emitValidation(cycle, 'schema', true, nil, intent)
contains(lines[3], '"event":"validation"')
contains(lines[3], '"stage":"schema"')
contains(lines[3], '"accepted":true')

clock = 500
bridge:emitDispatch(cycle, intent, true, nil, Real33DAgentBridge.snapshot(obs1), 'g_game.walk')
contains(lines[4], '"event":"dispatch"')
contains(lines[4], '"path":"g_game.walk"')

-- The server moved the player. The next cycle must close the loop back onto
-- the originating action and observation.
clock = 1500
local obs2 = validObservation()
obs2.player.position.x = 101
obs2.player.hp = 150
local cycle2 = bridge:beginCycle(obs2, S.projectObservation(obs2))
eq(cycle2.observationId, 'o-TEST-000002')
eq(cycle2.correlationId, 'c-TEST-000002')
local result = lines[5]
contains(result, '"event":"result"')
contains(result, '"action_id":"a-TEST-000001"')       -- originating action
contains(result, '"correlation_id":"c-TEST-000001"')  -- originating cycle
contains(result, '"observation_id":"o-TEST-000001"')  -- observation decided on
contains(result, '"observed_in":"o-TEST-000002"')     -- where the effect showed
contains(result, '"authoritative_change":true')
contains(result, '"elapsed_ms":1000')
contains(result, '"field":"x"')
contains(result, '"field":"hp"')
contains(lines[6], '"event":"observation"')

-- No pending action means no result event is invented.
local before = #lines
bridge:beginCycle(validObservation(), S.projectObservation(validObservation()))
eq(#lines, before + 1)
contains(lines[before + 1], '"event":"observation"')

-- A dispatch the server did not act on is reported as such, not hidden.
local quiet = Real33DAgentBridge.new({ sessionId = 'Q',
  sink = function(line) lines[#lines + 1] = line end, clock = function() return 0 end })
local q = quiet:beginCycle(obs1, S.projectObservation(obs1))
quiet:emitIntent(q, intent, 'mock')
quiet:emitDispatch(q, intent, true, nil, Real33DAgentBridge.snapshot(obs1), 'g_game.walk')
quiet:beginCycle(obs1, S.projectObservation(obs1))
contains(lines[#lines - 1], '"authoritative_change":false')

-- Chat echo and container change are observable consequences too.
local s1 = Real33DAgentBridge.snapshot(validObservation())
local spoken = validObservation()
spoken.chat[#spoken.chat + 1] = { name = 'QA', mode = 1, text = 'Hello' }
eq(#Real33DAgentBridge.diff(s1, Real33DAgentBridge.snapshot(spoken)), 1)
local looted = validObservation()
looted.containers[1].itemCount = 1
looted.containers[1].items[1] = { key = 'c:0:0', id = 3577, count = 1 }
eq(#Real33DAgentBridge.diff(s1, Real33DAgentBridge.snapshot(looted)), 1)

-- --- JSONL serialization ---------------------------------------------------
eq(S.encode(nil), 'null')
eq(S.encode(true), 'true')
eq(S.encode(7), '7')
eq(S.encode(-3), '-3')
eq(S.encode(1.5), '1.5')
eq(S.encode(0 / 0), 'null')       -- nan would not parse as JSON
eq(S.encode(math.huge), 'null')
eq(S.encode('a"b\\c'), '"a\\"b\\\\c"')
eq(S.encode('line\nbreak'), '"line\\nbreak"')
eq(S.encode('\1'), '"\\u0001"')
eq(S.encode(S.array({})), '[]')
eq(S.encode(S.object({})), '{}')
eq(S.encode({ 'a', 'b' }), '["a","b"]')
-- Key order is sorted, not insertion order, so equal state encodes identically.
eq(S.encode({ b = 1, a = 2 }), '{"a":2,"b":1}')
local one, two = {}, {}
one.z, one.a, one.m = 1, 2, 3
two.m, two.z, two.a = 3, 1, 2
eq(S.encode(one), S.encode(two))
-- A number-keyed map stays an object; it must not silently become an array.
eq(S.encode(S.object({ [1] = 'x', [2] = 'y' })), '{"1":"x","2":"y"}')

-- Every emitted line is exactly one JSON object on one line.
for _, line in ipairs(lines) do
  eq(line:sub(1, 1), '{')
  eq(line:sub(-1), '}')
  eq(line:find('\n'), nil)
  eq(line:find('\r'), nil)
  contains(line, '"schema":"real33d.agent.event/1"')
  contains(line, '"session_id"')
  contains(line, '"seq":')
  contains(line, '"ts":')
end

-- The published projection carries the four documented groups and nothing that
-- would reveal state the client cannot see.
local projected = S.projectObservation(validObservation())
eq(projected.schema, 'real33d.agent.observation/1')
eq(projected.self.hp, 155)
eq(projected.self.x, 100)
eq(#projected.visible.creatures, 1)
eq(projected.visible.creatures[1].kind, 'monster')
eq(#projected.visible.tiles, 1)
eq(#projected.owned.equipment, 1)
eq(projected.owned.equipment[1].slot, 3)
eq(projected.owned.equipment[1].key, 'i:3')
eq(#projected.owned.containers, 1)
eq(#projected.social.chat, 1)
eq(projected.fullMap, nil)
eq(projected.destinations, nil)
eq(S.projectObservation({ online = false }).online, false)
local encoded = S.encode(projected)
eq(encoded:find('\n'), nil)
contains(encoded, '"kind":"monster"')

-- --- cross-session memory is not part of a bridge decision ------------------
-- The MVP brain may reuse an item id remembered from an earlier login. A bridge
-- brain may not: with memory off, the same observation produces no such intent.
local function foodObservation()
  return {
    online = true,
    player = { id = 1, position = { x = 100, y = 200, z = 7 }, hp = 100, maxHp = 100 },
    tiles = {}, creatures = {}, creatureList = {}, inventoryKeys = { 'i:3' },
    items = { ['i:3'] = { item = true, container = true, count = 1 },
              ['c:0:0'] = { id = 3577, item = true, count = 1 } },
    containers = { { id = 0, name = 'bag', itemCount = 1, capacity = 8,
      items = { { key = 'c:0:0', id = 3577, count = 1 } } } },
    combat = { fight = 2, chase = 1, safe = true },
  }
end
local function primed(brainInstance)
  brainInstance.greeted, brainInstance.inventoryTried = true, true
  brainInstance.inventoryUsed, brainInstance.bagReopened = true, true
  brainInstance.bagId = 0
  return brainInstance
end
local remembering = primed(A.mockBrain())
local rememberIntent
remembering:decide(foodObservation(), function(v) rememberIntent = v end, 0)
eq(rememberIntent.action, 'use')          -- preserved MVP behaviour
eq(rememberIntent.item, 'c:0:0')
local forgetful = primed(A.mockBrain({ crossSessionMemory = false }))
eq(forgetful.crossSessionMemory, false)
local forgetIntent = 'unset'
forgetful:decide(foodObservation(), function(v) forgetIntent = v end, 0)
eq(forgetIntent, nil)

-- --- loot goes for value, not for slot order -------------------------------
-- Reproduces an observed live corpse: dead rat holding worm x2 in slot 0 and
-- gold coin x2 in slot 1. Picking the first non-container item took the worm.
local function corpseObs(items)
  local o = validObservation()
  o.creatures, o.creatureList = {}, {}
  o.containers = {
    { id = 0, name = 'bag', capacity = 8, itemCount = 0, sourcePlace = 'carried',
      items = {} },
    { id = 1, name = 'dead rat', capacity = 8, itemCount = #items,
      sourcePlace = 'tile', items = items },
  }
  for _, it in ipairs(items) do
    o.items[it.key] = { id = it.id, item = true, count = it.count,
                        container = false, usable = it.usable or false }
  end
  o.destinations['c:0:0'] = { place = 'container', id = 0, slot = 0 }
  return o
end
local function lootChoice(items)
  local brain = A.mockBrain({ crossSessionMemory = false })
  brain.greeted, brain.inventoryTried, brain.inventoryUsed = true, true, true
  brain.bagReopened, brain.bagId = true, 0
  brain.lootStage, brain.corpseId = 'ready', 1
  brain.pendingLoot, brain.lootStartedAt = true, 0
  local got
  brain:decide(corpseObs(items), function(v) got = v end, 100)
  return got
end

local ratLoot = lootChoice({
  { key = 'c:1:0', id = 3492, count = 2, container = false },   -- worm
  { key = 'c:1:1', id = 3031, count = 2, container = false },   -- gold coin
})
eq(ratLoot.action, 'move_item')
eq(ratLoot.item, 'c:1:1')     -- the gold, even though the worm is in slot 0
eq(ratLoot.count, 2)          -- and the whole stack, not a single coin

-- Coin ranking, and food still beating unknown junk.
eq(lootChoice({ { key = 'c:1:0', id = 3031, count = 1, container = false },
                { key = 'c:1:1', id = 3043, count = 1, container = false } }).item,
   'c:1:1')                                                     -- crystal > gold
eq(lootChoice({ { key = 'c:1:0', id = 3031, count = 1, container = false },
                { key = 'c:1:1', id = 3035, count = 1, container = false } }).item,
   'c:1:1')                                                     -- platinum > gold
eq(lootChoice({ { key = 'c:1:0', id = 3492, count = 5, container = false },
                { key = 'c:1:1', id = 3607, count = 1, container = false,
                  usable = true } }).item, 'c:1:1')             -- food > worm
-- A corpse with only junk is still looted rather than skipped forever.
eq(lootChoice({ { key = 'c:1:0', id = 3492, count = 1, container = false } }).item,
   'c:1:0')

-- --- acquiring a container when nothing carried can hold items -------------
-- Both QA characters dropped their bag on death. With no carried container the
-- brain opens a visible ground container and then picks it up into the
-- backpack slot, using only ordinary validated actions.
local baglessObs = validObservation()
baglessObs.inventory, baglessObs.inventoryKeys = {}, {}
baglessObs.creatures, baglessObs.creatureList = {}, {}
baglessObs.containers = {}
baglessObs.items = { ['t:101:200:7:1'] = { id = 1987, item = true, count = 1,
                                           container = true } }
baglessObs.tiles = { ['101:200:7'] = { position = { x = 101, y = 200, z = 7 },
  walkable = true, things = { { key = 't:101:200:7:1', id = 1987, count = 1,
                                container = true } } } }
baglessObs.destinations['i:3'] = { place = 'inventory', slot = 3 }
local bagless = A.mockBrain({ crossSessionMemory = false })
bagless.greeted = true
local pick
bagless:decide(baglessObs, function(v) pick = v end, 0)
eq(pick.action, 'open_container')                 -- look inside it first
eq(pick.item, 't:101:200:7:1')
eq(A.validate(pick, baglessObs).action, 'open_container')
bagless:decide(baglessObs, function(v) pick = v end, 3000)
eq(pick.action, 'move_item')                      -- then carry it
eq(pick.destination, 'i:3')
eq(pick.count, 1)
eq(A.validate(pick, baglessObs).action, 'move_item')
-- While the move is outstanding the brain waits rather than spamming.
bagless:decide(baglessObs, function(v) pick = v end, 4000)
eq(pick, nil)
-- Once the container is carried, the ordinary inventory steps take over.
local carriedObs = validObservation()
carriedObs.creatures, carriedObs.creatureList = {}, {}
bagless:decide(carriedObs, function(v) pick = v end, 12000)
eq(pick.action, 'open_container')
eq(pick.item, 'i:3')

-- --- pendingLoot must expire without an open bag ---------------------------
-- A target that walks out of view sets pendingLoot. The loot machinery that
-- clears it only runs while a bag is open, so with no container open the flag
-- would otherwise stay set and disable target selection for good.
local orphan = validObservation()
orphan.containers = {}                     -- no bag open, as after walking away
local orphanBrain = A.mockBrain({ crossSessionMemory = false })
orphanBrain.greeted, orphanBrain.inventoryTried = true, true
orphanBrain.inventoryUsed, orphanBrain.bagReopened = true, true
orphanBrain.phase, orphanBrain.selectedTarget = 'attack', 99
orphanBrain.targetAt, orphanBrain.lastSeenAt = 0, 0
orphanBrain.targetPosition = { x = 101, y = 200, z = 7 }
local orphanIntent
orphanBrain:decide(orphan, function(v) orphanIntent = v end, 1000)
eq(orphanBrain.pendingLoot, true)          -- target vanished, looting armed
orphanBrain:decide(orphan, function(v) orphanIntent = v end, 20000)
eq(orphanBrain.pendingLoot, false)         -- and it expires on its own clock
eq(orphanIntent.action, 'attack')          -- so the rat in view is targeted
eq(orphanIntent.creatureId, 2)

-- --- the one container attempt is not spent on an empty inventory ----------
local lateBrain = A.mockBrain({ crossSessionMemory = false })
lateBrain.greeted = true
local empty = validObservation()
empty.inventory, empty.inventoryKeys, empty.items = {}, {}, {}
empty.creatures, empty.creatureList = {}, {}
lateBrain:decide(empty, function() end, 0)
eq(lateBrain.inventoryTried, false)        -- nothing carried yet: not spent
local arrived = validObservation()
arrived.creatures, arrived.creatureList = {}, {}
local lateIntent
lateBrain:decide(arrived, function(v) lateIntent = v end, 3000)
eq(lateIntent.action, 'open_container')
eq(lateBrain.inventoryTried, true)

-- --- bridge scenario reaches Follow before Attack --------------------------
-- followFirst is what makes the certification run exercise the follow
-- dispatcher live. Default (MVP) behaviour still attacks straight away.
local scenarioObs = validObservation()
scenarioObs.attackId, scenarioObs.followId = nil, nil
local plainBrain = A.mockBrain()
plainBrain.greeted, plainBrain.inventoryTried, plainBrain.inventoryUsed = true, true, true
plainBrain.bagReopened = true
local plainIntent
plainBrain:decide(scenarioObs, function(v) plainIntent = v end, 0)
eq(plainIntent.action, 'attack')

local followBridge = A.mockBrain({ crossSessionMemory = false, followFirst = true })
followBridge.greeted, followBridge.inventoryTried = true, true
followBridge.inventoryUsed, followBridge.bagReopened = true, true
local step
followBridge:decide(scenarioObs, function(v) step = v end, 0)
eq(step.action, 'follow')
eq(step.creatureId, 2)
eq(A.validate(step, scenarioObs).action, 'follow')
-- Once the server confirms the follow, the same brain converts it to an attack.
scenarioObs.followId = 2
followBridge:decide(scenarioObs, function(v) step = v end, 2500)
eq(step.action, 'attack')
eq(step.creatureId, 2)
-- Follow is chosen only once per session, not on every target.
followBridge.selectedTarget, followBridge.phase = nil, nil
scenarioObs.followId, scenarioObs.attackId = nil, nil
followBridge:decide(scenarioObs, function(v) step = v end, 6000)
eq(step.action, 'attack')

-- --- the scenario releases a target once, after chase is on ----------------
local cancelObs = validObservation()
local cancelBrain = A.mockBrain({ crossSessionMemory = false, proveCancel = true })
cancelBrain.greeted, cancelBrain.inventoryTried = true, true
cancelBrain.inventoryUsed, cancelBrain.bagReopened = true, true
cancelBrain.selectedTarget, cancelBrain.phase = 2, 'attack'
cancelBrain.lastSeenAt, cancelBrain.targetAt = 0, 0
local cancelStep
-- Attacking but chase is still off: combat_mode comes first, not the cancel.
cancelObs.attackId, cancelObs.combat.chase = 2, 0
cancelBrain:decide(cancelObs, function(v) cancelStep = v end, 1000)
eq(cancelStep.action, 'combat_mode')
eq(cancelStep.chase, 1)
-- Chase confirmed by the server: now the scenario releases the target once.
cancelObs.combat.chase = 1
cancelBrain:decide(cancelObs, function(v) cancelStep = v end, 3000)
eq(cancelStep.action, 'cancel_attack')
eq(cancelBrain.cancelledOnce, true)
-- And only once: a later attack is not cancelled again by the scenario.
cancelBrain.selectedTarget, cancelBrain.phase = 2, 'attack'
cancelBrain.lastSeenAt = 5000
cancelBrain:decide(cancelObs, function(v) cancelStep = v end, 6000)
assert(cancelStep == nil or cancelStep.action ~= 'cancel_attack',
       'scenario cancelled a second time')
-- Low HP cancels when there is distance to break...
cancelObs.player.hp = 60
cancelObs.creatures[2].position = { x = 104, y = 200, z = 7 }
cancelObs.creatureList[1].position = { x = 104, y = 200, z = 7 }
cancelBrain:decide(cancelObs, function(v) cancelStep = v end, 9000)
eq(cancelStep.action, 'cancel_attack')
-- ...but not toe to toe, where letting go only stops the agent hitting back.
local pinnedCancel = A.mockBrain({ crossSessionMemory = false, proveCancel = true })
pinnedCancel.greeted, pinnedCancel.inventoryTried = true, true
pinnedCancel.inventoryUsed, pinnedCancel.bagReopened = true, true
pinnedCancel.cancelledOnce = true          -- the one scenario cancel is spent
pinnedCancel.selectedTarget, pinnedCancel.phase = 2, 'attack'
pinnedCancel.lastSeenAt, pinnedCancel.targetAt = 9000, 9000
local pinnedStep = 'unset'
local toeToToe = validObservation()
toeToToe.player.hp, toeToToe.player.maxHp = 60, 155
toeToToe.attackId, toeToToe.combat.chase = 2, 1   -- creature 2 is at distance 1
pinnedCancel:decide(toeToToe, function(v) pinnedStep = v end, 10000)
assert(pinnedStep == nil or pinnedStep.action ~= 'cancel_attack',
       'released an adjacent attacker while hurt')
-- A brain without proveCancel keeps the MVP behaviour: no scenario cancel.
local plainCancel = A.mockBrain()
plainCancel.greeted, plainCancel.inventoryTried = true, true
plainCancel.inventoryUsed, plainCancel.bagReopened = true, true
plainCancel.selectedTarget, plainCancel.phase = 2, 'attack'
plainCancel.lastSeenAt, plainCancel.targetAt = 0, 0
local plainStep
local healthy = validObservation()
healthy.attackId, healthy.combat.chase = 2, 1
plainCancel:decide(healthy, function(v) plainStep = v end, 1000)
assert(plainStep == nil or plainStep.action ~= 'cancel_attack',
       'MVP brain cancelled a healthy target')

-- --- agent mode is off unless explicitly opted in --------------------------
-- The test process exports neither R33D_AGENT_MODE nor R33D_AGENT, so the
-- module that a human client would load must do nothing at all: no timer, no
-- g_game subscription, no protocol hook.
eq(os.getenv('R33D_AGENT_MODE'), nil)
eq(os.getenv('R33D_AGENT'), nil)
local scheduled, connected, hooked = 0, 0, false
scheduleEvent = function() scheduled = scheduled + 1 end
removeEvent = function() end
connect = function() connected = connected + 1 end
disconnect = function() end
ProtocolGame = { onOpcode = function() return true end }
local untouchedOpcode = ProtocolGame.onOpcode
dofile('agent/real33d2d/modules/real33d_agent/agent_runtime.lua')
Real33DAgentRuntime.init()
eq(scheduled, 0)
eq(connected, 0)
eq(ProtocolGame.onOpcode, untouchedOpcode)
hooked = ProtocolGame.onOpcode ~= untouchedOpcode
eq(hooked, false)
Real33DAgentRuntime.terminate()
eq(scheduled, 0)

print('REAL33D_AGENT_BRIDGE schema, correlation, JSONL and opt-in tests PASS')

-- ===========================================================================
-- REAL33D-AGENT-MEMORY-001
-- ===========================================================================

local M = AgentMemory

-- --- JSON round trip -------------------------------------------------------
-- Memory is written with the same encoder the trace uses and read back by the
-- decoder beside it, so the two have to agree exactly.
local function roundTrip(value)
  local decoded, reason = S.decode(S.encode(value))
  assert(decoded ~= nil or reason == nil, 'decode failed: ' .. tostring(reason))
  return decoded
end
eq(S.decode('7'), 7)
eq(S.decode('-12'), -12)
eq(S.decode('true'), true)
eq(S.decode('"hi"'), 'hi')
eq(S.decode('"a\\nb"'), 'a\nb')
eq(S.decode('"\\u0001"'), '\1')
eq(#S.decode('[]'), 0)
eq(S.decode('{"a":1,"b":[1,2]}').b[2], 2)
eq(S.decode('  {"a" : 1 }  ').a, 1)
eq(select(2, S.decode('{"a":1')), 'expected_separator')
eq(select(2, S.decode('{"a" 1}')), 'expected_colon')
eq(select(2, S.decode('{a:1}')), 'expected_key')
eq(select(2, S.decode('nope')), 'unexpected_token')
eq(select(2, S.decode('{"a":1} trailing')), 'trailing_content')
eq(select(2, S.decode(nil)), 'not_a_string')
local deep = roundTrip({ n = 1, s = 'x', t = true, a = S.array({ 1, 2, 3 }),
                         o = { nested = 'y' } })
eq(deep.n, 1) eq(deep.s, 'x') eq(deep.t, true) eq(deep.a[3], 3)
eq(deep.o.nested, 'y')

-- --- recording from observations only --------------------------------------
local function memoryObservation()
  local o = validObservation()
  o.player.name = 'Test Player B'
  o.player.position = { x = 32137, y = 32245, z = 8 }
  o.tiles = {
    ['32138:32245:8'] = { position = { x = 32138, y = 32245, z = 8 },
                          walkable = true, things = {} },
    ['32137:32246:8'] = { position = { x = 32137, y = 32246, z = 8 },
                          walkable = false, things = {} },
  }
  local rat = { id = 4242, name = 'Rat', monster = true, hpPercent = 100,
                position = { x = 32138, y = 32245, z = 8 } }
  o.creatures = { [4242] = rat }
  o.creatureList = { rat }
  o.chat = { { name = 'Cipfried', mode = 1, text = 'Hello there' } }
  o.containers = { { id = 0, name = 'backpack', capacity = 8, itemCount = 1,
                     sourcePlace = 'carried',
                     items = { { key = 'c:0:0', id = 3031, count = 2,
                                 container = false } } } }
  return o
end

local identityB = { agent = 'mock', world = 'local', character = 'Test Player B' }
local store = M.new(identityB)
local obsA = memoryObservation()
assert(M.observe(store, obsA, 'o-A-000001', 1000) > 0)

-- --- provenance ------------------------------------------------------------
local placeKey = M.bucketKey(32137, 32245, 8)
local place = store:get('places', placeKey)
assert(place, 'the place the agent stood in was not remembered')
eq(place.source, 'direct_observation')
eq(place.first_observed, 1000)
eq(place.last_observed, 1000)
eq(place.first_observation_id, 'o-A-000001')
eq(place.last_observation_id, 'o-A-000001')
eq(place.observations, 1)
eq(place.category, 'places')
-- Every record in every category carries the same provenance, and validates.
for _, category in ipairs(M.categoryNames()) do
  for _, record in ipairs(store:recall(category)) do
    local checked, reason = M.checkRecord(record)
    assert(checked, category .. '/' .. record.key .. ': ' .. tostring(reason))
    eq(record.source, 'direct_observation')
    assert(record.first_observation_id:sub(1, 2) == 'o-', 'provenance lost')
  end
end
assert(#store:recall('creature_sightings') > 0, 'sighting not remembered')
assert(#store:recall('conversations') > 0, 'conversation not remembered')
assert(#store:recall('item_observations') > 0, 'item not remembered')
assert(#store:recall('encounters') >= 0)

-- --- duplicate observation merging -----------------------------------------
M.observe(store, memoryObservation(), 'o-A-000002', 2000)
local merged = store:get('places', placeKey)
eq(merged.observations, 2)
eq(merged.first_observed, 1000)          -- kept
eq(merged.last_observed, 2000)           -- advanced
eq(merged.first_observation_id, 'o-A-000001')
eq(merged.last_observation_id, 'o-A-000002')
eq(store:count('places'), 1)             -- merged, not duplicated
local sightings = store:recall('creature_sightings')
eq(#sightings, 1)
eq(sightings[1].observations, 2)

-- --- memory never records an unobserved tile -------------------------------
-- Every position held in memory has to trace back to a position that appeared
-- in an observation: the agent's own, or a creature it could see.
local legitimate = {}
for _, o in ipairs({ obsA }) do
  legitimate[o.player.position.x .. ':' .. o.player.position.y] = true
  for _, c in ipairs(o.creatureList) do
    legitimate[c.position.x .. ':' .. c.position.y] = true
  end
  for _, row in pairs(o.tiles) do
    legitimate[row.position.x .. ':' .. row.position.y] = true
  end
end
for _, category in ipairs(M.categoryNames()) do
  for _, record in ipairs(store:recall(category)) do
    if record.x then
      local found = false
      for seen in pairs(legitimate) do
        local sx, sy = seen:match('(%-?%d+):(%-?%d+)')
        local bx, by = M.bucket(tonumber(sx), tonumber(sy), 0)
        if bx == record.x and by == record.y then found = true break end
      end
      assert(found, 'memory holds a position never observed: ' .. record.key)
    end
  end
end

-- Memory holds no creature id at all, so a stale id cannot even be expressed.
for _, record in ipairs(store:recall('creature_sightings')) do
  eq(record.id, nil)
  eq(record.creatureId, nil)
end

-- --- save / load and session restart ---------------------------------------
store.sessions = 1
local serialized = store:serialize(2000)
eq(serialized:find('\n'), nil)                        -- one diffable document
contains(serialized, '"schema":"real33d.agent.memory/1"')
contains(serialized, '"source":"direct_observation"')
local reloaded, loadReason = M.load(serialized, identityB)
assert(reloaded, 'reload failed: ' .. tostring(loadReason))
eq(reloaded:total(), store:total())
eq(reloaded:get('places', placeKey).observations, 2)
eq(reloaded:get('places', placeKey).first_observation_id, 'o-A-000001')
eq(reloaded.sessions, 1)
-- Session restart: the reloaded store keeps counting and keeps its history.
reloaded.sessions = reloaded.sessions + 1
M.observe(reloaded, memoryObservation(), 'o-B-000001', 5000)
eq(reloaded.sessions, 2)
eq(reloaded:get('places', placeKey).observations, 3)
eq(reloaded:get('places', placeKey).first_observed, 1000)   -- survived the restart
eq(reloaded:get('places', placeKey).first_observation_id, 'o-A-000001')
-- and the round trip is stable: saving what was loaded reproduces the file.
eq(M.load(reloaded:serialize(5000), identityB):total(), reloaded:total())

-- --- memory isolation between characters -----------------------------------
local identityA = { agent = 'mock', world = 'local', character = 'Test Player A' }
assert(M.identityKey(identityA) ~= M.identityKey(identityB))
local spaced = { agent = 'mock', world = 'local', character = 'Test Player B' }
local underscored = { agent = 'mock', world = 'local', character = 'Test_Player_B' }
assert(M.identityKey(spaced) ~= M.identityKey(underscored),
       'distinct character names must never share a memory path')
eq(select(2, M.load(serialized, underscored)), 'memory_identity_mismatch')
eq(select(2, M.load(serialized, identityA)), 'memory_identity_mismatch')
-- Same character name on another world is also a different recollection.
eq(select(2, M.load(serialized, { agent = 'mock', world = 'other',
                                  character = 'Test Player B' })),
   'memory_identity_mismatch')
eq(select(2, M.load(serialized, { agent = 'other', world = 'local',
                                  character = 'Test Player B' })),
   'memory_identity_mismatch')
local storeA = M.new(identityA)
eq(storeA:total(), 0)                     -- a second character starts blank
M.observe(storeA, memoryObservation(), 'o-A2-000001', 1000)
eq(store:get('places', placeKey).observations, 2)  -- untouched by the other store

-- --- invalid memory injection ----------------------------------------------
eq(select(2, M.load('not json', identityB)):sub(1, 17), 'memory_unreadable')
eq(select(2, M.load('[]', identityB)), 'memory_bad_schema')
eq(select(2, M.load(S.encode({ schema = 'other' }), identityB)), 'memory_bad_schema')
eq(select(2, M.load(S.encode({ schema = M.SCHEMA, agent = 'mock', world = 'local',
                               character = 'Test Player B' }), identityB)),
   'memory_missing_entries')

local function injected(record, category)
  return S.encode({
    schema = M.SCHEMA, agent = 'mock', world = 'local',
    character = 'Test Player B', sessions = 1, updated = 0,
    entries = { [category or 'places'] = S.array({ record }) },
  })
end
local function goodRecord(overrides)
  local record = { key = 'k', category = 'places', source = 'direct_observation',
                   first_observed = 1, last_observed = 2,
                   first_observation_id = 'o-1', last_observation_id = 'o-2',
                   observations = 1, x = 0, y = 0, z = 7,
                   walkable = 1, sightings = 0, visits = 1 }
  for name, value in pairs(overrides or {}) do
    if value == '__nil__' then record[name] = nil else record[name] = value end
  end
  return record
end
assert(M.load(injected(goodRecord()), identityB), 'a good record should load')
-- A record claiming any origin other than the agent's own eyes is refused.
for _, forged in ipairs({ 'server_database', 'spawn_file', 'wiki', 'minimap',
                          'another_player', 'imported' }) do
  eq(select(2, M.load(injected(goodRecord({ source = forged })), identityB)),
     'record_bad_source')
end
eq(select(2, M.load(injected(goodRecord({ source = '__nil__' })), identityB)),
   'record_bad_source')
-- Unknown categories and unknown fields do not sneak in beside good ones.
eq(select(2, M.load(injected(goodRecord({ category = 'spawns' }), 'spawns'),
                    identityB)), 'memory_unknown_category:spawns')
eq(select(2, M.load(injected(goodRecord({ full_map = true })), identityB)),
   'record_unknown_field:full_map')
eq(select(2, M.load(injected(goodRecord({ creatureId = 4242 })), identityB)),
   'record_unknown_field:creatureId')
-- Provenance is mandatory and internally consistent.
eq(select(2, M.load(injected(goodRecord({ first_observation_id = '__nil__' })),
                    identityB)), 'record_bad_provenance:first_observation_id')
eq(select(2, M.load(injected(goodRecord({ last_observed = '__nil__' })), identityB)),
   'record_bad_provenance:last_observed')
eq(select(2, M.load(injected(goodRecord({ first_observed = 9, last_observed = 2 })),
                    identityB)), 'record_provenance_order')
eq(select(2, M.load(injected(goodRecord({ observations = 0 })), identityB)),
   'record_provenance_count')
eq(select(2, M.load(injected(goodRecord({ x = 1.5 })), identityB)),
   'record_bad_position')
eq(select(2, M.load(injected(goodRecord({ z = '__nil__' })), identityB)),
   'record_missing_position:z')
-- Known fields also need usable values; a malformed count previously loaded
-- and crashed the next observation when the agent incremented it.
eq(select(2, M.load(injected(goodRecord({ sightings = 'many' })), identityB)),
   'record_bad_field:sightings')
eq(select(2, M.load(injected(goodRecord({ walkable = -1 })), identityB)),
   'record_bad_field:walkable')
eq(select(2, M.load(injected(goodRecord({ visits = 1.5 })), identityB)),
   'record_bad_field:visits')
eq(select(2, M.load(injected(goodRecord({
     category = 'routes', walkable = '__nil__', sightings = '__nil__',
     visits = '__nil__', from = 'A', to = 'B', steps = 'many' }), 'routes'),
   identityB)), 'record_bad_field:steps')
local original = store:get('places', placeKey)
local originalCount, originalTime = original.sightings, original.last_observed
local failed, failedReason = store:remember('places', placeKey,
  { sightings = 'many' }, 'o-bad', 9000)
eq(failed, nil)
eq(failedReason, 'record_bad_field:sightings')
eq(store:get('places', placeKey), original)
eq(original.sightings, originalCount)
eq(original.last_observed, originalTime)
-- Field names alone are not enough: a known field carrying the wrong kind of
-- value is refused too, so a plausible-looking record cannot smuggle rubbish
-- past the loader.
eq(select(2, M.load(injected(goodRecord({ sightings = 'many' })), identityB)),
   'record_bad_field:sightings')
eq(select(2, M.load(injected(goodRecord({ visits = -1 })), identityB)),
   'record_bad_field:visits')
eq(select(2, M.load(injected(goodRecord({ walkable = 1.5 })), identityB)),
   'record_bad_field:walkable')
local sighting = { key = 'Rat@0:0:7', category = 'creature_sightings',
                   source = 'direct_observation', first_observed = 1,
                   last_observed = 2, first_observation_id = 'o-1',
                   last_observation_id = 'o-2', observations = 1,
                   name = 'Rat', kind = 'monster', hp_percent = 100 }
assert(M.load(injected(sighting, 'creature_sightings'), identityB))
sighting.hp_percent = 500
eq(select(2, M.load(injected(sighting, 'creature_sightings'), identityB)),
   'record_bad_field:hp_percent')
sighting.hp_percent, sighting.name = 100, 42
eq(select(2, M.load(injected(sighting, 'creature_sightings'), identityB)),
   'record_bad_field:name')

-- A record filed under the wrong category is rejected rather than re-filed.
-- Which of its now-foreign fields is named depends on table order, so only the
-- kind of rejection is asserted.
eq(select(2, M.load(injected(goodRecord({ category = 'routes' })), identityB))
     :sub(1, 21), 'record_unknown_field:')

-- --- stale memory ----------------------------------------------------------
local aged = M.new(identityB)
aged:remember('places', 'old', { walkable = 1, sightings = 5, visits = 1,
                                 x = 100, y = 100, z = 7 }, 'o-1', 1000)
aged:remember('places', 'new', { walkable = 1, sightings = 5, visits = 1,
                                 x = 120, y = 100, z = 7 }, 'o-2', 90000)
eq(#aged:fresh('places', 100000, nil), 2)        -- no age limit: everything
eq(#aged:fresh('places', 100000, 20000), 1)      -- limit: only the recent one
eq(aged:fresh('places', 100000, 20000)[1].key, 'new')
eq(#aged:fresh('places', 100000, 1), 0)
-- Planning honours the age limit too, so the agent does not cross the map for
-- something it saw once, long ago.
eq(aged:huntingGround(200, 200, 7, 100000, nil).key, 'new')
eq(aged:huntingGround(200, 200, 7, 100000, 20000).key, 'new')
eq(aged:huntingGround(200, 200, 7, 100000, 1), nil)
-- Danger weighs on the choice but does not veto it: a healthy agent still goes
-- to the only hunting ground it knows, because every hunting ground is
-- somewhere it has been hurt.
aged:remember('dangers', 'new', { cause = 'low_health', hp = 5, max_hp = 160,
                                  x = 120, y = 100, z = 7 }, 'o-3', 90000)
eq(aged:huntingGround(200, 200, 7, 100000, 20000).key, 'new')
-- A cautious agent -- one already hurt -- avoids it outright.
eq(aged:huntingGround(200, 200, 7, 100000, 20000, true), nil)
-- Given a safe alternative of similar value, the safe one wins.
aged:remember('places', 'safe', { walkable = 1, sightings = 5, visits = 1,
                                  x = 121, y = 100, z = 7 }, 'o-4', 90000)
eq(aged:huntingGround(200, 200, 7, 100000, 20000).key, 'safe')

-- A remembered cave cannot be reached by walking toward its x/y coordinates
-- from the surface. A cross-floor goal needs an observed route from here.
local floors = M.new(identityB)
local surface = M.bucketKey(200, 200, 7)
local cave = M.bucketKey(205, 200, 8)
floors:remember('places', cave,
  { walkable = 3, sightings = 10, visits = 1, x = 205, y = 200, z = 8 },
  'o-cave', 100)
eq(floors:huntingGround(200, 200, 7, 110, 1000), nil)
floors:remember('routes', surface .. '>' .. cave,
  { from = surface, to = cave, steps = 1 }, 'o-stairs', 105)
eq(floors:huntingGround(200, 200, 7, 110, 1000).key, cave)

-- --- cornered between two monsters -----------------------------------------
-- Reproduces an observed live stall: two rats either side, the agent backing
-- away from whichever was nearest and walking into the other, flipping between
-- two tiles until it was at 6/160. Retreat must consider every threat.
local function pinnedObs(hp)
  local o = validObservation()
  o.player.hp, o.player.maxHp = hp, 160
  o.player.position = { x = 100, y = 200, z = 7 }
  local west = { id = 11, name = 'Rat', monster = true, hpPercent = 100,
                 position = { x = 99, y = 200, z = 7 } }
  local east = { id = 12, name = 'Rat', monster = true, hpPercent = 100,
                 position = { x = 101, y = 200, z = 7 } }
  o.creatures = { [11] = west, [12] = east }
  o.creatureList = { west, east }
  o.tiles = {
    ['99:200:7']  = { position = { x = 99,  y = 200, z = 7 }, walkable = true, things = {} },
    ['101:200:7'] = { position = { x = 101, y = 200, z = 7 }, walkable = true, things = {} },
  }
  return o
end
local function pinnedBrain()
  local b = A.mockBrain({ crossSessionMemory = false })
  b.greeted, b.inventoryTried, b.inventoryUsed, b.bagReopened = true, true, true, true
  return b
end

-- Nowhere improves: both neighbours are a rat. Fight rather than shuffle.
local cornered, corneredStep = pinnedBrain(), nil
cornered:decide(pinnedObs(40), function(v) corneredStep = v end, 0)
eq(corneredStep.action, 'attack')
assert(corneredStep.creatureId == 11 or corneredStep.creatureId == 12)
eq(A.validate(corneredStep, pinnedObs(40)).action, 'attack')

-- With a genuine way out, take it, and do not step back where it came from.
local escaping = pinnedBrain()
local openObs = pinnedObs(40)
openObs.tiles['100:199:7'] = { position = { x = 100, y = 199, z = 7 },
                               walkable = true, things = {} }
local escapeStep
escaping:decide(openObs, function(v) escapeStep = v end, 0)
eq(escapeStep.action, 'move')
eq(escapeStep.direction, 0)              -- north, away from both rats
eq(escaping.retreatFrom, '100:200:7')

-- The dead zone: hurt but above the retreat threshold, with a rat adjacent.
-- It used to neither fight nor flee and simply absorbed the damage.
local hurt, hurtStep = pinnedBrain(), nil
hurt:decide(pinnedObs(100), function(v) hurtStep = v end, 0)  -- 62% of 160
eq(hurtStep.action, 'attack')
-- Healthy and unthreatened, nothing about the above changes ordinary play.
local healthyPinned = pinnedBrain()
local calm = pinnedObs(160)
calm.creatures, calm.creatureList = {}, {}
local calmStep
healthyPinned:decide(calm, function(v) calmStep = v end, 0)
eq(calmStep.action, 'move')

-- --- routes are walked adjacencies, not a map ------------------------------
local routed = M.new(identityB)
eq(M.bucketCentre('100:200:7'), 102)
eq(select(2, M.bucketCentre('100:200:7')), 202)
eq(M.parseBucketKey('bad'), nil)
eq(M.bucketCentre('bad'), nil)
-- A > B > C, walked in that order. Asking to get from A to C names B.
routed:remember('routes', 'A>B', { from = 'A', to = 'B', steps = 1 }, 'o-1', 10)
routed:remember('routes', 'B>C', { from = 'B', to = 'C', steps = 1 }, 'o-2', 20)
eq(routed:routeTo('A', 'C'), 'B')
eq(routed:routeTo('A', 'B'), 'B')
eq(routed:routeTo('A', 'A'), nil)
-- Somewhere it never walked to is not reachable by recollection.
eq(routed:routeTo('A', 'Z'), nil)
eq(routed:routeTo('Q', 'C'), nil)
-- The graph is directed: it only remembers the way it actually went.
eq(routed:routeTo('C', 'A'), nil)
routed:remember('routes', 'C>B', { from = 'C', to = 'B', steps = 1 }, 'o-3', 30)
routed:remember('routes', 'B>A', { from = 'B', to = 'A', steps = 1 }, 'o-4', 40)
eq(routed:routeTo('C', 'A'), 'B')

-- --- remembered targets still require current visibility -------------------
-- The whole point of the milestone. A creature the agent remembers seeing is
-- not a creature it may attack.
local rememberedObs = validObservation()
rememberedObs.creatures, rememberedObs.creatureList = {}, {}   -- nothing visible now
local recalled = store:recall('creature_sightings')[1]
assert(recalled, 'expected a remembered sighting')
eq(recalled.name, 'Rat')
-- The id 4242 was visible in session A and is in no memory record; even handed
-- to the validator directly it is refused against the current observation.
eq(select(2, A.validate({ action = 'attack', creatureId = 4242 }, rememberedObs)),
   'creature_not_visible')
eq(select(2, A.validate({ action = 'follow', creatureId = 4242 }, rememberedObs)),
   'creature_not_visible')
-- Standing on the remembered tile changes nothing: visibility is the authority.
rememberedObs.player.position = { x = recalled.x, y = recalled.y, z = recalled.z }
eq(select(2, A.validate({ action = 'attack', creatureId = 4242 }, rememberedObs)),
   'creature_not_visible')
-- A remembered item is likewise not a usable item.
eq(select(2, A.validate({ action = 'use', item = 'c:0:0' }, rememberedObs)),
   'item_not_visible')
-- And a remembered place is not a walkable tile.
local blocked = validObservation()
blocked.tiles = {}
eq(select(2, A.validate({ action = 'move', direction = 1 }, blocked)),
   'tile_not_visible_walkable')

-- --- memory guides navigation, observation authorises each step ------------
local navMemory = M.new(identityB)
navMemory:remember('places', 'goal', { walkable = 9, sightings = 20, visits = 3,
                                       x = 130, y = 200, z = 7 }, 'o-1', 500)
navMemory.wallClock = 1000
local navBrain = A.mockBrain({ crossSessionMemory = false, memory = navMemory })
navBrain.greeted, navBrain.inventoryTried = true, true
navBrain.inventoryUsed, navBrain.bagReopened = true, true
local navObs = validObservation()
navObs.creatures, navObs.creatureList = {}, {}
navObs.player.position = { x = 100, y = 200, z = 7 }
-- East moves toward the remembered place; west moves away. Both are visible.
navObs.tiles = {
  ['101:200:7'] = { position = { x = 101, y = 200, z = 7 }, walkable = true, things = {} },
  ['99:200:7']  = { position = { x = 99,  y = 200, z = 7 }, walkable = true, things = {} },
}
local navStep
navBrain:decide(navObs, function(v) navStep = v end, 0)
eq(navStep.action, 'move')
eq(navStep.direction, 1)                  -- east, toward the recollection
eq(navBrain.navigatingTo, 'goal')
eq(A.validate(navStep, navObs).action, 'move')
-- Take away the tile that leads there and the agent does not walk through a
-- wall on the strength of a memory: it falls back to ordinary exploration.
navObs.tiles['101:200:7'] = nil
navBrain:decide(navObs, function(v) navStep = v end, 3000)
eq(navStep.direction, 3)                  -- the only legal step
eq(navBrain.navigatingTo, nil)
-- With no legal step at all it proposes nothing rather than inventing one.
navObs.tiles = {}
navStep = 'unset'
navBrain:decide(navObs, function(v) navStep = v end, 6000)
eq(navStep, nil)
-- Once standing in the remembered place there is nothing left to navigate to.
navObs.tiles = { ['131:200:7'] = { position = { x = 131, y = 200, z = 7 },
                                   walkable = true, things = {} } }
navObs.player.position = { x = 130, y = 200, z = 7 }
navBrain:decide(navObs, function(v) navStep = v end, 9000)
eq(navBrain.navigatingTo, nil)

-- --- a brain with no memory behaves exactly as certified in BRIDGE-001 ------
local blindBrain = A.mockBrain({ crossSessionMemory = false })
eq(blindBrain.memory, nil)
local blindObs = validObservation()
blindObs.creatures, blindObs.creatureList = {}, {}
blindBrain.greeted, blindBrain.inventoryTried = true, true
blindBrain.inventoryUsed, blindBrain.bagReopened = true, true
local blindStep
blindBrain:decide(blindObs, function(v) blindStep = v end, 0)
eq(blindStep.action, 'move')

-- --- deleting memory does not touch authoritative state --------------------
-- Forgetting is only forgetting: the observation and every validator verdict
-- computed from it are identical before and after.
local beforeObs = S.encode(S.projectObservation(navObs))
local verdicts = {}
local probes = {
  { action = 'move', direction = 1 }, { action = 'move', direction = 3 },
  { action = 'attack', creatureId = 4242 }, { action = 'use', item = 'c:0:0' },
  { action = 'say', text = 'hello' },
}
for i, probe in ipairs(probes) do
  local okIntent, why = A.validate(probe, navObs)
  verdicts[i] = okIntent and 'ok' or why
end
navMemory.entries = {}
for _, name in ipairs(M.categoryNames()) do navMemory.entries[name] = {} end
eq(navMemory:total(), 0)
eq(S.encode(S.projectObservation(navObs)), beforeObs)
for i, probe in ipairs(probes) do
  local okIntent, why = A.validate(probe, navObs)
  eq(okIntent and 'ok' or why, verdicts[i])
end
-- And the observation schema still refuses to carry memory into the world view.
local leaky = validObservation()
leaky.memory = { places = {} }
eq(select(2, S.checkObservation(leaky)), 'schema_forbidden_field:memory')

print('REAL33D_AGENT_MEMORY persistence, provenance, isolation and fair-play tests PASS')
