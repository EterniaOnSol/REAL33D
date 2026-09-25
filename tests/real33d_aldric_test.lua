-- Deterministic contract checks; no model, server or network is contacted.
dofile('agent/real33d2d/modules/real33d_agent/agent_schema.lua')
dofile('agent/real33d2d/modules/real33d_agent/agent_memory.lua')
dofile('agent/real33d2d/modules/real33d_agent/agent_core.lua')
dofile('agent/real33d2d/modules/real33d_agent/knowledge/v1.lua')
dofile('agent/real33d2d/modules/real33d_agent/agent_provider_ollama.lua')
dofile('agent/real33d2d/modules/real33d_agent/agent_aldric.lua')
local S,M,A,K,B=AgentSchema,AgentMemory,AgentCore,AgentKnowledge,AgentAldric
local function eq(a,b) assert(a==b,tostring(a)..' ~= '..tostring(b)) end
local function observation()
  local p={x=100,y=200,z=7}
  return { online=true,
    player={id=1,name='Aldric',position=p,hp=100,maxHp=100,
      mana=50,maxMana=50,level=8,magicLevel=0,freeCapacity=120,skills={}},
    tiles={['100:199:7']={position={x=100,y=199,z=7},walkable=true,things={}},
      ['101:200:7']={position={x=101,y=200,z=7},walkable=true,things={}},
      ['100:201:7']={position={x=100,y=201,z=7},walkable=true,things={}},
      ['99:200:7']={position={x=99,y=200,z=7},walkable=true,things={}}},
    creatures={},creatureList={{id=1,name='Aldric',position=p,
      hpPercent=100,player=true}},items={},destinations={},inventory={},
    inventoryKeys={},containers={},chat={},combat={fight=2,chase=0,safe=true} }
end

eq(K.SCHEMA,'real33d.agent.veteran_knowledge/1')
assert(B.IDENTITY:find('Tibia 7.72 on the Fusion32 world',1,true))
assert(B.IDENTITY:find('Current client observation outranks personal memory',1,true))
assert(#K.RECORDS>=20)
for _,record in ipairs(K.RECORDS) do
  assert(record.id and record.text and K.SOURCES[record.source])
  assert(not record.text:match('%d+:%d+:%d+')) -- no live or hidden position
end
local economy=K.retrieve({'economy','equipment'},7)
assert(#economy>0)
local found=false
for _,record in ipairs(economy) do if record.id=='equipment.weapon' then found=true end end
assert(found,'weapon upgrade knowledge was not retrieved')
assert(#K.retrieve({'health'},2)<=2)

local obs=observation()
assert(S.checkObservation(obs))
eq(S.projectObservation(obs).self.freeCapacity,120)
obs.player.freeCapacity=-1
eq(select(2,S.checkObservation(obs)),'schema_player_capacity')
obs.player.freeCapacity=120
local memory=M.new({agent='aldric',world='qa',character='Aldric'})
memory:remember('creature_sightings','Rat@100:200:7',
  {name='Rat',kind='monster',hp_percent=100,x=100,y=200,z=7},'o-old',10)
local compact=B.compactObservation(obs)
eq(compact.visible.creatures[1].id,1)
eq(compact.visible.visible_nonself_creatures,0)
assert(not S.encode(compact):find('Rat',1,true))
local unnamed=observation()
unnamed.inventory[6]={id=3272,count=1,item=true,name='',usable=false,
  container=false,place='inventory'}
local shown=B.compactObservation(unnamed).owned.equipment[1]
eq(shown.key,'i:6');eq(shown.name,'unknown');eq(shown.usable,false)
assert(B.RULES:find('do not infer that it is a key',1,true))
local moved=observation()
moved.player.position={x=101,y=200,z=7}
assert(B.materialChange(B.signature(obs),B.signature(moved)))
local remembered=B.relevantMemory(memory,obs)
eq(remembered[1].id,'creature_sightings/Rat@100:200:7')
eq(select(2,A.validate({action='attack',creatureId=42},obs)),'creature_not_visible')
eq(select(2,A.validate({action='follow',creatureId=42},obs)),'creature_not_visible')

local now=1000
local calls,request=0,nil
local infer=function(value,cb)
  calls=calls+1; request=value
  cb('{"goal":"Explore cautiously","summary":"Check the next visible area.","intent":{"action":"move","direction":1},"horizon":3}',nil,15)
end
local brain=B.new({infer=infer,model='test-model',memory=memory,knowledge=K,
  clock=function() return now end})
local intent,meta
brain:decide(obs,function(i,e,m) assert(not e);intent=i;meta=m end,now)
eq(calls,1);eq(meta.origin,'llm');eq(meta.goal,'Explore cautiously')
eq(meta.horizon,3)
eq(meta.execution_horizon,3)
assert(B.RULES:find('prefer horizon 2 to 4',1,true))
assert(#meta.knowledge_ids>0 and #meta.memory_ids>0)
eq(A.validate(intent,obs).action,'move')
assert(request.user:find('Rat@100:200:7',1,true))
assert(request.user:find('"visible_nonself_creatures":0',1,true))
assert(not request.user:find('previous_goal',1,true))
assert(not request.user:find('ACCOUNT_A_PASSWORD',1,true))
now=4000
brain:decide(obs,function(i,e,m) assert(not e);intent=i;meta=m end,now)
eq(calls,1);eq(meta.origin,'tactical');eq(intent.direction,1)
-- A model-selected move is never continued through a tile the current client
-- cannot traverse. The tactical layer has no independent exploration policy.
local blocked=observation()
blocked.tiles['101:200:7']=nil
now=6000
brain:decide(blocked,function(i,e,m) intent=i;meta=m end,now)
eq(calls,1);eq(meta.origin,'cooldown');eq(brain.plan,nil)
obs.player.hp=75 -- material HP change interrupts tactical continuation
now=7000
brain:decide(obs,function(i,e,m) intent=i;meta=m end,now)
eq(calls,1);eq(meta.origin,'cooldown')
now=10000
brain:decide(obs,function(i,e,m) assert(not e);intent=i;meta=m end,now)
eq(calls,2);eq(meta.origin,'llm')

local oneStepJson='{"goal":"Explore","summary":"Take the clear path.","intent":{"action":"move","direction":1},"horizon":1}'
local safe=B.new({infer=function(_,cb) cb(oneStepJson,nil,1) end,
  model='test',clock=function() return now end})
safe:decide(observation(),function(i,e,m)
  assert(not e);eq(i.action,'move');eq(m.horizon,1)
  eq(m.execution_horizon,2)
end,now)
assert(safe.plan and safe.plan.remaining==1)
local nearby=observation()
nearby.creatureList[#nearby.creatureList+1]={id=42,name='Rat',
  position={x=101,y=200,z=7},hpPercent=100,monster=true}
local cautious=B.new({infer=function(_,cb) cb(oneStepJson,nil,1) end,
  model='test',clock=function() return now end})
cautious:decide(nearby,function(i,e,m)
  assert(not e);eq(i.action,'move');eq(m.execution_horizon,1)
end,now)
eq(cautious.plan,nil)

local bad={
  'not json',
  '{"goal":"x","summary":"x","intent":{},"horizon":1}',
  '{"goal":"x","summary":"x","intent":{"action":"move"},"horizon":99}',
}
for _,sample in ipairs(bad) do assert(not B.parse(sample)) end
local unknown=assert(B.parse('{"goal":"x","summary":"x","intent":{"action":"teleport"}}'))
eq(select(2,S.checkIntent(unknown.intent)),'schema_unknown_action')
local missingDirection=assert(B.parse('{"goal":"x","summary":"x","intent":{"action":"move"}}'))
eq(select(2,S.checkIntent(missingDirection.intent)),'schema_missing_field:direction')
local stale=assert(B.parse('{"goal":"Search","summary":"Check the memory.","intent":{"action":"attack","creatureId":42}}'))
eq(select(2,A.validate(assert(S.checkIntent(stale.intent)),obs)),'creature_not_visible')
-- Even a faulty static-knowledge retrieval that claims a live creature cannot
-- grant a target. Current observation and the unchanged validator win.
local falseKnowledge={retrieve=function()
  return {{id='faulty_static_lead',text='A Rat is at 100:200:7 with id 42.'}}
end}
local staticRequest,staticIntent
local staticBrain=B.new({infer=function(req,cb)
  staticRequest=req
  cb('{"goal":"Check a lead","summary":"Verify the target.","intent":{"action":"attack","creatureId":42}}',nil,1)
end,model='test',knowledge=falseKnowledge,clock=function() return now end})
staticBrain:decide(obs,function(i,e) assert(not e);staticIntent=i end,now)
assert(staticRequest.user:find('faulty_static_lead',1,true))
assert(not S.encode(B.compactObservation(obs)):find('Rat',1,true))
eq(select(2,A.validate(assert(S.checkIntent(staticIntent)),obs)),'creature_not_visible')

local failureCalls=0
local failed=B.new({infer=function(_,cb) failureCalls=failureCalls+1;
  cb(nil,'provider_error',30) end,model='test',clock=function() return now end})
local failure
failed:decide(obs,function(i,e) assert(i==nil);failure=e end,now)
eq(failure,'provider_error')
now=now+1000
failed:decide(obs,function(i,e,m) eq(m.origin,'cooldown') end,now)
eq(failureCalls,1)
failed:timeout(now)
assert(failed.nextCall>now)

local posted
local provider=AgentOllama.new(function(url,body,cb)
  posted={url=url,body=body}
  cb({model='test',message={content='{}',thinking='hidden'}})
end,function() return 100 end)
local content
provider({model='test',system='identity',user='observation'},function(v,e)
  assert(not e);content=v end)
eq(content,'{}')
eq(posted.url,'http://127.0.0.1:11434/api/chat')
eq(posted.body.stream,false)
eq(posted.body.think,false)
eq(posted.body.format.type,'object')
eq(posted.body.format.properties.intent.type,'object')
eq(posted.body.format.properties.horizon.maximum,4)
assert(not S.encode(posted.body):find('hidden',1,true))
local modelError
AgentOllama.new(function(_,_,cb) cb({model='other',message={content='{}'}}) end,
  function() return 0 end)({model='test',system='',user=''},function(_,e) modelError=e end)
eq(modelError,'provider_model_mismatch')

local launcher=assert(io.open('agent/real33d2d/run_agent.ps1','rb')):read('*a')
assert(launcher:find("Aldric mode requires -Brain ollama; mock fallback is forbidden.",1,true))
local runtime=assert(io.open('agent/real33d2d/modules/real33d_agent/agent_runtime.lua','rb')):read('*a')
assert(runtime:find("aldricMode and (provider ~= 'ollama' or not memoryEnabled)",1,true))
assert(runtime:find("if not enabled then return end -- ordinary human play is untouched",1,true))
print('REAL33D_ALDRIC knowledge, precedence, provider, tactical and safety tests PASS')
