-- REAL33D-AGENT-BRIDGE-001 formal schemas.
--
-- This file owns three things and nothing else:
--   * AgentObservation  - what the bridge is allowed to publish about the game
--   * AgentIntent       - what a Brain is allowed to ask for
--   * a deterministic JSON encoder used by the JSONL trace
--
-- It is pure data and pure functions. It never touches g_game, g_map, the
-- protocol, the filesystem or any Fusion32 state, so it runs unchanged under a
-- bare LuaJIT for the test suite.
AgentSchema = {}

AgentSchema.OBSERVATION_SCHEMA = 'real33d.agent.observation/1'
AgentSchema.INTENT_SCHEMA = 'real33d.agent.intent/1'
AgentSchema.EVENT_SCHEMA = 'real33d.agent.event/1'

-- ---------------------------------------------------------------------------
-- Deterministic JSON
-- ---------------------------------------------------------------------------
-- Object keys are emitted in sorted order so two runs over equal state produce
-- byte-identical lines. That is what makes a JSONL trace diffable evidence.

local ARRAY_MT = {}
local OBJECT_MT = {}

-- Force a table to encode as [] / {} regardless of how its keys happen to look.
function AgentSchema.array(t) return setmetatable(t or {}, ARRAY_MT) end
function AgentSchema.object(t) return setmetatable(t or {}, OBJECT_MT) end

local function looksLikeArray(t)
  local mt = getmetatable(t)
  if mt == ARRAY_MT then return true end
  if mt == OBJECT_MT then return false end
  local count = 0
  for key in pairs(t) do
    if type(key) ~= 'number' or key < 1 or key ~= math.floor(key) then return false end
    count = count + 1
  end
  return count > 0 and count == #t
end

local ESCAPES = { ['"'] = '\\"', ['\\'] = '\\\\', ['\b'] = '\\b', ['\f'] = '\\f',
                  ['\n'] = '\\n', ['\r'] = '\\r', ['\t'] = '\\t' }

local function encodeString(s)
  return '"' .. s:gsub('[%z\1-\31\\"]', function(c)
    return ESCAPES[c] or string.format('\\u%04x', c:byte())
  end) .. '"'
end

local function encodeNumber(n)
  -- A trace line must never carry nan/inf: it would not parse as JSON.
  if n ~= n or n == math.huge or n == -math.huge then return 'null' end
  if n == math.floor(n) and math.abs(n) < 2 ^ 53 then return string.format('%d', n) end
  return string.format('%.14g', n)
end

function AgentSchema.encode(value, depth)
  depth = (depth or 0) + 1
  if depth > 16 then return '"__depth_limit__"' end
  local kind = type(value)
  if value == nil then return 'null' end
  if kind == 'boolean' then return value and 'true' or 'false' end
  if kind == 'number' then return encodeNumber(value) end
  if kind == 'string' then return encodeString(value) end
  if kind ~= 'table' then return 'null' end
  if looksLikeArray(value) then
    local parts = {}
    for i = 1, #value do parts[i] = AgentSchema.encode(value[i], depth) end
    return '[' .. table.concat(parts, ',') .. ']'
  end
  local keys, byName = {}, {}
  for key in pairs(value) do
    local name = tostring(key)
    keys[#keys + 1] = name
    byName[name] = key
  end
  table.sort(keys)
  local parts = {}
  for i = 1, #keys do
    parts[i] = encodeString(keys[i]) .. ':' .. AgentSchema.encode(value[byName[keys[i]]], depth)
  end
  return '{' .. table.concat(parts, ',') .. '}'
end

-- ---------------------------------------------------------------------------
-- Primitive checks
-- ---------------------------------------------------------------------------

local function isInteger(v)
  return type(v) == 'number' and v == math.floor(v) and v ~= math.huge and v ~= -math.huge
end

local function isPosition(v)
  return type(v) == 'table' and isInteger(v.x) and isInteger(v.y) and isInteger(v.z)
end

-- ---------------------------------------------------------------------------
-- AgentIntent
-- ---------------------------------------------------------------------------
-- Declarative and closed. An action that is not listed here cannot be
-- expressed, and a field that is not listed on its action is a hard reject
-- rather than something silently dropped: a Brain that sends junk must be told.

AgentSchema.INTENTS = {
  move           = { direction = { type = 'integer', required = true, min = 0, max = 3 } },
  say            = { text = { type = 'string', required = true, minLength = 1, maxLength = 100 } },
  attack         = { creatureId = { type = 'integer', required = true, min = 1 } },
  follow         = { creatureId = { type = 'integer', required = true, min = 1 } },
  cancel_attack  = {},
  cancel_follow  = {},
  use            = { item = { type = 'string', required = true, minLength = 1, maxLength = 64 } },
  open_container = { item = { type = 'string', required = true, minLength = 1, maxLength = 64 } },
  use_with       = { item = { type = 'string', required = true, minLength = 1, maxLength = 64 },
                     targetItem = { type = 'string', minLength = 1, maxLength = 64 },
                     targetCreatureId = { type = 'integer', min = 1 } },
  move_item      = { item = { type = 'string', required = true, minLength = 1, maxLength = 64 },
                     destination = { type = 'string', required = true, minLength = 1, maxLength = 64 },
                     count = { type = 'integer', required = true, min = 1, max = 100 } },
  combat_mode    = { fight = { type = 'integer', required = true, min = 1, max = 3 },
                     chase = { type = 'integer', required = true, min = 0, max = 1 },
                     safe  = { type = 'boolean', required = true } },
}

-- The legal action list, sorted, for documentation and for the READY event.
function AgentSchema.actions()
  local names = {}
  for name in pairs(AgentSchema.INTENTS) do names[#names + 1] = name end
  table.sort(names)
  return names
end

local function checkField(name, rule, value)
  if value == nil then
    if rule.required then return 'schema_missing_field:' .. name end
    return nil
  end
  if rule.type == 'integer' then
    if not isInteger(value) then return 'schema_type:' .. name end
    if rule.min and value < rule.min then return 'schema_range:' .. name end
    if rule.max and value > rule.max then return 'schema_range:' .. name end
  elseif rule.type == 'string' then
    if type(value) ~= 'string' then return 'schema_type:' .. name end
    if rule.minLength and #value < rule.minLength then return 'schema_length:' .. name end
    if rule.maxLength and #value > rule.maxLength then return 'schema_length:' .. name end
    -- Control characters would let a Brain forge extra chat lines or corrupt a
    -- trace line, so they are rejected everywhere, not only in say().
    if value:find('[%z\1-\31]') then return 'schema_control_character:' .. name end
  elseif rule.type == 'boolean' then
    if type(value) ~= 'boolean' then return 'schema_type:' .. name end
  end
  return nil
end

-- Returns a normalized copy carrying only schema fields, or nil plus a reason.
function AgentSchema.checkIntent(intent)
  if type(intent) ~= 'table' then return nil, 'schema_intent_not_table' end
  local action = intent.action
  if type(action) ~= 'string' then return nil, 'schema_missing_action' end
  local fields = AgentSchema.INTENTS[action]
  if not fields then return nil, 'schema_unknown_action' end
  for key in pairs(intent) do
    if key ~= 'action' and not fields[key] then
      return nil, 'schema_unknown_field:' .. tostring(key)
    end
  end
  local normalized = { action = action }
  for name, rule in pairs(fields) do
    local reason = checkField(name, rule, intent[name])
    if reason then return nil, reason end
    normalized[name] = intent[name]
  end
  if action == 'use_with' then
    local hasItem = normalized.targetItem ~= nil
    local hasCreature = normalized.targetCreatureId ~= nil
    if hasItem == hasCreature then return nil, 'schema_use_with_one_target' end
  end
  return normalized, nil
end

-- ---------------------------------------------------------------------------
-- AgentObservation
-- ---------------------------------------------------------------------------
-- The observation is a closed whitelist. Every key the bridge may publish is
-- named here; anything else is a reject. That is the fair-play boundary
-- expressed as code rather than as a comment, so a later change that starts
-- copying the full map, the minimap cache, spawn data or another player's
-- inventory into the observation fails the schema instead of shipping.

AgentSchema.OBSERVATION_FIELDS = {
  online = true, player = true, tiles = true, creatures = true, creatureList = true,
  items = true, destinations = true, inventory = true, inventoryKeys = true,
  containers = true, chat = true, combat = true, attackId = true, followId = true,
}

AgentSchema.OBSERVATION_REQUIRED = {
  'player', 'tiles', 'creatures', 'creatureList', 'items', 'destinations',
  'inventory', 'inventoryKeys', 'containers', 'chat', 'combat',
}

function AgentSchema.checkObservation(obs)
  if type(obs) ~= 'table' then return nil, 'schema_observation_not_table' end
  if type(obs.online) ~= 'boolean' then return nil, 'schema_missing_field:online' end
  for key in pairs(obs) do
    if not AgentSchema.OBSERVATION_FIELDS[key] then
      return nil, 'schema_forbidden_field:' .. tostring(key)
    end
  end
  if not obs.online then return obs, nil end
  for _, name in ipairs(AgentSchema.OBSERVATION_REQUIRED) do
    if type(obs[name]) ~= 'table' then return nil, 'schema_missing_field:' .. name end
  end
  local player = obs.player
  if not isInteger(player.id) or not isPosition(player.position) then
    return nil, 'schema_player_identity'
  end
  if not isInteger(player.hp) or not isInteger(player.maxHp) or player.maxHp < 1 then
    return nil, 'schema_player_health'
  end
  local combat = obs.combat
  if not isInteger(combat.fight) or not isInteger(combat.chase)
     or type(combat.safe) ~= 'boolean' then
    return nil, 'schema_combat_mode'
  end
  if obs.attackId ~= nil and not isInteger(obs.attackId) then return nil, 'schema_type:attackId' end
  if obs.followId ~= nil and not isInteger(obs.followId) then return nil, 'schema_type:followId' end
  return obs, nil
end

-- ---------------------------------------------------------------------------
-- Published projection
-- ---------------------------------------------------------------------------
-- The trace and the Brain contract see this grouped view. The lookup maps the
-- validator and dispatcher need (items/destinations/creatures by key) stay on
-- the internal record; they are derived from the same visible state, but they
-- are indexes, not new knowledge.
--
-- Every collection below is built as an explicit sorted array. Number-keyed
-- Lua maps would encode inconsistently as array or object depending on which
-- slots happened to be filled, which would make the trace non-deterministic.

local function sortedKeys(t)
  local keys = {}
  for key in pairs(t) do keys[#keys + 1] = key end
  table.sort(keys)
  return keys
end

function AgentSchema.projectObservation(obs)
  if not obs or not obs.online then
    return { online = false, schema = AgentSchema.OBSERVATION_SCHEMA }
  end
  local player = obs.player

  local skills = AgentSchema.array({})
  for _, skill in ipairs(sortedKeys(player.skills or {})) do
    skills[#skills + 1] = { skill = skill, level = player.skills[skill] }
  end

  local tiles = AgentSchema.array({})
  for _, key in ipairs(sortedKeys(obs.tiles)) do
    local row = obs.tiles[key]
    local things = AgentSchema.array({})
    for _, thing in ipairs(row.things or {}) do
      things[#things + 1] = { key = thing.key, id = thing.id, count = thing.count,
                              container = thing.container }
    end
    tiles[#tiles + 1] = { x = row.position.x, y = row.position.y, z = row.position.z,
                          walkable = row.walkable, things = things }
  end

  local creatures = AgentSchema.array({})
  for _, creature in ipairs(obs.creatureList) do
    creatures[#creatures + 1] = {
      id = creature.id, name = creature.name, hpPercent = creature.hpPercent,
      x = creature.position.x, y = creature.position.y, z = creature.position.z,
      kind = creature.monster and 'monster' or creature.npc and 'npc'
             or creature.player and 'player' or 'other',
    }
  end

  local equipment = AgentSchema.array({})
  for _, slot in ipairs(sortedKeys(obs.inventory)) do
    local item = obs.inventory[slot]
    equipment[#equipment + 1] = { slot = slot, key = AgentSchema.inventoryKey(slot),
                                  id = item.id, count = item.count,
                                  container = item.container }
  end

  local containers = AgentSchema.array({})
  for _, container in ipairs(obs.containers) do
    local items = AgentSchema.array({})
    for _, item in ipairs(container.items) do
      items[#items + 1] = { key = item.key, id = item.id, count = item.count,
                            container = item.container }
    end
    containers[#containers + 1] = { id = container.id, name = container.name,
                                    capacity = container.capacity,
                                    itemCount = container.itemCount,
                                    source = container.sourcePlace, items = items }
  end

  local chat = AgentSchema.array({})
  for _, line in ipairs(obs.chat) do
    chat[#chat + 1] = { from = line.name, mode = line.mode, text = line.text }
  end

  return {
    schema = AgentSchema.OBSERVATION_SCHEMA,
    online = true,
    self = {
      id = player.id, name = player.name,
      x = player.position.x, y = player.position.y, z = player.position.z,
      hp = player.hp, maxHp = player.maxHp, mana = player.mana, maxMana = player.maxMana,
      level = player.level, magicLevel = player.magicLevel, skills = skills,
      combat = { fight = obs.combat.fight, chase = obs.combat.chase, safe = obs.combat.safe },
      attackId = obs.attackId, followId = obs.followId,
    },
    visible = { tiles = tiles, creatures = creatures },
    owned = { equipment = equipment, containers = containers },
    social = { chat = chat },
  }
end

function AgentSchema.inventoryKey(slot) return 'i:' .. tostring(slot) end

return AgentSchema
