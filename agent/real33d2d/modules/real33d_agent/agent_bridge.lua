-- REAL33D-AGENT-BRIDGE-001 correlation and JSONL trace.
--
-- One decision cycle produces one correlation_id and up to five events:
--
--   observation -> intent -> validation -> dispatch -> result
--
-- The result event is emitted on the *following* observation, because the
-- authoritative answer to an action is whatever Fusion32 sends back afterwards.
-- It carries the originating action_id and observation_id plus the
-- observation_id it was actually seen in, so a reader can join the two halves.
--
-- Pure Lua: the trace sink and the clock are injected, so the whole file runs
-- under bare LuaJIT in the test suite.
Real33DAgentBridge = {}

local B = {}
B.__index = B

local function fingerprintContainers(obs)
  local parts = {}
  for _, container in ipairs(obs.containers) do
    local ids = {}
    for _, item in ipairs(container.items) do
      ids[#ids + 1] = item.id .. 'x' .. item.count
    end
    parts[#parts + 1] = container.id .. ':' .. container.itemCount .. '[' .. table.concat(ids, ',') .. ']'
  end
  table.sort(parts)
  return table.concat(parts, ';')
end

local function fingerprintEquipment(obs)
  local slots = {}
  for slot in pairs(obs.inventory) do slots[#slots + 1] = slot end
  table.sort(slots)
  local parts = {}
  for _, slot in ipairs(slots) do
    local item = obs.inventory[slot]
    parts[#parts + 1] = slot .. ':' .. item.id .. 'x' .. item.count
  end
  return table.concat(parts, ',')
end

local function fingerprintChat(obs)
  local last = obs.chat[#obs.chat]
  return #obs.chat .. ':' .. (last and (tostring(last.name) .. '|' .. tostring(last.text)) or '')
end

-- The comparable summary of authoritative state. Anything listed here can be
-- named as an observed consequence of an action.
function Real33DAgentBridge.snapshot(obs)
  if not obs or not obs.online then return { online = false } end
  return {
    online = true,
    x = obs.player.position.x, y = obs.player.position.y, z = obs.player.position.z,
    hp = obs.player.hp, mana = obs.player.mana,
    attackId = obs.attackId or 0, followId = obs.followId or 0,
    fight = obs.combat.fight, chase = obs.combat.chase, safe = obs.combat.safe,
    containers = fingerprintContainers(obs),
    equipment = fingerprintEquipment(obs),
    chat = fingerprintChat(obs),
  }
end

local TRACKED = { 'x', 'y', 'z', 'hp', 'mana', 'attackId', 'followId', 'fight',
                  'chase', 'safe', 'containers', 'equipment', 'chat' }

function Real33DAgentBridge.diff(before, after)
  local changed = AgentSchema.array({})
  if not before or not after or not before.online or not after.online then return changed end
  for _, field in ipairs(TRACKED) do
    if before[field] ~= after[field] then
      changed[#changed + 1] = { field = field, before = before[field], after = after[field] }
    end
  end
  return changed
end

function Real33DAgentBridge.new(options)
  options = options or {}
  local self = setmetatable({}, B)
  self.sessionId = options.sessionId or ('s-' .. tostring(os.time()))
  self.sink = options.sink or function() end
  self.clock = options.clock or function() return 0 end
  self.eventSeq, self.obsSeq, self.actionSeq, self.cycleSeq = 0, 0, 0, 0
  self.pending = nil
  self.lines = 0
  return self
end

function B:emit(event, fields)
  self.eventSeq = self.eventSeq + 1
  local record = {
    schema = AgentSchema.EVENT_SCHEMA,
    event = event,
    seq = self.eventSeq,
    ts = self.clock(),
    session_id = self.sessionId,
  }
  for key, value in pairs(fields or {}) do record[key] = value end
  local line = AgentSchema.encode(record)
  self.lines = self.lines + 1
  self.sink(line)
  return line
end

-- Opens a cycle. Emits the pending previous action's result first so that the
-- trace reads in causal order.
function B:beginCycle(obs, projection)
  self.obsSeq = self.obsSeq + 1
  self.cycleSeq = self.cycleSeq + 1
  local cycle = {
    correlationId = string.format('c-%s-%06d', self.sessionId, self.cycleSeq),
    observationId = string.format('o-%s-%06d', self.sessionId, self.obsSeq),
    actionId = nil,
  }
  local snapshot = Real33DAgentBridge.snapshot(obs)
  if self.pending then
    local changed = Real33DAgentBridge.diff(self.pending.snapshot, snapshot)
    self:emit('result', {
      correlation_id = self.pending.correlationId,
      observation_id = self.pending.observationId,
      action_id = self.pending.actionId,
      observed_in = cycle.observationId,
      action = self.pending.action,
      elapsed_ms = self.clock() - self.pending.at,
      authoritative_change = #changed > 0,
      changed = changed,
    })
    self.pending = nil
  end
  self:emit('observation', {
    correlation_id = cycle.correlationId,
    observation_id = cycle.observationId,
    observation = projection,
  })
  cycle.snapshot = snapshot
  return cycle
end

function B:emitIntent(cycle, intent, source)
  self.actionSeq = self.actionSeq + 1
  cycle.actionId = string.format('a-%s-%06d', self.sessionId, self.actionSeq)
  self:emit('intent', {
    correlation_id = cycle.correlationId,
    observation_id = cycle.observationId,
    action_id = cycle.actionId,
    source = source or 'mock',
    intent = intent,
  })
  return cycle.actionId
end

function B:emitValidation(cycle, stage, accepted, reason, intent)
  self:emit('validation', {
    correlation_id = cycle.correlationId,
    observation_id = cycle.observationId,
    action_id = cycle.actionId,
    stage = stage,
    accepted = accepted,
    reason = reason,
    intent = intent,
  })
end

-- Records the dispatch and arms result correlation. `snapshot` is the state as
-- observed immediately before the action left the client.
function B:emitDispatch(cycle, intent, accepted, reason, snapshot)
  self:emit('dispatch', {
    correlation_id = cycle.correlationId,
    observation_id = cycle.observationId,
    action_id = cycle.actionId,
    action = intent and intent.action,
    accepted = accepted,
    reason = reason,
    path = 'g_game.' .. tostring(intent and intent.action),
  })
  if accepted then
    self.pending = {
      correlationId = cycle.correlationId,
      observationId = cycle.observationId,
      actionId = cycle.actionId,
      action = intent.action,
      snapshot = snapshot or cycle.snapshot,
      at = self.clock(),
    }
  end
end

function B:emitSession(event, fields)
  self:emit(event, fields)
end

return Real33DAgentBridge
