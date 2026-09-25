-- Run with REAL33D2D's built LuaJIT: luajit tests/real33d_agent_test.lua
--
-- Sections:
--   1. REAL33D-AGENT-MVP-001 policy, budget, brain, loot/eat, viewport (preserved)
--   2. REAL33D-AGENT-BRIDGE-001 schemas, correlation ids, JSONL, opt-in default
dofile('agent/real33d2d/modules/real33d_agent/agent_schema.lua')
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
eq(#S.actions(), 11)
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
bridge:emitDispatch(cycle, intent, true, nil, Real33DAgentBridge.snapshot(obs1))
contains(lines[4], '"event":"dispatch"')
contains(lines[4], '"path":"g_game.move"')

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
quiet:emitDispatch(q, intent, true, nil, Real33DAgentBridge.snapshot(obs1))
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
-- Low HP still cancels regardless of the one-shot scenario step.
cancelObs.player.hp = 60
cancelBrain:decide(cancelObs, function(v) cancelStep = v end, 9000)
eq(cancelStep.action, 'cancel_attack')
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
