-- REAL33D-AGENT-MEMORY-001: persistent recollection of what the agent itself saw.
--
-- The one rule this file exists to enforce:
--
--   Memory is never world truth.
--
-- AgentMemory is kept in its own table and is never merged into an
-- AgentObservation. A Brain may consult it to decide *where to go* or *what to
-- try*, but every action still passes the ordinary validator against the
-- current observation. A remembered creature id, item key or tile is therefore
-- useless as a target unless the client can see it right now. Nothing in here
-- can widen the fair-play boundary, because nothing in here is ever consulted
-- by the validator.
--
-- Everything stored is derived from an AgentObservation the agent received
-- while playing. There is no importer, and `source` is a closed set with one
-- member, so a hand-edited file claiming another origin fails to load.
--
-- Pure Lua: no g_game, no filesystem. The runtime owns reading and writing.
AgentMemory = {}

AgentMemory.SCHEMA = 'real33d.agent.memory/1'
AgentMemory.SOURCE = 'direct_observation'

-- Positions are remembered in buckets rather than exact tiles. A creature that
-- walks two steps is the same sighting, and "rats live around here" is the
-- useful recollection; an exact tile would be both noisier and a finer-grained
-- record than the agent can honestly claim.
AgentMemory.BUCKET = 5

function AgentMemory.bucket(x, y, z)
  local size = AgentMemory.BUCKET
  return math.floor(x / size) * size, math.floor(y / size) * size, z
end

function AgentMemory.bucketKey(x, y, z)
  local bx, by, bz = AgentMemory.bucket(x, y, z)
  return bx .. ':' .. by .. ':' .. bz
end

-- Fields every record carries, whatever its category.
local COMMON = {
  key = true, category = true, source = true,
  first_observed = true, last_observed = true,
  first_observation_id = true, last_observation_id = true,
  observations = true, x = true, y = true, z = true,
}

-- Closed per-category field sets. A record carrying anything else does not
-- load: that is the guard against a hand-written or injected memory file.
AgentMemory.CATEGORIES = {
  places               = { walkable = true, sightings = true, visits = true },
  routes               = { from = true, to = true, steps = true },
  creature_sightings   = { name = true, kind = true, hp_percent = true },
  dangers              = { cause = true, hp = true, max_hp = true },
  encounters           = { name = true, kind = true },
  conversations        = { name = true, mode = true, text = true },
  item_observations    = { item_id = true, container = true, where = true },
  action_outcomes      = { action = true, accepted = true, reason = true },
  learned_facts        = { fact = true, value = true },
}

function AgentMemory.categoryNames()
  local names = {}
  for name in pairs(AgentMemory.CATEGORIES) do names[#names + 1] = name end
  table.sort(names)
  return names
end

local function isInteger(v)
  return type(v) == 'number' and v == math.floor(v) and v ~= math.huge and v ~= -math.huge
end

local FIELD_TYPES = {
  places = { walkable = 'count', sightings = 'count', visits = 'count' },
  routes = { from = 'string', to = 'string', steps = 'count' },
  creature_sightings = { name = 'string', kind = 'string', hp_percent = 'percent' },
  dangers = { cause = 'string', hp = 'count', max_hp = 'count' },
  encounters = { name = 'string', kind = 'string' },
  conversations = { name = 'string', mode = 'integer', text = 'string' },
  item_observations = { item_id = 'count', container = 'boolean', where = 'string' },
  action_outcomes = { action = 'string', accepted = 'boolean', reason = 'string' },
  learned_facts = { fact = 'string', value = 'string' },
}

local function validField(value, expected)
  if expected == 'count' then return isInteger(value) and value >= 0 end
  if expected == 'integer' then return isInteger(value) end
  if expected == 'percent' then
    return type(value) == 'number' and value >= 0 and value <= 100
  end
  return type(value) == expected
end

-- Strict single-record check. Used on every load, so a file edited by hand or
-- planted by something else is rejected field by field rather than trusted.
function AgentMemory.checkRecord(record)
  if type(record) ~= 'table' then return nil, 'record_not_table' end
  if type(record.category) ~= 'string' then return nil, 'record_missing_category' end
  local allowed = AgentMemory.CATEGORIES[record.category]
  if not allowed then return nil, 'record_unknown_category:' .. record.category end
  -- The only admissible origin. A record claiming to come from a map file, a
  -- spawn table, a wiki or another player is refused here.
  if record.source ~= AgentMemory.SOURCE then return nil, 'record_bad_source' end
  if type(record.key) ~= 'string' or #record.key < 1 or #record.key > 160 then
    return nil, 'record_bad_key'
  end
  for _, field in ipairs({ 'first_observed', 'last_observed', 'observations' }) do
    if not isInteger(record[field]) or record[field] < 0 then
      return nil, 'record_bad_provenance:' .. field
    end
  end
  if record.last_observed < record.first_observed then
    return nil, 'record_provenance_order'
  end
  if record.observations < 1 then return nil, 'record_provenance_count' end
  for _, field in ipairs({ 'first_observation_id', 'last_observation_id' }) do
    if type(record[field]) ~= 'string' or #record[field] < 1 then
      return nil, 'record_bad_provenance:' .. field
    end
  end
  for _, axis in ipairs({ 'x', 'y', 'z' }) do
    if record[axis] ~= nil and not isInteger(record[axis]) then
      return nil, 'record_bad_position'
    end
    if record.category == 'places' and record[axis] == nil then
      return nil, 'record_missing_position:' .. axis
    end
  end
  for field in pairs(record) do
    if not COMMON[field] and not allowed[field] then
      return nil, 'record_unknown_field:' .. tostring(field)
    end
    local expected = FIELD_TYPES[record.category][field]
    if expected and not validField(record[field], expected) then
      return nil, 'record_bad_field:' .. field
    end
  end
  return record, nil
end

-- ---------------------------------------------------------------------------
-- Store
-- ---------------------------------------------------------------------------

local Store = {}
Store.__index = Store

-- Identity is what keeps one character's recollection out of another's. The
-- runtime turns the same triple into the file path, so two characters on one
-- account, or the same name on two worlds, never share a file.
function AgentMemory.identityKey(identity)
  local function encode(value)
    return (tostring(value or 'unknown'):gsub('.', function(char)
      return string.format('%02X', string.byte(char))
    end))
  end
  return encode(identity and identity.agent) .. '/' .. encode(identity and identity.world)
    .. '/' .. encode(identity and identity.character)
end

function AgentMemory.new(identity)
  local self = setmetatable({}, Store)
  self.identity = {
    agent = tostring(identity and identity.agent or 'mock'),
    world = tostring(identity and identity.world or 'unknown'),
    character = tostring(identity and identity.character or 'unknown'),
  }
  self.sessions = 0
  self.entries = {}
  for _, name in ipairs(AgentMemory.categoryNames()) do self.entries[name] = {} end
  self.dirty = false
  return self
end

function Store:count(category)
  local total = 0
  for _ in pairs(self.entries[category] or {}) do total = total + 1 end
  return total
end

function Store:total()
  local total = 0
  for _, name in ipairs(AgentMemory.categoryNames()) do total = total + self:count(name) end
  return total
end

-- Merge semantics: the same observed thing seen again updates last_observed and
-- the sighting count and keeps the original first_observed. That is what makes
-- repeated observation strengthen a memory instead of duplicating it.
function Store:remember(category, key, fields, observationId, now)
  local bucket = self.entries[category]
  if not bucket then return nil, 'unknown_category' end
  local existing = bucket[key]
  local record
  if existing then
    -- Validate a candidate before replacing the stored record. A failed update
    -- must not leave the previous memory partially changed.
    record = {}
    for name, value in pairs(existing) do record[name] = value end
    record.last_observed = now
    record.last_observation_id = observationId
    record.observations = record.observations + 1
    for name, value in pairs(fields or {}) do record[name] = value end
  else
    record = { key = key, category = category, source = AgentMemory.SOURCE,
               first_observed = now, last_observed = now,
               first_observation_id = observationId,
               last_observation_id = observationId, observations = 1 }
    for name, value in pairs(fields or {}) do record[name] = value end
  end
  local checked, reason = AgentMemory.checkRecord(record)
  if not checked then
    return nil, reason
  end
  bucket[key] = record
  self.dirty = true
  return record
end

function Store:recall(category)
  local list = {}
  for _, record in pairs(self.entries[category] or {}) do list[#list + 1] = record end
  table.sort(list, function(a, b) return a.key < b.key end)
  return list
end

function Store:get(category, key)
  return (self.entries[category] or {})[key]
end

-- Records older than `maxAge` are recollection, not news. The brain uses this
-- to avoid walking across the map toward something it saw days ago.
function Store:fresh(category, now, maxAge)
  local list = {}
  for _, record in ipairs(self:recall(category)) do
    if not maxAge or (now - record.last_observed) <= maxAge then list[#list + 1] = record end
  end
  return list
end

function Store:serialize(now)
  local entries = {}
  for _, name in ipairs(AgentMemory.categoryNames()) do
    local bucket = AgentSchema.array({})
    for _, record in ipairs(self:recall(name)) do bucket[#bucket + 1] = record end
    entries[name] = bucket
  end
  return AgentSchema.encode({
    schema = AgentMemory.SCHEMA,
    agent = self.identity.agent,
    world = self.identity.world,
    character = self.identity.character,
    sessions = self.sessions,
    updated = now or 0,
    entries = entries,
  })
end

-- Loading is strict on purpose. A wrong schema, a mismatched identity or a
-- single bad record fails the whole file rather than silently importing part of
-- it: a half-trusted memory is worse than none.
function AgentMemory.load(text, identity)
  local decoded, reason = AgentSchema.decode(text)
  if not decoded then return nil, 'memory_unreadable:' .. tostring(reason) end
  if type(decoded) ~= 'table' then return nil, 'memory_not_object' end
  if decoded.schema ~= AgentMemory.SCHEMA then return nil, 'memory_bad_schema' end
  local store = AgentMemory.new({ agent = decoded.agent, world = decoded.world,
                                  character = decoded.character })
  if identity then
    local wanted = AgentMemory.identityKey(identity)
    if AgentMemory.identityKey(store.identity) ~= wanted then
      return nil, 'memory_identity_mismatch'
    end
  end
  store.sessions = isInteger(decoded.sessions) and decoded.sessions or 0
  if type(decoded.entries) ~= 'table' then return nil, 'memory_missing_entries' end
  for category, records in pairs(decoded.entries) do
    if not AgentMemory.CATEGORIES[category] then
      return nil, 'memory_unknown_category:' .. tostring(category)
    end
    if type(records) ~= 'table' then return nil, 'memory_bad_category_shape' end
    for _, record in ipairs(records) do
      local checked, badRecord = AgentMemory.checkRecord(record)
      if not checked then return nil, badRecord end
      if record.category ~= category then return nil, 'memory_category_mismatch' end
      store.entries[category][record.key] = record
    end
  end
  return store, nil
end

-- ---------------------------------------------------------------------------
-- Recording from observations
-- ---------------------------------------------------------------------------
-- Everything below reads an AgentObservation and nothing else. If a fact is not
-- in the observation the agent just received, it does not get written.

local function positionFields(x, y, z)
  local bx, by, bz = AgentMemory.bucket(x, y, z)
  return bx, by, bz
end

function AgentMemory.observe(store, obs, observationId, now)
  if not store or not obs or not obs.online then return 0 end
  local written = 0
  local p = obs.player.position
  local bx, by, bz = positionFields(p.x, p.y, p.z)
  local here = bx .. ':' .. by .. ':' .. bz

  local walkable = 0
  for _, row in pairs(obs.tiles) do
    if row.walkable then walkable = walkable + 1 end
  end
  local place = store:get('places', here)
  if store:remember('places', here, {
        walkable = math.max(walkable, place and place.walkable or 0),
        sightings = place and place.sightings or 0,
        visits = (place and place.visits or 0) + 1,
        x = bx, y = by, z = bz }, observationId, now) then
    written = written + 1
  end

  -- A route is a remembered adjacency between two places this agent actually
  -- walked between, not a path computed from a map it was never given.
  if store.lastPlace and store.lastPlace ~= here then
    local key = store.lastPlace .. '>' .. here
    local existing = store:get('routes', key)
    if store:remember('routes', key, { from = store.lastPlace, to = here,
                                       steps = (existing and existing.steps or 0) + 1 },
                      observationId, now) then
      written = written + 1
    end
  end
  store.lastPlace = here

  for _, creature in ipairs(obs.creatureList) do
    local cp = creature.position
    local cx, cy, cz = positionFields(cp.x, cp.y, cp.z)
    local name = tostring(creature.name or '')
    if creature.monster then
      local key = name .. '@' .. cx .. ':' .. cy .. ':' .. cz
      if store:remember('creature_sightings', key, {
            name = name, kind = 'monster', hp_percent = creature.hpPercent,
            x = cx, y = cy, z = cz }, observationId, now) then
        written = written + 1
      end
      local spot = store:get('places', cx .. ':' .. cy .. ':' .. cz)
      if spot then
        spot.sightings = (spot.sightings or 0) + 1
        spot.last_observed, spot.last_observation_id = now, observationId
        store.dirty = true
        -- Enough sightings in one place is a fact this agent learned by being
        -- there, which is exactly what it is allowed to remember.
        if spot.sightings >= 10 then
          store:remember('learned_facts', 'hunting_ground:' .. spot.key,
                         { fact = 'hunting_ground', value = spot.key,
                           x = spot.x, y = spot.y, z = spot.z }, observationId, now)
        end
      end
    elseif creature.id ~= obs.player.id then
      if store:remember('encounters', name, {
            name = name, kind = creature.npc and 'npc' or 'player' },
            observationId, now) then
        written = written + 1
      end
    end
  end

  for _, line in ipairs(obs.chat) do
    local who = tostring(line.name or '')
    local text = tostring(line.text or ''):sub(1, 120)
    if #text > 0 then
      if store:remember('conversations', who .. '|' .. text,
                        { name = who, mode = line.mode, text = text },
                        observationId, now) then
        written = written + 1
      end
    end
  end

  for _, container in ipairs(obs.containers) do
    for _, item in ipairs(container.items) do
      if store:remember('item_observations', 'item:' .. item.id,
                        { item_id = item.id, container = item.container and true or false,
                          where = tostring(container.name or '') },
                        observationId, now) then
        written = written + 1
      end
    end
  end

  -- Losing health where it happened is the danger the agent can honestly claim
  -- to have learned. It records the place, never a cause it did not observe.
  local maxHp = obs.player.maxHp or 1
  if obs.player.hp <= maxHp * 0.5 then
    if store:remember('dangers', here, { cause = 'low_health', hp = obs.player.hp,
                                         max_hp = maxHp, x = bx, y = by, z = bz },
                      observationId, now) then
      written = written + 1
    end
  end

  return written
end

function AgentMemory.recordOutcome(store, action, accepted, reason, observationId, now)
  if not store or type(action) ~= 'string' then return end
  local key = action .. ':' .. (accepted and 'accepted' or 'rejected')
  store:remember('action_outcomes', key,
                 { action = action, accepted = accepted and true or false,
                   reason = reason and tostring(reason) or nil },
                 observationId, now)
end

-- ---------------------------------------------------------------------------
-- Planning support
-- ---------------------------------------------------------------------------

function AgentMemory.parseBucketKey(key)
  local x, y, z = tostring(key):match('^(%-?%d+):(%-?%d+):(%-?%d+)$')
  if not x then return nil end
  return tonumber(x), tonumber(y), tonumber(z)
end

-- Aim for the middle of a bucket rather than its corner, so a step that
-- "reduces distance" actually heads into the place rather than along its edge.
function AgentMemory.bucketCentre(key)
  local x, y, z = AgentMemory.parseBucketKey(key)
  if not x then return nil end
  local half = math.floor(AgentMemory.BUCKET / 2)
  return x + half, y + half, z
end

-- Breadth-first search over routes the agent itself walked, returning the next
-- place to head for. This is the difference between "the goal is east" and "I
-- remember getting there by going through here": a straight line runs into the
-- walls a greedy step cannot see around, whereas every edge in this graph is an
-- adjacency this agent has actually traversed.
function Store:routeTo(fromKey, goalKey)
  if fromKey == goalKey then return nil end
  local adjacency = {}
  for _, route in ipairs(self:recall('routes')) do
    if route.from and route.to then
      adjacency[route.from] = adjacency[route.from] or {}
      adjacency[route.from][#adjacency[route.from] + 1] = route.to
    end
  end
  for _, neighbours in pairs(adjacency) do table.sort(neighbours) end
  local queue, seen, parent = { fromKey }, { [fromKey] = true }, {}
  local head = 1
  while head <= #queue do
    local node = queue[head]
    head = head + 1
    for _, next in ipairs(adjacency[node] or {}) do
      if not seen[next] then
        seen[next] = true
        parent[next] = node
        if next == goalKey then
          local step = next
          while parent[step] and parent[step] ~= fromKey do step = parent[step] end
          return step
        end
        queue[#queue + 1] = next
      end
    end
  end
  return nil
end

-- The best place this agent remembers seeing monsters, excluding where it
-- already is and anywhere it remembers nearly dying. Returns a position to head
-- towards, never permission to act: reaching it proves nothing, and whatever
-- the agent does on arrival is judged by the observation it has then.
-- Danger weighs on the choice; it does not veto it. Somewhere with monsters is
-- somewhere the agent has been hurt, so a hard veto silently ruled out every
-- hunting ground it had ever found and left the recollection unusable. A
-- cautious agent -- one that is already hurt -- does avoid them outright.
function Store:huntingGround(currentX, currentY, currentZ, now, maxAge, cautious)
  local hereKey = AgentMemory.bucketKey(currentX, currentY, currentZ)
  local best, bestScore
  for _, place in ipairs(self:fresh('places', now, maxAge)) do
    local danger = self:get('dangers', place.key)
    -- A straight-line heading cannot change floors. After death the character
    -- may respawn above a remembered hunting ground; only a route this agent
    -- actually walked makes a different-floor place a usable destination.
    local reachableFloor = place.z == currentZ
      or self:routeTo(hereKey, place.key) ~= nil
    if place.key ~= hereKey and (place.sightings or 0) > 0
       and reachableFloor and not (cautious and danger) then
      local distance = math.abs(place.x - currentX) + math.abs(place.y - currentY)
        + math.abs(place.z - currentZ) * 50
      local score = place.sightings - distance * 0.05
        - (danger and danger.observations * 2 or 0)
      if not bestScore or score > bestScore then best, bestScore = place, score end
    end
  end
  return best
end

return AgentMemory
