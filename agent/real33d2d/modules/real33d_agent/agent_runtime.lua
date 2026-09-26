Real33DAgentRuntime = {}
local R = Real33DAgentRuntime

-- R33D_AGENT_MODE=1 is the canonical, documented opt-in for the agent bridge.
-- R33D_AGENT=1 is accepted as a compatibility alias so the REAL33D-AGENT-MVP-001
-- launchers keep working unchanged. With neither variable set, every branch in
-- this file returns immediately: no timer is scheduled, no g_game event is
-- connected, no observation is built and no trace file is opened. An ordinary
-- human session behaves exactly as it does without this module installed.
local modeSetting = os.getenv('R33D_AGENT_MODE')
local bridgeMode = modeSetting == '1'
local aldricMode = modeSetting == 'aldric'
local traceMode = bridgeMode or aldricMode
local memoryEnabled = os.getenv('R33D_AGENT_MEMORY') == '1'
local legacyMode = os.getenv('R33D_AGENT') == '1'
local enabled = traceMode or legacyMode
local provider = os.getenv('R33D_AGENT_BRAIN') or 'mock'
local timer, pending, epoch, budget, brain, lastDecision, lastState
local pendingSince, decisionSerial = nil, 0
local chat, sequence = {}, 0
local previousOpcode, opcodeHook
local bridge, traceFile
local memory, memoryFile, memorySince
local shop, shopRefs = {open=false,offers={},goods={}}, {}

local function onOpenNpcTrade(items)
  shop,shopRefs={open=true,offers={},goods={},money=nil},{}
  for index,row in ipairs(items or {}) do
    local item=row[1]
    if item and index<=100 then
      local key='s:'..index
      shopRefs[key]=item
      shop.offers[#shop.offers+1]={key=key,id=item:getId(),
        name=tostring(row[2] or ''),weight=math.max(0,math.ceil((row[3] or 0)/100)),
        buyPrice=row[4] or 0,sellPrice=row[5] or 0}
    end
  end
end

local function onPlayerGoods(money,goods)
  if not shop.open then return end
  shop.money=money
  shop.goods={}
  for _,row in ipairs(goods or {}) do
    if row[1] then shop.goods[tostring(row[1]:getId())]=row[2] or 0 end
  end
end

local function onCloseNpcTrade()
  shop,shopRefs={open=false,offers={},goods={}},{}
end

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
  if traceFile or not traceMode then return end
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

-- Memory lives in one flat, human-readable JSON file per identity triple, so
-- two characters never share a recollection and the file can be read, diffed or
-- deleted by hand. The name is flattened rather than nested so no directory has
-- to be created from Lua; R33D_AGENT_MEMORY_DIR points at an existing one.
local function memoryPath(identity)
  local root = os.getenv('R33D_AGENT_MEMORY_DIR')
  -- Hex preserves every byte, including spaces and underscores. Replacing
  -- punctuation with '_' let two distinct character names share one file.
  local name = 'agent_memory__' ..
    AgentMemory.identityKey(identity):gsub('/', '__') .. '.json'
  if root and root ~= '' then return root .. '/' .. name end
  return name
end

local function worldIdentity()
  local ok, host, port = pcall(function()
    return REAL33D.getHost(), REAL33D.getLoginPort()
  end)
  if ok and host then return tostring(host) .. '_' .. tostring(port or '') end
  return 'local'
end

local function loadMemory(identity)
  local path = memoryPath(identity)
  local handle = io.open(path, 'r')
  if not handle then
    log('MEMORY_NEW', 'path=' .. path)
    return AgentMemory.new(identity), path, 'new'
  end
  local text = handle:read('*a')
  handle:close()
  local store, reason = AgentMemory.load(text, identity)
  if not store then
    -- A memory that does not load is not silently half-imported. The session
    -- starts with an empty recollection. Disable saving for this session so
    -- the rejected file remains available for inspection instead of being
    -- overwritten at the next periodic save.
    log('MEMORY_REJECTED', tostring(reason) .. ' path=' .. path)
    return AgentMemory.new(identity), nil, 'rejected:' .. tostring(reason)
  end
  return store, path, 'loaded'
end

local function saveMemory()
  if not memory or not memoryFile then return false end
  local handle = io.open(memoryFile, 'w')
  if not handle then
    log('MEMORY_UNWRITABLE', memoryFile)
    return false
  end
  handle:write(memory:serialize(os.time()), '\n')
  handle:close()
  memory.dirty = false
  return true
end

local function newBridge()
  if not traceMode then return nil end
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
               freeCapacity = player.getFreeCapacity and player:getFreeCapacity() or nil,
               level = player:getLevel(), magicLevel = player:getMagicLevel(),
               skills = {} },
    tiles = {}, creatures = {}, creatureList = {}, items = {},
    destinations = {}, inventory = {}, inventoryKeys = {}, containers = {},
    chat = copyChat(), attackId = attack and attack:getId() or nil,
    followId = follow and follow:getId() or nil,
    combat = { fight = g_game.getFightMode(), chase = g_game.getChaseMode(),
               safe = g_game.isSafeFight() },
    shop = {open=shop.open,money=shop.money,offers={},goods={}}
  }
  for _,offer in ipairs(shop.offers) do
    obs.shop.offers[#obs.shop.offers+1]={key=offer.key,id=offer.id,
      name=offer.name,weight=offer.weight,buyPrice=offer.buyPrice,
      sellPrice=offer.sellPrice}
  end
  for id,count in pairs(shop.goods) do obs.shop.goods[id]=count end
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
  if action == 'move' then return g_game.walk(intent.direction), 'g_game.walk'
  elseif action == 'say' then g_game.talk(intent.text); return true, 'g_game.talk'
  elseif action == 'attack' then g_game.attack(refs['creature:' .. intent.creatureId]); return true, 'g_game.attack'
  elseif action == 'follow' then g_game.follow(refs['creature:' .. intent.creatureId]); return true, 'g_game.follow'
  elseif action == 'cancel_attack' then g_game.cancelAttack(); return true, 'g_game.cancelAttack'
  elseif action == 'cancel_follow' then g_game.cancelFollow(); return true, 'g_game.cancelFollow'
  elseif action == 'use' then g_game.use(refs[intent.item]); return true, 'g_game.use'
  elseif action == 'open_container' then g_game.open(refs[intent.item]); return true, 'g_game.open'
  elseif action == 'use_with' then
    g_game.useWith(refs[intent.item], refs[intent.targetItem or ('creature:' .. intent.targetCreatureId)])
    return true, 'g_game.useWith'
  elseif action == 'move_item' then
    g_game.move(refs[intent.item], destinations[intent.destination], intent.count)
    return true, 'g_game.move'
  elseif action == 'buy' then
    if not shopRefs[intent.offer] then return false,'g_game.buyItem' end
    g_game.buyItem(shopRefs[intent.offer],intent.count,false,false)
    return true,'g_game.buyItem'
  elseif action == 'sell' then
    if not shopRefs[intent.offer] then return false,'g_game.sellItem' end
    g_game.sellItem(shopRefs[intent.offer],intent.count,false)
    return true,'g_game.sellItem'
  elseif action == 'combat_mode' then
    if intent.fight ~= g_game.getFightMode() then
      g_game.setFightMode(intent.fight); return true, 'g_game.setFightMode'
    elseif intent.chase ~= g_game.getChaseMode() then
      g_game.setChaseMode(intent.chase); return true, 'g_game.setChaseMode'
    else g_game.setSafeFight(intent.safe); return true, 'g_game.setSafeFight' end
  end
  return false
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
local function applyIntent(raw, cycle, decisionEpoch, meta)
  if not enabled or epoch ~= decisionEpoch or not g_game.isOnline() or not raw then return end
  local obs, refs, destinations = R.observe() -- recheck after async brain response

  if bridge then
    local plan = nil
    if aldricMode and meta then
      plan = { origin = meta.origin, goal = meta.goal, summary = meta.summary,
        provider = meta.provider, model = meta.model,
        knowledge_ids = meta.knowledge_ids,
        world_knowledge_ids = meta.world_knowledge_ids,
        memory_ids = meta.memory_ids,
        latency_ms = meta.latency_ms, horizon = meta.horizon,
        execution_horizon = meta.execution_horizon }
    elseif memory and brain then
      plan = { memory_records = memory:total(),
               navigating_to = brain.navigatingTo,
               navigating_via = brain.navigatingVia,
               memory_guided = brain.navigatingTo ~= nil }
    end
    bridge:emitIntent(cycle, raw, aldricMode and meta and meta.origin or provider, plan)
    if aldricMode and meta and meta.origin == 'llm' then
      bridge:emit('brain_decision', {
        correlation_id = cycle.correlationId, observation_id = cycle.observationId,
        action_id = cycle.actionId, provider = meta.provider, model = meta.model,
        knowledge_ids = meta.knowledge_ids,
        world_knowledge_ids = meta.world_knowledge_ids,
        memory_ids = meta.memory_ids,
        goal = meta.goal, summary = meta.summary, proposed_intent = raw,
        latency_ms = meta.latency_ms, horizon = meta.horizon,
        execution_horizon = meta.execution_horizon,
        decision_number = meta.decision_number,
      })
    end
  end

  local freshSignature=aldricMode and AgentAldric.signature(obs) or nil
  if aldricMode and cycle and cycle.decisionSignature and
     AgentAldric.materialChange(cycle.decisionSignature,freshSignature) then
    local changed={}
    for key,value in pairs(cycle.decisionSignature) do
      if freshSignature[key]~=value then changed[#changed+1]=key end
    end
    table.sort(changed)
    if bridge then bridge:emitValidation(cycle,'freshness',false,'material_state_changed') end
    if brain and brain.onRejected then brain:onRejected('material_state_changed') end
    log('REJECT','material_state_changed fields='..table.concat(changed,','))
    return
  end

  local checked, schemaReason = AgentSchema.checkObservation(obs)
  if not checked then
    if bridge then bridge:emitValidation(cycle, 'observation', false, schemaReason) end
    if aldricMode and brain and brain.onRejected then brain:onRejected(schemaReason) end
    log('REJECT', schemaReason)
    return
  end

  local normalized, reason = AgentSchema.checkIntent(raw)
  if not normalized then
    if bridge then bridge:emitValidation(cycle, 'schema', false, reason) end
    if aldricMode and brain and brain.onRejected then brain:onRejected(reason) end
    log('REJECT', reason)
    return
  end
  if bridge then bridge:emitValidation(cycle, 'schema', true, nil, normalized) end

  local intent
  intent, reason = AgentCore.validate(normalized, obs)
  if not intent then
    if bridge then bridge:emitValidation(cycle, 'state', false, reason) end
    if aldricMode and brain and brain.onRejected then brain:onRejected(reason) end
    -- Worth remembering: this is how the agent learns that something it tried
    -- was not legal, which is experience rather than imported knowledge.
    if memory then
      AgentMemory.recordOutcome(memory, normalized.action, false, reason,
                                cycle and cycle.observationId or 'unknown',
                                memory.wallClock or os.time())
    end
    log('REJECT', reason)
    return
  end
  if bridge then bridge:emitValidation(cycle, 'state', true, nil, intent) end

  local now = g_clock.millis()
  local allowed, why = budget:allow(intent.action, now)
  if not allowed then
    if bridge then bridge:emitValidation(cycle, 'budget', false, why) end
    if aldricMode and brain and brain.onRejected then brain:onRejected(why) end
    log('LIMIT', why)
    return
  end
  if bridge then bridge:emitValidation(cycle, 'budget', true) end

  local snapshot = bridge and Real33DAgentBridge.snapshot(obs) or nil
  local ok, result, apiPath = pcall(dispatch, intent, refs, destinations)
  if not ok or result == false then
    if bridge then bridge:emitDispatch(cycle, intent, false, 'dispatch_failed', nil, apiPath) end
    if aldricMode and brain and brain.onRejected then brain:onRejected('dispatch_failed') end
    log('DISPATCH_FAILED', intent.action)
    return
  end
  if bridge then bridge:emitDispatch(cycle, intent, true, nil, snapshot, apiPath) end
  if memory then
    AgentMemory.recordOutcome(memory, intent.action, true, nil,
                              cycle and cycle.observationId or 'unknown',
                              memory.wallClock or os.time())
  end
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
    if aldricMode and pending and pendingSince and now-pendingSince > 90000 then
      pending, pendingSince = false, nil
      decisionSerial = decisionSerial + 1 -- discard any late provider callback
      if brain and brain.timeout then brain:timeout(now) end
      if bridge then bridge:emit('provider_failure', {
        provider='ollama',model=brain and brain.model,
        reason='provider_timeout',retry_after_ms=brain and (brain.nextCall-now) }) end
      log('BRAIN_ERROR','provider_timeout')
    end
    if not pending and now - lastDecision >=
       ((provider == 'ollama' and not aldricMode) and 15000 or 2200) then
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
            if aldricMode then cycle.decisionSignature=AgentAldric.signature(obs) end
            -- Memory is written from the observation the agent just received,
            -- after that observation passed its own schema check, and is keyed
            -- to the observation_id that produced it. Nothing else feeds it.
            if memory then
              memory.wallClock = os.time()
              AgentMemory.observe(memory, obs, cycle.observationId, memory.wallClock)
              memorySince = memorySince + 1
              if memorySince >= 20 and memory.dirty then
                memorySince = 0
                if saveMemory() then
                  bridge:emitSession('memory_saved', {
                    records = memory:total(), file = memoryFile, reason = 'interval' })
                end
              end
            end
          else
            observationOk = false
            log('REJECT', schemaReason)
          end
        end
        if observationOk then
          pending, pendingSince, lastDecision = true, now, now
          local currentEpoch = epoch
          decisionSerial = decisionSerial + 1
          local currentSerial = decisionSerial
          local ok, err = pcall(function()
            brain:decide(obs, function(intent, brainError, meta)
              if epoch ~= currentEpoch or decisionSerial ~= currentSerial then return end
              pending, pendingSince = false, nil
              if brainError then
                log('BRAIN_ERROR', brainError)
                if bridge and aldricMode then bridge:emit('provider_failure', {
                  correlation_id=cycle and cycle.correlationId,
                  observation_id=cycle and cycle.observationId,
                  provider='ollama',model=brain.model,reason=brainError,
                  latency_ms=meta and meta.latency_ms,
                  retry_after_ms=math.max(0,brain.nextCall-g_clock.millis()) }) end
              elseif aldricMode and meta and meta.origin=='llm' and not intent then
                if bridge then bridge:emit('brain_decision', {
                  correlation_id=cycle.correlationId,observation_id=cycle.observationId,
                  provider=meta.provider,model=meta.model,
                  knowledge_ids=meta.knowledge_ids,
                  world_knowledge_ids=meta.world_knowledge_ids,
                  memory_ids=meta.memory_ids,
                  goal=meta.goal,summary=meta.summary,
                  proposed_intent={action='wait'},latency_ms=meta.latency_ms,
                  horizon=meta.horizon,
                  decision_number=meta.decision_number }) end
              end
              applyIntent(intent, cycle, currentEpoch, meta)
            end, now)
          end)
          if not ok then
            pending, pendingSince = false, nil
            if aldricMode and brain and brain.timeout then brain:timeout(now) end
            if bridge and aldricMode then bridge:emit('provider_failure', {
              correlation_id=cycle and cycle.correlationId,
              observation_id=cycle and cycle.observationId,
              provider='ollama',model=brain and brain.model,
              reason='brain_exception',retry_after_ms=brain and (brain.nextCall-now) }) end
            log('BRAIN_ERROR', tostring(err))
          end
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
  onCloseNpcTrade()
  epoch, budget, lastDecision, lastState = epoch + 1, AgentCore.newBudget(), -100000, nil
  pending, pendingSince = false, nil
  chat = {}
  bridge = newBridge()
  -- Bridge mode is mock-only by construction. The Ollama adapter is never
  -- instantiated on this path, so certification cannot contact an LLM even if
  -- R33D_AGENT_BRAIN was exported; init() has already forced provider to mock.
  if traceMode then
    -- Persistent memory is opt-in on top of bridge mode. Without it the
    -- certified BRIDGE-001 behaviour is unchanged: no file is read or written.
    local loadState = 'disabled'
    memory, memoryFile, memorySince = nil, nil, 0
    if memoryEnabled then
      local player = g_game.getLocalPlayer()
      local identity = { agent = aldricMode and 'aldric' or 'mock', world = worldIdentity(),
                         character = player and player:getName() or 'unknown' }
      memory, memoryFile, loadState = loadMemory(identity)
      memory.sessions = memory.sessions + 1
      memory.wallClock = os.time()
    end
    -- crossSessionMemory stays false: that flag is the MVP's hardcoded item id,
    -- which is not observation-derived. AgentMemory is the sanctioned route and
    -- carries provenance for everything it holds.
    local model = os.getenv('R33D_AGENT_MODEL') or 'qwen3:4b'
    if aldricMode then
      local infer = AgentOllama.new(function(url,data,callback)
        local previous = HTTP.timeout
        HTTP.timeout = 75
        local ok,result = pcall(HTTP.postJSON,url,data,callback)
        HTTP.timeout = previous
        if not ok then callback(nil,'http_unavailable') end
        return result
      end,function() return g_clock.millis() end)
      brain = AgentAldric.new({infer=infer,model=model,memory=memory,
        knowledge=AgentKnowledge,world=AgentWorldKnowledge,
        clock=function() return g_clock.millis() end})
    else
      brain = AgentCore.mockBrain({ crossSessionMemory = false, followFirst = true,
                                    proveCancel = true, memory = memory })
    end
    log('GAME_START', 'brain=' .. provider .. ' mode=' ..
      (aldricMode and 'aldric' or 'bridge') .. ' memory=' .. loadState)
    bridge:emitSession('session_start', {
      mode = aldricMode and 'aldric' or 'bridge', brain = provider,
      provider = aldricMode and 'ollama' or nil,
      model = aldricMode and model or nil,
      knowledge_schema = aldricMode and AgentKnowledge.SCHEMA or nil,
      knowledge_version = aldricMode and AgentKnowledge.VERSION or nil,
      world_knowledge_schema = aldricMode and AgentWorldKnowledge.SCHEMA or nil,
      world_knowledge_version = aldricMode and AgentWorldKnowledge.VERSION or nil,
      mock_disabled = aldricMode and true or nil,
      cross_session_memory = false,
      observation_schema = AgentSchema.OBSERVATION_SCHEMA,
      intent_schema = AgentSchema.INTENT_SCHEMA,
      actions = AgentSchema.array(AgentSchema.actions()),
      memory = { state = loadState, schema = AgentMemory.SCHEMA,
                 enabled = memoryEnabled and true or false,
                 sessions = memory and memory.sessions or 0,
                 records = memory and memory:total() or 0,
                 file = memoryFile },
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
  onCloseNpcTrade()
  epoch, pending = epoch + 1, false
  local saved = false
  if memory then
    memory.wallClock = os.time()
    saved = saveMemory()
  end
  if bridge then
    bridge:emitSession('session_end', {
      memory_saved = saved,
      memory_records = memory and memory:total() or 0,
      memory_file = memoryFile,
    })
  end
  log('GAME_END', memory and ('memory_saved=' .. tostring(saved) ..
      ' records=' .. memory:total()) or nil)
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
  if aldricMode and (provider ~= 'ollama' or not memoryEnabled) then
    g_logger.error('[R33D-AGENT] aldric requires ollama and persistent memory; disabled')
    enabled = false
    return
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
  connect(g_game, { onGameStart = onStart, onGameEnd = onEnd,
    onTalk = onTalk,onOpenNpcTrade=onOpenNpcTrade,
    onPlayerGoods=onPlayerGoods,onCloseNpcTrade=onCloseNpcTrade })
  if g_game.isOnline() then onStart() end
  timer = scheduleEvent(tick, 1000)
  log('READY', 'opt_in=' .. (traceMode and 'R33D_AGENT_MODE' or 'R33D_AGENT') ..
      ' mode=' .. (aldricMode and 'aldric' or bridgeMode and 'bridge' or 'legacy') ..
      ' brain=' .. provider)
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
  disconnect(g_game, { onGameStart = onStart, onGameEnd = onEnd,
    onTalk = onTalk,onOpenNpcTrade=onOpenNpcTrade,
    onPlayerGoods=onPlayerGoods,onCloseNpcTrade=onCloseNpcTrade })
  if ProtocolGame and ProtocolGame.onOpcode == opcodeHook then
    ProtocolGame.onOpcode = previousOpcode
  end
  opcodeHook, previousOpcode = nil, nil
  epoch, pending = epoch + 1, false
  if memory and memory.dirty then saveMemory() end
  memory, memoryFile = nil, nil
  bridge = nil
  if traceFile then traceFile:close() traceFile = nil end
end
