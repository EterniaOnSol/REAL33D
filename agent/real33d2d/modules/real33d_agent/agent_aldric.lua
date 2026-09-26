-- Provider-independent strategic Brain. Only the model chooses goals and
-- actions. The tactical continuation repeats at most three further legal
-- local moves from one model-selected direction, stopping on material change.
AgentAldric = {}
local B = AgentAldric

B.IDENTITY = 'You are Aldric, an autonomous player of Tibia 7.72 on the Fusion32 world through the REAL33D2D client. Your objective: Progress your character intelligently. Stay alive. Earn resources. Improve your equipment or supplies when useful. Think like an experienced Tibia player: choose your own goals for orientation, hunting, looting, supplies, economy and progression. Fusion32 resolves all gameplay. Current client observation outranks personal memory, which outranks static world knowledge, which outranks general veteran game knowledge. Static landmarks are public historical leads; a position-only match is tentative, while a visible named cue strengthens recognition. Current client observation outranks personal memory. Memories and knowledge never prove live creatures, items, occupancy or NPC offers. Exact local prices and offerings are unknown until a current trade observation.'
B.RULES = 'Return one JSON object only with goal, summary, intent object, and horizon integer from 1 to 4. Intent must be a JSON object, never a string. Include every required field for the chosen action: move requires integer direction; attack/follow require a currently visible creatureId; use/open_container require a current item key. Legal actions: move(direction 0 north,1 east,2 south,3 west), say(text), attack(creatureId), follow(creatureId), cancel_attack, cancel_follow, use(item), open_container(item), use_with(item,targetItem or targetCreatureId), move_item(item,destination,count), buy(offer,count), sell(offer,count), combat_mode(fight,chase,safe), wait. Buy/sell only when a current shop is open and the offer key, price, money and owned goods are in current observation; never invent a price. Form the goal from the current observation; reconsider it when visibility changes. If visible_nonself_creatures is zero, memory of a creature is only a historical lead, not a currently observed target. An item with name unknown has unknown identity and effect; do not infer that it is a key, weapon, food, or loot from its number or slot. A legal use may test an unknown item, but describe it as a test rather than an established effect. Choose your own goal; no scripted hunt. Use only currently listed creature IDs, item keys and offer keys. Treat friendly-seeming inhabitants cautiously even if the client labels them monsters. Never claim a remembered creature or item is currently present. If uncertain or unsafe, wait or choose a cautious legal action. For safe local exploration with a clear adjacent route and no visible nonself creature, prefer horizon 2 to 4; choose horizon 1 near inhabitants, threats, blocked terrain, or uncertainty. A move with horizon 1 may still continue one extra tile through a freshly visible walkable route if no other creature is visible and health is high; any relevant change stops it. The tactical controller repeats only your selected direction and chooses no new goal. Do not include chain-of-thought.'
B.RULES = B.RULES .. ' Use current_session_history to notice repeated backtracking. If your recent moves have not improved position, resources or safety, choose a different useful goal or route. A public landmark bearing is approximate; every step still requires a currently visible walkable tile. Keep goal and summary short.'

local DIR = { [0]={0,-1}, [1]={1,0}, [2]={0,1}, [3]={-1,0} }
local function positionKey(x,y,z) return x .. ':' .. y .. ':' .. z end
local function addTopics(obs)
  local t = { 'progression', 'safety', 'explore', 'movement' }
  local hp = obs.player.hp / math.max(obs.player.maxHp, 1)
  if hp < 0.65 then t[#t+1]='health'; t[#t+1]='supplies' end
  if #obs.creatureList > 1 or obs.attackId then
    t[#t+1]='combat'; t[#t+1]='loot'
  end
  if #obs.containers > 0 or #obs.inventoryKeys > 0 then
    t[#t+1]='inventory'; t[#t+1]='equipment'; t[#t+1]='economy'
  end
  if #obs.chat > 0 then t[#t+1]='social'; t[#t+1]='chat'; t[#t+1]='npc' end
  if obs.shop and obs.shop.open then t[#t+1]='economy'; t[#t+1]='npc' end
  return t
end

function B.compactObservation(obs)
  local p = obs.player.position
  local occupants={}
  for _,c in ipairs(obs.creatureList) do
    if c.id~=obs.player.id then
      occupants[positionKey(c.position.x,c.position.y,c.position.z)]=
        c.npc and 'N' or c.monster and 'M' or 'P'
    end
  end
  local mapRows={}
  for dy=-3,3 do
    local row={}
    for dx=-3,3 do
      local key=positionKey(p.x+dx,p.y+dy,p.z)
      local tile=obs.tiles[key]
      row[#row+1]=(dx==0 and dy==0) and '@'
        or occupants[key] or (tile and (tile.walkable and '.' or '#') or '?')
    end
    mapRows[#mapRows+1]=table.concat(row)
  end
  local adjacent = {}
  for direction = 0, 3 do
    local d = DIR[direction]
    local tile = obs.tiles[positionKey(p.x+d[1], p.y+d[2], p.z)]
    adjacent[#adjacent+1] = { direction=direction, visible=tile ~= nil,
                              walkable=tile and tile.walkable or false }
  end
  local creatures = {}
  local nonselfCount=0
  for _, c in ipairs(obs.creatureList) do
    if c.id~=obs.player.id then nonselfCount=nonselfCount+1 end
    if #creatures < 12 then
      creatures[#creatures+1] = { id=c.id, name=c.name, x=c.position.x,
        y=c.position.y, z=c.position.z, hpPercent=c.hpPercent,
        kind=c.monster and 'monster' or c.npc and 'npc'
          or c.player and 'player' or 'other' }
    end
  end
  local equipment = {}
  for slot=1,10 do
    local item = obs.inventory[slot]
    if item then equipment[#equipment+1] = { key='i:'..slot, slot=slot,
      id=item.id, name=item.name~='' and item.name or 'unknown',
      count=item.count, container=item.container, usable=item.usable } end
  end
  local containers = {}
  for i=1, math.min(#obs.containers, 3) do
    local c = obs.containers[i]
    local items = {}
    for j=1, math.min(#c.items, 8) do
      local item=c.items[j]
      items[#items+1]={ key=item.key, id=item.id, name=item.name,
                        count=item.count, container=item.container }
    end
    containers[#containers+1]={ name=c.name, capacity=c.capacity,
      itemCount=c.itemCount, source=c.sourcePlace, items=items }
  end
  local nearbyContainers = {}
  for _, tile in pairs(obs.tiles) do
    local distance=math.abs(tile.position.x-p.x)+math.abs(tile.position.y-p.y)
    if distance <= 2 then
      for _, thing in ipairs(tile.things) do
        if thing.container and #nearbyContainers < 6 then
          nearbyContainers[#nearbyContainers+1]={ key=thing.key,
            x=tile.position.x,y=tile.position.y,z=tile.position.z,
            name=thing.name }
        end
      end
    end
  end
  table.sort(nearbyContainers,function(a,b) return a.key < b.key end)
  local chat={}
  for i=math.max(1,#obs.chat-3),#obs.chat do
    local line=obs.chat[i]
    chat[#chat+1]={ from=line.name, text=line.text, mode=line.mode }
  end
  return {
    self={ name=obs.player.name, x=p.x,y=p.y,z=p.z,
      hp=obs.player.hp,maxHp=obs.player.maxHp,mana=obs.player.mana,
      maxMana=obs.player.maxMana,level=obs.player.level,
      magicLevel=obs.player.magicLevel,skills=obs.player.skills,
      freeCapacity=obs.player.freeCapacity,attackId=obs.attackId,
      followId=obs.followId,combat=obs.combat },
    visible={creatures=creatures,visible_nonself_creatures=nonselfCount,adjacent=adjacent,
             nearbyContainers=nearbyContainers,
             local_map={rows=mapRows,legend='7x7 north-to-south, west-to-east: @ self, . visible walkable, # visible blocked, ? unseen, M monster, N NPC, P player'}},
    owned={equipment=equipment,containers=containers},chat=chat,
    shop=obs.shop and {open=obs.shop.open,money=obs.shop.money,
      offers=obs.shop.offers,goods=obs.shop.goods} or nil,
  }
end

function B.relevantMemory(memory, obs)
  local rows={}
  if not memory then return rows end
  local categories={'dangers','creature_sightings','places','routes',
    'conversations','item_observations','action_outcomes','learned_facts'}
  for _,category in ipairs(categories) do
    local records=memory:recall(category)
    table.sort(records,function(a,b)
      if a.last_observed ~= b.last_observed then return a.last_observed > b.last_observed end
      return a.key < b.key
    end)
    local limit=category=='places' and 2 or 1
    for i=1,math.min(limit,#records) do
      local r=records[i]
      rows[#rows+1]={ id=category..'/'..r.key, category=category,
        key=r.key, name=r.name, x=r.x,y=r.y,z=r.z,
        last_observed=r.last_observed, observations=r.observations,
        source=r.source, value=r.value, cause=r.cause }
    end
  end
  return rows
end

local function signature(obs)
  local p=obs.player
  local ids={}
  for _,c in ipairs(obs.creatureList) do
    if c.id ~= p.id then ids[#ids+1]=tostring(c.id)..':'..tostring(c.hpPercent) end
  end
  table.sort(ids)
  local containers={}
  for _,c in ipairs(obs.containers) do
    local items={}
    for _,item in ipairs(c.items) do
      items[#items+1]=tostring(item.id)..'x'..tostring(item.count)
    end
    containers[#containers+1]=tostring(c.id)..':'..tostring(c.itemCount)..
      '['..table.concat(items,',')..']'
  end
  table.sort(containers)
  local equipment={}
  for slot,item in pairs(obs.inventory) do
    equipment[#equipment+1]=tostring(slot)..':'..tostring(item.id)
  end
  table.sort(equipment)
  local chat=obs.chat[#obs.chat]
  local pos=p.position
  return { hp=p.hp, maxHp=p.maxHp,
    position=positionKey(pos.x,pos.y,pos.z),attack=obs.attackId or 0,
    follow=obs.followId or 0, creatures=table.concat(ids,','),
    containers=table.concat(containers,','),
    equipment=table.concat(equipment,','),capacity=p.freeCapacity or 0,
    chat=chat and (chat.name..'|'..chat.text) or '',
    shop=obs.shop and AgentSchema.encode(obs.shop) or '' }
end
B.signature=signature

function B.materialChange(a,b,ignorePosition)
  if not a then return true end
  if math.abs(a.hp-b.hp) >= math.max(5,math.floor(b.maxHp*0.08)) then return true end
  if math.abs(a.capacity-b.capacity)>=10 then return true end
  return (not ignorePosition and a.position~=b.position)
    or a.attack~=b.attack or a.follow~=b.follow
    or a.creatures~=b.creatures or a.containers~=b.containers
    or a.equipment~=b.equipment or a.chat~=b.chat or a.shop~=b.shop
end

function B.parse(text)
  if type(text)~='string' or #text>3000 then return nil,'provider_invalid_output' end
  local decoded,reason=AgentSchema.decode(text)
  if not decoded or type(decoded)~='table' then return nil,'provider_invalid_json:'..tostring(reason) end
  if type(decoded.goal)~='string' or #decoded.goal<1
     or decoded.goal:find('[%z\1-\31]') then return nil,'provider_invalid_goal' end
  if type(decoded.summary)~='string' or #decoded.summary<1
     or decoded.summary:find('[%z\1-\31]') then return nil,'provider_invalid_summary' end
  if type(decoded.intent)~='table' or type(decoded.intent.action)~='string' then
    return nil,'provider_invalid_intent'
  end
  local horizon=decoded.horizon or 1
  if type(horizon)~='number' or horizon~=math.floor(horizon) or horizon<1 or horizon>4 then
    return nil,'provider_invalid_horizon'
  end
  return { goal=decoded.goal:sub(1,120),summary=decoded.summary:sub(1,180),
    intent=decoded.intent,horizon=horizon },nil
end

function B.new(options)
  assert(options and type(options.infer)=='function','provider inference required')
  local self={ infer=options.infer,model=options.model or 'unknown',
    clock=options.clock or function() return 0 end,
    memory=options.memory,knowledge=options.knowledge or AgentKnowledge,
    world=options.world or AgentWorldKnowledge,
    plan=nil,lastCall=-100000,nextCall=-100000,failures=0,
    decisions=0,recentPositions={},recentDecisions={} }

  local function rememberPosition(obs)
    local p=obs.player.position
    local key=positionKey(p.x,p.y,p.z)
    local recent=self.recentPositions
    if recent[#recent]~=key then
      recent[#recent+1]=key
      if #recent>12 then table.remove(recent,1) end
    end
  end

  function self:decide(obs,callback,now)
    now=now or 0
    rememberPosition(obs)
    local state=signature(obs)
    local plan=self.plan
    if plan and not B.materialChange(plan.state,state,true) and plan.remaining>0 then
      local d=DIR[plan.direction]
      local p=obs.player.position
      local tile=obs.tiles[positionKey(p.x+d[1],p.y+d[2],p.z)]
      if tile and tile.walkable then
        plan.remaining=plan.remaining-1
        plan.state=state
        callback({action='move',direction=plan.direction},nil,
          {origin='tactical',goal=plan.goal,summary='Continue bounded model-selected movement.',
           provider='none',model=self.model,knowledge_ids=plan.knowledge_ids,
           world_knowledge_ids=plan.world_knowledge_ids,
           memory_ids=plan.memory_ids,latency_ms=0})
        return
      end
    end
    self.plan=nil
    if now<self.nextCall then callback(nil,nil,{origin='cooldown'}) return end
    local knowledge=self.knowledge.retrieve(addTopics(obs),7)
    local snippets,knowledgeIds={},{}
    for _,r in ipairs(knowledge) do
      snippets[#snippets+1]={id=r.id,text=r.text}
      knowledgeIds[#knowledgeIds+1]=r.id
    end
    local memories=B.relevantMemory(self.memory,obs)
    local memoryIds={}
    for _,r in ipairs(memories) do memoryIds[#memoryIds+1]=r.id end
    local landmarks=self.world and self.world.retrieve(obs,8) or {}
    local worldIds={}
    for _,r in ipairs(landmarks) do worldIds[#worldIds+1]=r.id end
    local prompt=AgentSchema.encode({
      current_observation=B.compactObservation(obs),
      personal_memory=memories,
      current_session_history={recent_positions=self.recentPositions,
        recent_decisions=self.recentDecisions,
        note='These positions and decisions were observed or chosen in this session. Repeated backtracking without gain is evidence to reconsider the goal or route.'},
      static_world_knowledge=landmarks,
      veteran_knowledge=snippets,
      note='Priority: current observation > personal memory > static world knowledge > general veteran game knowledge. Landmarks are public leads, not proof of local terrain or NPC availability. Only current observation authorizes a target.',
    })
    local request={system=B.IDENTITY..' '..B.RULES,user=prompt,model=self.model}
    self.lastCall=now
    self.nextCall=now+8000
    local answered=false
    self.infer(request,function(content,providerError,latency)
      if answered then return end
      answered=true
      if providerError then
        self.failures=math.min(self.failures+1,3)
        self.nextCall=self.clock()+math.min(60000,15000*2^(self.failures-1))
        callback(nil,providerError,{origin='provider_failure',provider='ollama',
          model=self.model,latency_ms=latency or 0})
        return
      end
      local decision,reason=B.parse(content)
      if not decision then
        self.failures=math.min(self.failures+1,3)
        self.nextCall=self.clock()+math.min(60000,15000*2^(self.failures-1))
        callback(nil,reason,{origin='provider_failure',provider='ollama',
          model=self.model,latency_ms=latency or 0})
        return
      end
      self.failures=0
      self.decisions=self.decisions+1
      local recent=self.recentDecisions
      recent[#recent+1]={position=state.position,goal=decision.goal,
        action=decision.intent.action,
        direction=decision.intent.direction}
      if #recent>6 then table.remove(recent,1) end
      local remaining=decision.horizon-1
      if decision.intent.action=='move' and remaining==0
         and DIR[decision.intent.direction]
         and obs.player.hp>=obs.player.maxHp*0.8
         and not obs.attackId and not obs.followId then
        local otherVisible=false
        for _,creature in ipairs(obs.creatureList) do
          if creature.id~=obs.player.id then otherVisible=true; break end
        end
        local p=obs.player.position
        local step=DIR[decision.intent.direction]
        local tile=obs.tiles[positionKey(p.x+step[1],p.y+step[2],p.z)]
        if not otherVisible and tile and tile.walkable then remaining=1 end
      end
      local meta={origin='llm',provider='ollama',model=self.model,
        goal=decision.goal,summary=decision.summary,
        horizon=decision.horizon,execution_horizon=remaining+1,
        knowledge_ids=knowledgeIds,world_knowledge_ids=worldIds,
        memory_ids=memoryIds,
        latency_ms=latency or 0,decision_number=self.decisions}
      if decision.intent.action=='move' and remaining>0 then
        self.plan={direction=decision.intent.direction,
          remaining=remaining,goal=decision.goal,state=state,
          knowledge_ids=knowledgeIds,world_knowledge_ids=worldIds,
          memory_ids=memoryIds}
      end
      if decision.intent.action=='wait' then
        callback(nil,nil,meta)
      else
        callback(decision.intent,nil,meta)
      end
    end)
  end

  function self:timeout(now)
    self.failures=math.min(self.failures+1,3)
    self.nextCall=now+math.min(60000,15000*2^(self.failures-1))
    self.plan=nil
  end
  function self:onRejected()
    self.plan=nil
  end
  return self
end

return B
