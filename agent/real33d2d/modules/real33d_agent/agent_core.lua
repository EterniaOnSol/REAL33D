-- Pure policy and safety layer. The runtime supplies only public OTClient views
-- and g_game actions. No protocol or server object is available to a Brain.
AgentCore = {}

local function isInteger(n)
  return type(n) == 'number' and n == math.floor(n)
end

function AgentCore.itemKey(place, a, b, c, d)
  if place == 'inventory' then return 'i:' .. tostring(a) end
  if place == 'container' then return 'c:' .. tostring(a) .. ':' .. tostring(b) end
  if place == 'tile' then
    return 't:' .. tostring(a) .. ':' .. tostring(b) .. ':' .. tostring(c) .. ':' .. tostring(d)
  end
end

function AgentCore.validate(intent, observation)
  if type(intent) ~= 'table' or type(observation) ~= 'table' or not observation.online then
    return nil, 'offline_or_bad_intent'
  end
  local action = intent.action
  if action == 'move' then
    if not isInteger(intent.direction) or intent.direction < 0 or intent.direction > 3 then
      return nil, 'direction'
    end
    local p = observation.player.position
    local delta = { [0] = { 0, -1 }, [1] = { 1, 0 }, [2] = { 0, 1 }, [3] = { -1, 0 } }
    local d = delta[intent.direction]
    local key = (p.x + d[1]) .. ':' .. (p.y + d[2]) .. ':' .. p.z
    local tile = observation.tiles[key]
    if not tile or not tile.walkable then return nil, 'tile_not_visible_walkable' end
    return { action = action, direction = intent.direction }, nil
  elseif action == 'say' then
    if type(intent.text) ~= 'string' or #intent.text < 1 or #intent.text > 100
       or intent.text:find('[\r\n%z]') then return nil, 'chat_text' end
    return { action = action, text = intent.text }, nil
  elseif action == 'attack' or action == 'follow' then
    if not isInteger(intent.creatureId) then return nil, 'creature_id' end
    local creature = observation.creatures[intent.creatureId]
    if not creature or creature.id == observation.player.id then return nil, 'creature_not_visible' end
    if action == 'attack' and not creature.monster then return nil, 'attack_monster_only' end
    return { action = action, creatureId = intent.creatureId }, nil
  elseif action == 'cancel_attack' or action == 'cancel_follow' then
    return { action = action }, nil
  elseif action == 'use' or action == 'open_container' then
    if type(intent.item) ~= 'string' or not observation.items[intent.item] then
      return nil, 'item_not_visible'
    end
    if action == 'open_container' and not observation.items[intent.item].container then
      return nil, 'item_not_container'
    end
    return { action = action, item = intent.item }, nil
  elseif action == 'use_with' then
    if type(intent.item) ~= 'string' or not observation.items[intent.item] then
      return nil, 'source_not_visible'
    end
    if not observation.items[intent.item].item then return nil, 'source_not_item' end
    if type(intent.targetItem) == 'string' and observation.items[intent.targetItem] then
      return { action = action, item = intent.item, targetItem = intent.targetItem }, nil
    end
    if isInteger(intent.targetCreatureId) and observation.creatures[intent.targetCreatureId] then
      return { action = action, item = intent.item, targetCreatureId = intent.targetCreatureId }, nil
    end
    return nil, 'target_not_visible'
  elseif action == 'move_item' then
    local source = observation.items[intent.item]
    local destination = observation.destinations[intent.destination]
    if not source or not source.item or not destination then return nil, 'item_or_destination_not_visible' end
    if not isInteger(intent.count) or intent.count < 1 or intent.count > source.count then
      return nil, 'item_count'
    end
    return { action = action, item = intent.item, destination = intent.destination,
             count = intent.count }, nil
  elseif action == 'combat_mode' then
    if not isInteger(intent.fight) or intent.fight < 1 or intent.fight > 3
       or not isInteger(intent.chase) or intent.chase < 0 or intent.chase > 1
       or type(intent.safe) ~= 'boolean' then return nil, 'combat_mode' end
    local changes = (intent.fight ~= observation.combat.fight and 1 or 0)
      + (intent.chase ~= observation.combat.chase and 1 or 0)
      + (intent.safe ~= observation.combat.safe and 1 or 0)
    if changes ~= 1 then return nil, 'combat_mode_one_change' end
    return { action = action, fight = intent.fight, chase = intent.chase, safe = intent.safe }, nil
  end
  return nil, 'unknown_action'
end

function AgentCore.newBudget()
  local self = { sent = {}, last = {}, maxPerMinute = 24 }
  function self:allow(action, now)
    local kept = {}
    for _, t in ipairs(self.sent) do if now - t < 60000 then kept[#kept + 1] = t end end
    self.sent = kept
    local gap = action == 'move' and 750 or action == 'say' and 15000 or 1000
    if #self.sent >= self.maxPerMinute then return false, 'minute_budget' end
    if self.last[action] and now - self.last[action] < gap then return false, 'action_cooldown' end
    if self.last.any and now - self.last.any < 600 then return false, 'global_cooldown' end
    self.sent[#self.sent + 1] = now
    self.last[action], self.last.any = now, now
    return true
  end
  return self
end

local function tileDistance(a, b)
  return math.abs(a.x - b.x) + math.abs(a.y - b.y)
end

local function itemCount(container, id)
  local total = 0
  if container then
    for _, item in ipairs(container.items) do
      if item.id == id then total = total + item.count end
    end
  end
  return total
end

-- What a loot pile is worth, highest first. These are ordinary Tibia 7.72 item
-- ids: prior knowledge of the game's mechanics, the same thing a human player
-- brings with them. It is not live server state, not a loot table read from
-- Fusion32, and nothing here tells the agent what a corpse contains -- only how
-- to rank what it can already see inside an opened container.
local LOOT_VALUE = {
  [3043] = 100,  -- crystal coin
  [3035] = 90,   -- platinum coin
  [3031] = 80,   -- gold coin
  [3028] = 70,   -- small diamond
  [3029] = 68,   -- small sapphire
  [3030] = 66,   -- small emerald
  [3032] = 64,   -- small amethyst
  [3492] = 1,    -- worm: takeable, but never ahead of coins
}

local function lootValue(observation, item)
  local known = LOOT_VALUE[item.id]
  if known then return known end
  local visible = observation.items[item.key]
  if visible and visible.usable then return 20 end -- food and other consumables
  return 10
end

local function findContainer(observation, id)
  for _, container in ipairs(observation.containers) do
    if container.id == id then return container end
  end
end

local function findBag(observation)
  for _, container in ipairs(observation.containers) do
    local name = container.name:lower()
    if name == 'bag' or name == 'backpack' then return container end
  end
end

local function visibleGroundContainer(observation, near, tried)
  local candidates = {}
  for _, row in pairs(observation.tiles) do
    if row.position.z == near.z and tileDistance(row.position, near) <= 2 then
      for _, thing in ipairs(row.things) do
        if thing.container and not tried[thing.id .. ':' .. row.position.x .. ':' .. row.position.y] then
          candidates[#candidates + 1] = { thing = thing, position = row.position,
            distance = tileDistance(row.position, near) }
        end
      end
    end
  end
  table.sort(candidates, function(a, b)
    if a.distance ~= b.distance then return a.distance < b.distance end
    return a.thing.key < b.thing.key
  end)
  return candidates[1]
end

-- Brain contract: decide(AgentObservation, callback(AgentIntent|nil)).
--
-- options.crossSessionMemory: MVP behaviour remembers one item id observed in a
-- previous session (see the 3577 branch below). REAL33D-AGENT-BRIDGE-001 runs
-- with this false: bridge decisions may depend only on the current
-- AgentObservation, never on anything carried across a login. The within-session
-- `visits` counter stays either way; it is derived from positions this brain
-- itself observed during this session and is reset on every new session.
function AgentCore.mockBrain(options)
  local self = { greeted = false, inventoryTried = false, inventoryUsed = false,
                 crossSessionMemory = not (options and options.crossSessionMemory == false),
                 followFirst = options and options.followFirst == true or false,
                 followedOnce = false, triedPickups = {}, pickupStage = nil,
                 proveCancel = options and options.proveCancel == true or false,
                 cancelledOnce = false,
                 bagReopened = false, bagId = nil, steps = 0,
                 targetAt = nil, lastSeenAt = nil, selectedTarget = nil, phase = nil,
                 targetPosition = nil, pendingLoot = false, lootStage = nil,
                 triedCorpses = {}, looted = false, ate = false,
                 observedFoodTrial = false,
                 heading = 1, visits = {} }
  function self:decide(obs, callback, now)
    local hp = obs.player.hp
    local maxHp = obs.player.maxHp
    -- A manual target is not this brain's decision. Leave it alone.
    if obs.attackId and obs.attackId ~= self.selectedTarget then callback(nil) return end
    if self.selectedTarget and obs.creatures[self.selectedTarget] then
      self.lastSeenAt = now
      self.targetPosition = obs.creatures[self.selectedTarget].position
    end
    -- proveCancel makes the bounded scenario release a target once, on purpose,
    -- after chase is already on. Rats die faster than either organic cancel
    -- condition can trigger, so without it the run would never exercise the
    -- cancel dispatcher against a live server-confirmed target. It fires at
    -- most once per session and the agent is free to re-engage afterwards.
    local scenarioCancel = self.proveCancel and not self.cancelledOnce
      and obs.attackId == self.selectedTarget and obs.combat.chase == 1
    if obs.attackId and (hp <= maxHp * 0.65 or scenarioCancel
       or (self.lastSeenAt and now - self.lastSeenAt > 15000)) then
      if scenarioCancel then self.cancelledOnce = true end
      self.phase = 'done'
      callback({ action = 'cancel_attack' })
      return
    end
    if obs.attackId and obs.attackId == self.selectedTarget and obs.combat.chase == 0 then
      callback({ action = 'combat_mode', fight = obs.combat.fight,
                 chase = 1, safe = obs.combat.safe })
      return
    end
    if hp <= maxHp * 0.65 and obs.followId then
      callback({ action = 'cancel_follow' })
      return
    end
    if obs.followId and not obs.attackId and hp > maxHp * 0.65
       and obs.creatures[obs.followId] and obs.creatures[obs.followId].monster then
      self.selectedTarget, self.targetAt, self.lastSeenAt, self.phase =
        obs.followId, now, now, 'attack'
      self.targetPosition = obs.creatures[obs.followId].position
      callback({ action = 'attack', creatureId = obs.followId })
      return
    end
    if self.phase == 'attack' and not obs.attackId and self.selectedTarget
       and not obs.creatures[self.selectedTarget] then
      self.pendingLoot, self.lootStartedAt, self.lootStage = true, now, nil
      self.lootPosition = self.targetPosition
      self.selectedTarget, self.phase = nil, nil
    end
    -- pendingLoot has to expire on its own clock, here, before target
    -- selection. The loot machinery further down is gated on an open bag, so
    -- when the bag is closed none of its timeouts ever run; a pendingLoot set
    -- by a target that simply walked out of view would then stay true forever
    -- and silently disable target selection for the rest of the session.
    if self.pendingLoot and now - self.lootStartedAt > 12000 then
      self.pendingLoot, self.lootStage = false, nil
    end
    if not self.pendingLoot and not obs.attackId and hp > maxHp * 0.65
       and not self.selectedTarget then
      for _, creature in ipairs(obs.creatureList) do
        local name = type(creature.name) == 'string' and creature.name:lower() or ''
        if creature.monster and creature.hpPercent and creature.hpPercent > 0
           and (name == 'rat' or name == 'rabbit' or name == 'bug'
                or name == 'deer' or name == 'snake') then
          self.selectedTarget, self.targetAt, self.lastSeenAt = creature.id, now, now
          self.targetPosition = creature.position
          -- followFirst walks the bounded scenario through Follow once before
          -- it ever attacks, so the certification run exercises the follow
          -- dispatcher against a live target instead of leaving it unproven.
          -- The existing follow phase below then converts it into the attack.
          if self.followFirst and not self.followedOnce then
            self.followedOnce, self.phase = true, 'follow'
            callback({ action = 'follow', creatureId = creature.id })
            return
          end
          self.phase = 'attack'
          callback({ action = 'attack', creatureId = creature.id })
          return
        end
      end
    end
    if hp <= maxHp * 0.5 then
      local p = obs.player.position
      local closest, distance
      for _, creature in ipairs(obs.creatureList) do
        if creature.monster and creature.hpPercent and creature.hpPercent > 0 then
          local d = math.abs(creature.position.x - p.x) + math.abs(creature.position.y - p.y)
          if not distance or d < distance then closest, distance = creature, d end
        end
      end
      if closest and distance <= 4 then
        local delta = { [0] = { 0, -1 }, [1] = { 1, 0 },
                        [2] = { 0, 1 }, [3] = { -1, 0 } }
        local escape, escapeDistance
        for direction = 0, 3 do
          local valid = AgentCore.validate({ action = 'move', direction = direction }, obs)
          if valid then
            local d = delta[direction]
            local candidate = math.abs(closest.position.x - p.x - d[1])
                            + math.abs(closest.position.y - p.y - d[2])
            if candidate > distance and (not escapeDistance or candidate > escapeDistance) then
              escape, escapeDistance = valid, candidate
            end
          end
        end
        callback(escape)
      else
        callback(nil)
      end
      return
    end
    if not self.greeted then
      self.greeted = true
      callback({ action = 'say', text = 'Hello, I am exploring.' })
      return
    end
    -- Nothing carried can hold items: the character never had a container or
    -- dropped it on death. Open a visible ground container, then pick it up
    -- into the backpack slot with the ordinary move_item action. Both steps are
    -- plain client-visible actions the server is free to refuse; the retry is
    -- driven by observing that the inventory did not change.
    local carriesContainer = false
    for _, key in ipairs(obs.inventoryKeys) do
      if obs.items[key] and obs.items[key].container then carriesContainer = true break end
    end
    if not carriesContainer and not obs.attackId and not self.pendingLoot then
      if self.pickupStage == 'moving' then
        if now - self.pickupAt < 7000 then callback(nil) return end
        self.pickupStage = nil
      end
      local candidate = visibleGroundContainer(obs, obs.player.position, self.triedPickups)
      if candidate then
        local mark = candidate.thing.id .. ':' .. candidate.position.x ..
                     ':' .. candidate.position.y
        if self.pickupOpened ~= mark then
          self.pickupOpened = mark
          callback({ action = 'open_container', item = candidate.thing.key })
          return
        end
        self.triedPickups[mark] = true
        self.pickupStage, self.pickupAt = 'moving', now
        callback({ action = 'move_item', item = candidate.thing.key,
                   destination = AgentCore.itemKey('inventory', 3), count = 1 })
        return
      end
    end
    if not self.inventoryTried then
      for _, key in ipairs(obs.inventoryKeys) do
        if obs.items[key] and obs.items[key].container then
          -- Only spend the one attempt once a carried container is actually
          -- visible. Marking it before the search burned the step whenever the
          -- first observation after login had not populated inventory yet, and
          -- the session then never opened a container at all.
          self.inventoryTried = true
          callback({ action = 'open_container', item = key })
          return
        end
      end
    end
    if not self.inventoryUsed and #obs.containers > 0 then
      for _, key in ipairs(obs.inventoryKeys) do
        if obs.items[key] and obs.items[key].container then
          self.inventoryUsed = true
          callback({ action = 'use', item = key })
          return
        end
      end
    end
    local bag = findContainer(obs, self.bagId) or findBag(obs)
    if bag then self.bagId = bag.id end
    if self.inventoryUsed and not bag and not self.bagReopened then
      for _, key in ipairs(obs.inventoryKeys) do
        if obs.items[key] and obs.items[key].container then
          self.bagReopened = true
          callback({ action = 'open_container', item = key })
          return
        end
      end
    end
    -- 3577 was learned from this character's visible dead-rabbit loot in the
    -- preceding QA session, not from server data. Try it through ordinary Use.
    if self.crossSessionMemory and bag and not self.looted and not self.ate
       and not self.observedFoodTrial
       and not self.pendingLoot and not self.lootStage then
      for _, item in ipairs(bag.items) do
        if item.id == 3577 and obs.items[item.key] then
          self.observedFoodTrial = true
          self.eatItemId, self.eatBefore = item.id, itemCount(bag, item.id)
          self.lootStage, self.eatAt = 'eating', now
          callback({ action = 'use', item = item.key })
          return
        end
      end
    end
    if not obs.attackId and bag and not self.looted then
      if self.lootStage == 'opening' then
        for _, container in ipairs(obs.containers) do
          if container.id ~= bag.id then
            self.corpseId, self.lootStage = container.id, 'ready'
            break
          end
        end
        if self.lootStage == 'opening' then
          if now - self.lootOpenedAt < 7000 then callback(nil) return end
          self.lootStage, self.pendingLoot = nil, false
        end
      end
      if self.lootStage == 'moved' then
        if itemCount(bag, self.lootItemId) > self.bagBefore then
          -- Clearing pendingLoot here is what lets the brain pick a new target.
          -- Every failure path below already cleared it; this success path did
          -- not, so after the first loot that actually worked the target search
          -- (`not self.pendingLoot`) was dead for the rest of the session and
          -- the agent wandered without ever attacking again.
          self.looted, self.lootStage, self.pendingLoot = true, 'eat', false
        elseif now - self.lootMovedAt < 7000 then
          callback(nil) return
        else
          self.lootStage, self.pendingLoot = nil, false
        end
      end
      if self.lootStage == 'ready' then
        local corpse = findContainer(obs, self.corpseId)
        if corpse and corpse.itemCount > 0 and bag.itemCount < bag.capacity then
          -- Take the most valuable thing in the corpse, not whatever happens to
          -- sit in slot 0. Ties go to the bigger stack, then to the lower slot
          -- key so the choice stays deterministic.
          local chosen, chosenValue
          for _, item in ipairs(corpse.items) do
            if not item.container and obs.items[item.key] then
              local value = lootValue(obs, item)
              local better = not chosen or value > chosenValue
                or (value == chosenValue and item.count > chosen.count)
              if better then chosen, chosenValue = item, value end
            end
          end
          if chosen then
            local available = obs.items[chosen.key].count or chosen.count or 1
            self.lootItemId, self.bagBefore = chosen.id, itemCount(bag, chosen.id)
            self.lootStage, self.lootMovedAt = 'moved', now
            callback({ action = 'move_item', item = chosen.key,
                       destination = 'c:' .. bag.id .. ':' .. bag.itemCount,
                       count = math.min(available, 100) })
            return
          end
        end
        self.lootStage, self.pendingLoot = nil, false
      end
      if not self.lootStage then
        local near = self.pendingLoot and self.lootPosition or obs.player.position
        if near and tileDistance(near, obs.player.position) <= 5 then
          local candidate = visibleGroundContainer(obs, near, self.triedCorpses)
          if candidate then
            local key = candidate.thing.id .. ':' .. candidate.position.x .. ':' .. candidate.position.y
            self.triedCorpses[key] = true
            self.lootStage, self.lootOpenedAt = 'opening', now
            callback({ action = 'open_container', item = candidate.thing.key })
            return
          end
        end
        if self.pendingLoot and now - self.lootStartedAt < 12000 then
          callback(nil) return
        end
        self.pendingLoot = false
      end
    end
    if self.looted and not self.ate and self.lootStage == 'eat' then
      local food
      for _, item in ipairs(bag and bag.items or {}) do
        local visible = obs.items[item.key]
        if visible and not visible.container then
          if item.id == self.lootItemId then food = item break end
          if visible.usable then food = food or item end
        end
      end
      if food then
        self.eatItemId, self.eatBefore = food.id, itemCount(bag, food.id)
        self.lootStage, self.eatAt = 'eating', now
        callback({ action = 'use', item = food.key })
        return
      end
      self.lootStage = 'done'
    end
    if self.lootStage == 'eating' then
      if itemCount(bag, self.eatItemId) < self.eatBefore then
        self.ate, self.lootStage = true, 'done'
      elseif now - self.eatAt < 7000 then
        callback(nil) return
      else
        self.lootStage = 'done'
      end
    end
    if self.phase == 'attack' and not obs.attackId then
      local target = obs.creatures[self.selectedTarget]
      if not target or target.hpPercent == 0 or now - self.targetAt > 12000 then
        self.selectedTarget, self.phase = nil, nil
      elseif now - self.targetAt > 3000 and hp > maxHp * 0.65 then
        self.targetAt = now
        callback({ action = 'attack', creatureId = target.id })
        return
      end
    end
    if self.phase == 'follow' then
      local target = obs.creatures[self.selectedTarget]
      if not target then
        self.selectedTarget, self.phase = nil, nil
      elseif now - self.targetAt > 20000 then
        self.selectedTarget, self.phase = nil, nil
        callback({ action = 'cancel_follow' })
        return
      elseif now - self.targetAt >= 2200 and hp > maxHp * 0.6 then
        self.phase, self.targetAt = 'attack', now
        callback({ action = 'attack', creatureId = self.selectedTarget })
        return
      else
        callback(nil) return
      end
    end
    if self.phase == 'done' then self.selectedTarget, self.phase = nil, nil end
    if obs.followId then callback(nil) return end
    if obs.attackId then callback(nil) return end
    local p = obs.player.position
    local here = p.x .. ':' .. p.y .. ':' .. p.z
    self.visits[here] = (self.visits[here] or 0) + 1
    local delta = { [0] = { 0, -1 }, [1] = { 1, 0 }, [2] = { 0, 1 }, [3] = { -1, 0 } }
    local best, score
    for j = 0, 3 do
      local direction = (self.heading + j) % 4
      local valid = AgentCore.validate({ action = 'move', direction = direction }, obs)
      if valid then
        local d = delta[direction]
        local key = (p.x + d[1]) .. ':' .. (p.y + d[2]) .. ':' .. p.z
        local visits = self.visits[key] or 0
        if not score or visits < score then best, score = valid, visits end
      end
    end
    if best then
      self.heading, self.steps = best.direction, self.steps + 1
    end
    callback(best)
  end
  return self
end

-- PRESERVED FROM REAL33D-AGENT-MVP-001, OUT OF SCOPE FOR BRIDGE-001.
--
-- This adapter is kept so the MVP result stays reproducible, but it is
-- unreachable in bridge mode: agent_runtime refuses to construct it whenever
-- R33D_AGENT_MODE=1, so no LLM is contacted during bridge certification. Its
-- own status is unchanged from the MVP: IMPLEMENTED_UNVERIFIED, mocked HTTP
-- response only, no local model was ever installed or invoked.
function AgentCore.ollamaBrain(httpPost, model)
  local self = {}
  function self:decide(obs, callback)
    local brief = { player = obs.player, creatures = obs.creatureList,
                    tiles = obs.tiles, inventory = obs.inventory,
                    containers = obs.containers, chat = obs.chat,
                    attackId = obs.attackId, followId = obs.followId,
                    combat = obs.combat }
    local request = {
      model = model, stream = false, format = 'json',
      options = { temperature = 0, num_predict = 80 },
      messages = {
        { role = 'system', content = 'You control one Tibia character. Return only JSON with one legal action: move(direction 0..3), say(text), attack(creatureId), follow(creatureId), cancel_attack, cancel_follow, use(item), open_container(item), use_with(item,targetItem), move_item(item,destination,count), combat_mode(fight,chase,safe), or wait. Use only IDs and item keys in the observation. Prefer safety and short actions.' },
        { role = 'user', content = json.encode(brief) }
      }
    }
    httpPost('http://127.0.0.1:11434/api/chat', request, function(response, err)
      if err or type(response) ~= 'table' or type(response.message) ~= 'table' then
        callback(nil, 'provider_error') return
      end
      local ok, intent = pcall(json.decode, response.message.content or '')
      if not ok or type(intent) ~= 'table' or intent.action == 'wait' then
        callback(nil, 'provider_invalid_json') return
      end
      callback(intent)
    end)
  end
  return self
end
