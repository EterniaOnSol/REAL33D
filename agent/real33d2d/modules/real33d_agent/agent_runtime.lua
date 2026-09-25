Real33DAgentRuntime = {}
local R = Real33DAgentRuntime

-- R33D_AGENT_MODE=1 is the canonical, documented opt-in for the agent bridge.
-- R33D_AGENT=1 is accepted as a compatibility alias so the REAL33D-AGENT-MVP-001
-- launchers keep working unchanged. With neither variable set, every branch in
-- this file returns immediately: no timer is scheduled, no g_game event is
-- connected, no observation is built and no trace file is opened. An ordinary
-- human session behaves exactly as it does without this module installed.
local bridgeMode = os.getenv('R33D_AGENT_MODE') == '1'
local legacyMode = os.getenv('R33D_AGENT') == '1'
local enabled = bridgeMode or legacyMode
local provider = os.getenv('R33D_AGENT_BRAIN') or 'mock'
local timer, pending, epoch, budget, brain, lastDecision, lastState
local chat, sequence = {}, 0
local previousOpcode, opcodeHook
local bridge, traceFile

local function log(event, detail)
  sequence = sequence + 1
  g_logger.info(string.format('[R33D-AGENT] seq=%d event=%s %s', sequence, event, detail or ''))
end

-- The JSONL trace is the machine-readable half of the same story the text log
-- tells. One event per line, flushed immediately so a crash still leaves a
-- usable prefix. Credentials never reach here: the runtime reads R33D_ACC and
-- R33D_PW only inside the login closure and never copies them into any
-- observation, intent or event record.
local function openTrace()
  if traceFile or not bridgeMode then return end
  local path = os.getenv('R33D_AGENT_TRACE') or 'real33d_agent_trace.jsonl'
  local handle, err = io.open(path, 'a')
  if not handle then
    log('TRACE_UNAVAILABLE', tostring(err))
    return
  end
  traceFile = handle
end

local function traceSink(line)
  if not traceFile then return end
  traceFile:write(line, '\n')
  traceFile:flush()
end

local function newBridge()
  if not bridgeMode then return nil end
  openTrace()
  return Real33DAgentBridge.new({
    sessionId = os.date('!%Y%m%dT%H%M%SZ'),
    sink = traceSink,
    clock = function() return g_clock.millis() end,
  })
end

local function copyPos(p)
  if not p then return nil end
  return { x = p.x, y = p.y, z = p.z }
end

local function posKey(p)
  return p.x .. ':' .. p.y .. ':' .. p.z
end

local function copyChat()
  local result = {}
  for i, line in ipairs(chat) do result[i] = line end
  return result
end

local function addItem(obs, refs, key, item, place, position)
  if not item or not item:isItem() then return end
  local count = item:getCount() or 1
  local itemType = g_things and g_things.getThingType(item:getId(), ThingCategoryItem)
  local name = itemType and itemType:getName() or ''
  obs.items[key] = { id = item:getId(), count = count, item = true,
                     name = name, usable = item:isUsable(),
                     container = item:isContainer(), place = place,
                     position = copyPos(position or item:getPosition()) }
  refs[key] = item
end

-- AgentObservation is a scalar copy of the currently displayed client state.
-- Only the map panel's current floor/range is queried; never g_map.getTiles(),
-- minimap cache, server data, protocol internals, or hidden creature tables.
function R.observe()
  if not g_game.isOnline() then return { online = false }, {}, {} end
  local player = g_game.getLocalPlayer()
  local panel = modules.game_interface and modules.game_interface.getMapPanel()
  if not player or not panel then return { online = false }, {}, {} end
  local p = player:getPosition()
  if not p then return { online = false }, {}, {} end
  local attack = g_game.getAttackingCreature()
  local follow = g_game.getFollowingCreature()
  local obs = {
    online = true,
    player = { id = player:getId(), name = player:getName(), position = copyPos(p),
               hp = player:getHealth(), maxHp = player:getMaxHealth(),
               mana = player:getMana(), maxMana = player:getMaxMana(),
               level = player:getLevel(), magicLevel = player:getMagicLevel(),
               skills = {} },
    tiles = {}, creatures = {}, creatureList = {}, items = {},
    destinations = {}, inventory = {}, inventoryKeys = {}, containers = {},
    chat = copyChat(), attackId = attack and attack:getId() or nil,
    followId = follow and follow:getId() or nil,
    combat = { fight = g_game.getFightMode(), chase = g_game.getChaseMode(),
               safe = g_game.isSafeFight() }
  }
  local refs, destinations = {}, {}
  for skill = 0, 6 do obs.player.skills[skill] = player:getSkillLevel(skill) end
  for slot = 1, 10 do
    local item = player:getInventoryItem(slot)
    if item then
      local key = AgentCore.itemKey('inventory', slot)
      addItem(obs, refs, key, item, 'inventory')
      obs.inventory[slot] = obs.items[key]
      obs.inventoryKeys[#obs.inventoryKeys + 1] = key
    end
    local destKey = 'i:' .. slot
    obs.destinations[destKey] = { place = 'inventory', slot = slot }
    destinations[destKey] = { x = 65535, y = slot, z = 0 }
  end
  for cid, container in pairs(g_game.getContainers()) do
    if container and not container:isClosed() then
      local parent = container:getContainerItem()
      local parentPosition = parent and parent:getPosition()
      local contents = { id = cid, name = container:getName(),
                         capacity = container:getCapacity(),
                         itemCount = container:getItemsCount(),
                         sourcePosition = copyPos(parentPosition),
                         sourcePlace = parentPosition and
                           (parentPosition.x == 65535 and 'carried' or 'tile') or 'unknown',
                         items = {} }
      obs.containers[#obs.containers + 1] = contents
      for slot = 0, container:getCapacity() - 1 do
        local destKey = 'c:' .. cid .. ':' .. slot
        obs.destinations[destKey] = { place = 'container', id = cid, slot = slot }
        destinations[destKey] = container:getSlotPosition(slot)
        local item = container:getItem(slot)
        if item then
          addItem(obs, refs, destKey, item, 'container', destinations[destKey])
          contents.items[#contents.items + 1] = { key = destKey, id = item:getId(),
                                                    name = obs.items[destKey].name,
                                                    count = item:getCount() or 1,
                                                    container = item:isContainer() }
        end
      end
    end
  end
  -- 11x11 is a conservative subset of the on-screen map. isInRange also
  -- excludes cells outside the real UI viewport when it is resized.
  for dx = -5, 5 do
    for dy = -5, 5 do
      local pos = { x = p.x + dx, y = p.y + dy, z = p.z }
      if panel:isInRange(pos) then
        local tile = g_map.getTile(pos)
        -- isCovered() takes the first visible floor, not a constant. The MVP
        -- passed 0, which asks "is anything at all above this tile" -- true for
        -- every tile indoors and underground, so the agent observed an empty
        -- map and froze under any roof. The scan below only ever visits the
        -- player's own floor, and Tile::isCovered returns false when the tile
        -- sits on the floor it is asked about, so passing p.z is both the
        -- faithful question and a no-op filter here. This narrows nothing: the
        -- bound is still same-floor plus the panel's own viewport test.
        if tile and not tile:isCovered(p.z) then
          local key = posKey(pos)
          local row = { position = copyPos(pos), walkable = tile:isWalkable(), things = {} }
          obs.tiles[key] = row
          obs.destinations['tile:' .. key] = { place = 'tile', position = copyPos(pos) }
          destinations['tile:' .. key] = pos
          for _, thing in ipairs(tile:getThings()) do
            if thing:isItem() then
              local itemKey = AgentCore.itemKey('tile', pos.x, pos.y, pos.z, thing:getStackPos())
              addItem(obs, refs, itemKey, thing, 'tile', pos)
              row.things[#row.things + 1] = { key = itemKey, id = thing:getId(),
                                              name = obs.items[itemKey].name,
                                              container = thing:isContainer(),
                                              count = thing:getCount() or 1,
                                              stack = thing:getStackPos() }
            end
          end
        end
      end
    end
  end
  for _, creature in ipairs(panel:getSightSpectators()) do
    local cp = creature:getPosition()
    if cp and cp.z == p.z and creature:canBeSeen() and panel:isInRange(cp)
       and obs.tiles[posKey(cp)] then
      local record = { id = creature:getId(), name = creature:getName(),
                       position = copyPos(cp), hpPercent = creature:getHealthPercent(),
                       monster = creature:isMonster(), player = creature:isPlayer(),
                       npc = creature:isNpc() }
      obs.creatures[record.id] = record
      obs.creatureList[#obs.creatureList + 1] = record
      refs['creature:' .. record.id] = creature
    end
  end
  table.sort(obs.creatureList, function(a, b) return a.id < b.id end)
  return obs, refs, destinations
end

local function dispatch(intent, refs, destinations)
  local action = intent.action
  if action == 'move' then return g_game.walk(intent.direction)
  elseif action == 'say' then g_game.talk(intent.text)
  elseif action == 'attack' then g_game.attack(refs['creature:' .. intent.creatureId])
  elseif action == 'follow' then g_game.follow(refs['creature:' .. intent.creatureId])
  elseif action == 'cancel_attack' then g_game.cancelAttack()
  elseif action == 'cancel_follow' then g_game.cancelFollow()
  elseif action == 'use' then g_game.use(refs[intent.item])
  elseif action == 'open_container' then g_game.open(refs[intent.item])
  elseif action == 'use_with' then
    g_game.useWith(refs[intent.item], refs[intent.targetItem or ('creature:' .. intent.targetCreatureId)])
  elseif action == 'move_item' then
    g_game.move(refs[intent.item], destinations[intent.destination], intent.count)
  elseif action == 'combat_mode' then
    if intent.fight ~= g_game.getFightMode() then g_game.setFightMode(intent.fight)
    elseif intent.chase ~= g_game.getChaseMode() then g_game.setChaseMode(intent.chase)
    else g_game.setSafeFight(intent.safe) end
  end
  return true
end

-- Three gates, in this order, every time:
--
--   schema  - is this a well-formed AgentIntent at all
--   state   - is it legal against freshly re-read client-visible state
--   budget  - is the agent allowed to act right now
--
-- The state gate re-reads the client after the Brain answered, so a target that
-- walked out of view or an item that moved between decision and dispatch is
-- rejected rather than acted on from a stale view.
local function applyIntent(raw, cycle, decisionEpoch)
  if not enabled or epoch ~= decisionEpoch or not g_game.isOnline() or not raw then return end
  local obs, refs, destinations = R.observe() -- recheck after async brain response

  if bridge then bridge:emitIntent(cycle, raw, provider) end

  local checked, schemaReason = AgentSchema.checkObservation(obs)
  if not checked then
    if bridge then bridge:emitValidation(cycle, 'observation', false, schemaReason) end
    log('REJECT', schemaReason)
    return
  end

  local normalized, reason = AgentSchema.checkIntent(raw)
  if not normalized then
    if bridge then bridge:emitValidation(cycle, 'schema', false, reason) end
    log('REJECT', reason)
    return
  end
  if bridge then bridge:emitValidation(cycle, 'schema', true, nil, normalized) end

  local intent
  intent, reason = AgentCore.validate(normalized, obs)
  if not intent then
    if bridge then bridge:emitValidation(cycle, 'state', false, reason) end
    log('REJECT', reason)
    return
  end
  if bridge then bridge:emitValidation(cycle, 'state', true, nil, intent) end

  local now = g_clock.millis()
  local allowed, why = budget:allow(intent.action, now)
  if not allowed then
    if bridge then bridge:emitValidation(cycle, 'budget', false, why) end
    log('LIMIT', why)
    return
  end
  if bridge then bridge:emitValidation(cycle, 'budget', true) end

  local snapshot = bridge and Real33DAgentBridge.snapshot(obs) or nil
  local ok, result = pcall(dispatch, intent, refs, destinations)
  if not ok or result == false then
    if bridge then bridge:emitDispatch(cycle, intent, false, 'dispatch_failed') end
    log('DISPATCH_FAILED', intent.action)
    return
  end
  if bridge then bridge:emitDispatch(cycle, intent, true, nil, snapshot) end
  log('INTENT', intent.action .. (intent.creatureId and (' creature=' .. intent.creatureId) or '') ..
      (intent.direction and (' direction=' .. intent.direction) or '') ..
      (intent.item and (' item=' .. intent.item) or '') ..
      (intent.destination and (' destination=' .. intent.destination) or ''))
end

local function tick()
  timer = nil
  if not enabled then return end
  if g_game.isOnline() then
    local now = g_clock.millis()
    if not pending and now - lastDecision >= (provider == 'ollama' and 15000 or 2200) then
      local obs = R.observe()
      if obs.online and obs.player.hp > 0 then
        local monsters = {}
        for _, creature in ipairs(obs.creatureList) do
          if creature.monster then
            monsters[#monsters + 1] = creature.name .. ':' .. creature.id .. '@' ..
              tostring(creature.hpPercent)
          end
        end
        local containerDetails = {}
        for _, container in ipairs(obs.containers) do
          local names = {}
          for i = 1, math.min(#container.items, 8) do
            local item = container.items[i]
            names[#names + 1] = item.id .. ':' .. item.name .. 'x' .. item.count ..
              (obs.items[item.key].usable and ':usable' or '')
          end
          containerDetails[#containerDetails + 1] = container.id .. ':' ..
            container.sourcePlace .. ':' .. container.name .. '[' ..
            table.concat(names, ',') .. ']'
        end
        local nearbyGround = {}
        for _, row in pairs(obs.tiles) do
          local distance = math.abs(row.position.x - obs.player.position.x) +
                           math.abs(row.position.y - obs.player.position.y)
          if distance <= 3 then
            for _, thing in ipairs(row.things) do
              if thing.container then
                nearbyGround[#nearbyGround + 1] = thing.id .. ':' .. thing.name .. '@' ..
                  row.position.x .. ',' .. row.position.y
              end
            end
          end
        end
        table.sort(nearbyGround)
        local state = string.format('pos=%d,%d,%d hp=%d/%d attack=%s follow=%s containers=%d monsters=%s bags=%s ground=%s',
          obs.player.position.x, obs.player.position.y, obs.player.position.z,
          obs.player.hp, obs.player.maxHp, tostring(obs.attackId), tostring(obs.followId),
          #obs.containers, table.concat(monsters, ','), table.concat(containerDetails, ';'),
          table.concat(nearbyGround, ';'))
        if state ~= lastState then log('OBSERVED_RESULT', state) lastState = state end
        -- The observation is schema-checked before any Brain may see it. A
        -- malformed or out-of-whitelist observation ends the cycle here; it is
        -- never published and never decided on.
        local cycle, observationOk = nil, true
        if bridge then
          local checked, schemaReason = AgentSchema.checkObservation(obs)
          if checked then
            cycle = bridge:beginCycle(obs, AgentSchema.projectObservation(obs))
          else
            observationOk = false
            log('REJECT', schemaReason)
          end
        end
        if observationOk then
          pending, lastDecision = true, now
          local currentEpoch = epoch
          local ok, err = pcall(function()
            brain:decide(obs, function(intent, brainError)
              if epoch ~= currentEpoch then return end
              pending = false
              if brainError then log('BRAIN_ERROR', brainError) end
              applyIntent(intent, cycle, currentEpoch)
            end, now)
          end)
          if not ok then pending = false log('BRAIN_ERROR', tostring(err)) end
        end
      end
    end
  end
  timer = scheduleEvent(tick, 1000)
end

local function onTalk(name, level, mode, message, channel, position)
  if type(message) ~= 'string' then return end
  chat[#chat + 1] = { name = tostring(name or ''), mode = mode,
                     text = message:sub(1, 200), position = copyPos(position) }
  while #chat > 8 do table.remove(chat, 1) end
  log('CHAT_RECEIVED', 'from=' .. tostring(name or '') .. ' mode=' .. tostring(mode))
end

local function onStart()
  epoch, budget, lastDecision, lastState = epoch + 1, AgentCore.newBudget(), -100000, nil
  chat = {}
  bridge = newBridge()
  -- Bridge mode is mock-only by construction. The Ollama adapter is never
  -- instantiated on this path, so certification cannot contact an LLM even if
  -- R33D_AGENT_BRAIN was exported; init() has already forced provider to mock.
  if bridgeMode then
    brain = AgentCore.mockBrain({ crossSessionMemory = false, followFirst = true,
                                  proveCancel = true })
    log('GAME_START', 'brain=mock mode=bridge')
    bridge:emitSession('session_start', {
      mode = 'bridge', brain = 'mock', cross_session_memory = false,
      observation_schema = AgentSchema.OBSERVATION_SCHEMA,
      intent_schema = AgentSchema.INTENT_SCHEMA,
      actions = AgentSchema.array(AgentSchema.actions()),
    })
    return
  end
  brain = provider == 'ollama' and AgentCore.ollamaBrain(function(url, data, callback)
    local previous = HTTP.timeout
    HTTP.timeout = 30
    local ok, result = pcall(HTTP.postJSON, url, data, callback)
    HTTP.timeout = previous
    if not ok then callback(nil, 'http_unavailable') end
    return result
  end, os.getenv('R33D_AGENT_MODEL') or 'gemma3:4b') or AgentCore.mockBrain()
  log('GAME_START', 'brain=' .. provider)
end

local function onEnd()
  epoch, pending = epoch + 1, false
  if bridge then bridge:emitSession('session_end', {}) end
  log('GAME_END')
end

function R.init()
  if not enabled then return end -- ordinary human play is untouched
  if provider ~= 'mock' and provider ~= 'ollama' then
    g_logger.error('[R33D-AGENT] unsupported brain; disabled')
    enabled = false
    return
  end
  -- REAL33D-AGENT-BRIDGE-001 certifies the deterministic mock brain only. An
  -- LLM provider requested alongside bridge mode is refused out loud rather
  -- than honoured, so a certification run cannot silently become an LLM run.
  if bridgeMode and provider ~= 'mock' then
    g_logger.error('[R33D-AGENT] bridge mode is mock-only; ignoring brain=' .. provider)
    provider = 'mock'
  end
  epoch, budget, lastDecision = 0, AgentCore.newBudget(), -100000
  -- Read-only hook, called before the ordinary C++ parser. Returning the
  -- original result leaves the parser and packet cursor untouched.
  if ProtocolGame and type(ProtocolGame.onOpcode) == 'function' then
    previousOpcode = ProtocolGame.onOpcode
    opcodeHook = function(self, opcode, message)
      if opcode == 163 then log('SERVER_CLEAR_TARGET') end
      return previousOpcode(self, opcode, message)
    end
    ProtocolGame.onOpcode = opcodeHook
  end
  connect(g_game, { onGameStart = onStart, onGameEnd = onEnd, onTalk = onTalk })
  if g_game.isOnline() then onStart() end
  timer = scheduleEvent(tick, 1000)
  log('READY', 'opt_in=' .. (bridgeMode and 'R33D_AGENT_MODE' or 'R33D_AGENT') ..
      ' mode=' .. (bridgeMode and 'bridge' or 'legacy') .. ' brain=' .. provider)
  if os.getenv('R33D_AGENT_AUTOLOGIN') == '1' then
    scheduleEvent(function()
      if g_game.isOnline() then return end
      local account, password = os.getenv('R33D_ACC'), os.getenv('R33D_PW')
      if not account or not password or account == '' or password == '' then
        log('LOGIN_WAIT', 'credentials_missing') return
      end
      EnterGame.setUniqueServer(REAL33D.getHost(), REAL33D.getLoginPort(), REAL33D.getProtocolVersion())
      local a = rootWidget:recursiveGetChildById('accountNameTextEdit')
      local w = rootWidget:recursiveGetChildById('accountPasswordTextEdit')
      if not a or not w then log('LOGIN_WAIT', 'widgets_missing') return end
      a:setText(account)
      w:setText(password)
      log('LOGIN_REQUEST', 'client_ui')
      EnterGame.doLogin()
    end, 3000)
  end
end

function R.terminate()
  if not enabled then return end
  if timer then removeEvent(timer) timer = nil end
  disconnect(g_game, { onGameStart = onStart, onGameEnd = onEnd, onTalk = onTalk })
  if ProtocolGame and ProtocolGame.onOpcode == opcodeHook then
    ProtocolGame.onOpcode = previousOpcode
  end
  opcodeHook, previousOpcode = nil, nil
  epoch, pending = epoch + 1, false
  bridge = nil
  if traceFile then traceFile:close() traceFile = nil end
end
