dofile('agent/real33d2d/modules/real33d_agent/agent_schema.lua')
dofile('agent/real33d2d/modules/real33d_agent/agent_core.lua')
dofile('agent/real33d2d/modules/real33d_agent/knowledge/v1.lua')
dofile('agent/real33d2d/modules/real33d_agent/knowledge/world_v1.lua')
dofile('agent/real33d2d/modules/real33d_agent/agent_aldric.lua')
local W,B,S,A=AgentWorldKnowledge,AgentAldric,AgentSchema,AgentCore
assert(W.SCHEMA=='real33d.agent.static_world_knowledge/1')
for _,record in ipairs(W.RECORDS) do
  assert(W.SOURCES[record.source] and record.x and record.y and record.z)
  assert(not record.text:find('spawn',1,true))
end
local obs={player={id=1,name='Aldric',position={x=32099,y=32189,z=6},
  hp=150,maxHp=150,mana=0,maxMana=0,level=1,magicLevel=0,
  freeCapacity=300,skills={}},creatureList={{id=1,name='Aldric',
  position={x=32099,y=32189,z=6},hpPercent=100,player=true},
  {id=8,name='The Oracle',position={x=32104,y=32190,z=6},
   hpPercent=100,monster=true}},tiles={},containers={},inventory={},
  inventoryKeys={},chat={},combat={fight=2,chase=0,safe=true},
  items={},destinations={},creatures={},online=true}
local match=W.lookup(obs)
assert(#match==1 and match[1].id=='rookgaard.oracle')
assert(match[1].match=='position_and_visible_cue')
assert(not W.lookup({player={position={x=32099,y=32189,z=7}}})[1])
obs.creatureList[2]=nil
assert(W.lookup(obs)[1].match=='position_only')
local regional=W.retrieve(obs,8)
local huntingLead=false
for _,row in ipairs(regional) do
  if row.id=='rookgaard.troll_cave' then huntingLead=true end
end
assert(huntingLead,'regional hunting lead omitted')
assert(regional[1].approximate_distance and regional[1].bearing)
local map=B.compactObservation(obs).visible.local_map
assert(#map.rows==7 and map.rows[4]:sub(4,4)=='@')
assert(map.rows[1]:find('?',1,true),'unseen cells must be marked unknown')
local request
local brain=B.new({infer=function(value,cb) request=value
  cb('{"goal":"Orient","summary":"Check the academy area.","intent":{"action":"wait"}}',nil,1)
end,model='test',world=W,clock=function() return 1000 end})
brain:decide(obs,function(intent,error,meta)
  assert(not intent and not error)
  assert(meta.world_knowledge_ids[1]=='rookgaard.oracle')
end,1000)
assert(request.user:find('rookgaard.oracle',1,true))
assert(request.user:find('position_only',1,true))
assert(request.user:find('current_session_history',1,true))
assert(request.user:find('32099:32189:6',1,true))
assert(request.system:find('Progress your character intelligently',1,true))
assert(request.system:find('static world knowledge',1,true))
assert(request.system:find('repeated backtracking',1,true))
local long=B.parse('{"goal":"'..string.rep('a',125)..'","summary":"'..
  string.rep('b',190)..'","intent":{"action":"wait"}}')
assert(long and #long.goal==120 and #long.summary==180)
assert(select(2,A.validate({action='attack',creatureId=8},obs))=='creature_not_visible')
obs.shop={open=true,money=80,offers={{key='s:1',id=100,name='Supplies',
  weight=2,buyPrice=20,sellPrice=4}},goods={['100']=3}}
assert(S.checkObservation(obs))
local before=B.signature(obs).shop
local projected=S.projectObservation(obs)
assert(projected.shop.offers and B.signature(obs).shop==before)
assert(A.validate(assert(S.checkIntent({action='buy',offer='s:1',count=2})),obs))
assert(select(2,A.validate({action='buy',offer='s:1',count=5},obs))
  =='insufficient_observed_money')
assert(A.validate(assert(S.checkIntent({action='sell',offer='s:1',count=2})),obs))
assert(select(2,A.validate({action='sell',offer='s:1',count=4},obs))
  =='insufficient_observed_goods')
assert(select(2,A.validate({action='buy',offer='s:9',count=1},obs))
  =='offer_not_current')
obs.shop.open=false
assert(select(2,A.validate({action='buy',offer='s:1',count=1},obs))
  =='shop_not_open')
obs.shop.offers={}
before=B.signature(obs).shop
S.projectObservation(obs)
assert(B.signature(obs).shop==before,'projection mutated empty shop offers')
print('REAL33D_VETERAN static landmark and observation priority PASS')
