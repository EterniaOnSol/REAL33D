dofile('agent/real33d2d/modules/real33d_agent/agent_schema.lua')
dofile('agent/real33d2d/modules/real33d_agent/agent_core.lua')
dofile('agent/real33d2d/modules/real33d_agent/knowledge/v1.lua')
dofile('agent/real33d2d/modules/real33d_agent/knowledge/world_v1.lua')
dofile('agent/real33d2d/modules/real33d_agent/knowledge/real33d_772.lua')
dofile('agent/real33d2d/modules/real33d_agent/agent_veteran_planner.lua')
dofile('agent/real33d2d/modules/real33d_agent/agent_preposition.lua')
dofile('agent/real33d2d/modules/real33d_agent/agent_aldric.lua')
local K,P,B=Real33DKnowledge,AgentVeteranPlanner,AgentAldric
local function eq(a,b) assert(a==b,tostring(a)..' ~= '..tostring(b)) end
local record={knowledge_id='npc.cipfried',domain='NPCs',subject='Cipfried',
  facts={historical_home={32097,32217,7}},coordinates={32097,32217,7},
  source='npc/cipfried.npc',source_version='local selected static archive',
  compatibility='VERIFIED_772',confidence='static_home_only'}
local localKnowledge=K.new()
localKnowledge.records={record}
local rows=localKnowledge:retrieve({x=32096,y=32208,z=7},100)
eq(#rows,1);eq(rows[1].source_classification,'REAL33D_STATIC')
eq(rows[1].compatibility,'VERIFIED_772')
local mcp=K.normalizeMcp('nearby_entities','Rookgaard','Cipfried',
  {historical_home={32099,32230,7}})
eq(mcp.compatibility,'UNVERIFIED')
eq(mcp.source_classification,'SECONDARY_REFERENCE')
eq(rows[1].facts.historical_home[2],32217) -- local fact wins
eq(select(2,localKnowledge:secondary('nearby_entities','Rookgaard')),
  'mcp_unavailable')
localKnowledge.mcpQuery=function() error('offline') end
eq(select(2,localKnowledge:secondary('nearby_entities','Rookgaard')),
  'mcp_unavailable')
local many={}
for i=1,30 do
  many[#many+1]={knowledge_id='n'..i,domain='NPCs',subject='N'..i,
    facts={},coordinates={32096+i,32208,7},source='fixture',
    compatibility='VERIFIED_772'}
end
localKnowledge.records=many
eq(#localKnowledge:retrieve({x=32096,y=32208,z=7},100),12)

local function obs(x,y)
  local p={x=x or 32096,y=y or 32208,z=7}
  local tiles={}
  for direction,d in ipairs({{0,-1},{1,0},{0,1},{-1,0}}) do
    local q={x=p.x+d[1],y=p.y+d[2],z=7}
    tiles[q.x..':'..q.y..':7']={position=q,walkable=true,things={}}
  end
  return {online=true,player={id=1,name='Aldric',position=p,hp=100,
    maxHp=100,mana=0,maxMana=0,level=2,magicLevel=0,
    freeCapacity=100,skills={}},tiles=tiles,creatures={},
    creatureList={{id=1,name='Aldric',position=p,player=true,hpPercent=100}},
    items={},destinations={},inventory={},inventoryKeys={},containers={},
    chat={},combat={fight=2,chase=0,safe=true},
    shop={open=false,offers={},goods={}}}
end
local p=P.new()
local function decision(status,goal,subgoal,target)
  return {goal_status=status,high_level_goal=goal,goal_reason='earn resources',
    current_plan={'reach known low-level area','hunt only visible creatures'},
    current_subgoal=subgoal,target_region_or_landmark='known area',
    target_position=target,progress_metric='resource count',
    progress_kind='resources',
    progress_assessment='not yet there',replan_condition='blocked route',
    selected_intent={action='move',direction=1}}
end
p:observe(obs(),1000)
local falseSafety=decision('new','seek safety','move to open tiles')
falseSafety.progress_kind='survival'
eq(select(2,p:accept(falseSafety,1000)),
  'provider_survival_without_observed_risk')
assert(p:accept(decision('new','earn resources','reach area',
  {x=32115,y=32205,z=7}),1000))
p:recordAction({action='move'})
p:observe(obs(32097,32208),2000)
assert(p:accept(decision('maintain','earn resources','reach area',
  {x=32115,y=32205,z=7}),2000))
eq(p.current_goal,'earn resources');eq(p.current_subgoal,'reach area')
local noRepeatedTarget=decision('maintain','earn resources','reach area')
assert(p:accept(noRepeatedTarget,2000,{record}))
eq(p.target_position.x,32115)
eq(select(2,p:accept(decision('maintain','explore academy','reach area'),2000)),
  'provider_goal_changed_without_replan')
eq(select(2,p:accept(decision('replan','earn resources','reach area',
  {x=32115,y=32205,z=7}),2000)), 'provider_replan_without_change')
local fakeTarget=decision('maintain','earn resources','reach area',
  {x=32116,y=32205,z=7})
eq(select(2,p:accept(fakeTarget,2000,{record})),
  'provider_unverified_target_position')
for i=1,12 do p:recordAction({action='move'}) end
p:observe(obs(32097,32208),90000)
eq(p.replan_reason,'movement_only_streak')
eq(select(2,p:accept(decision('maintain','earn resources','reach area'),90000)),
  'provider_replan_required')
assert(p:accept(decision('replan','earn resources','find alternate entrance',
  {x=32106,y=32218,z=7}),90000))
eq(p.replan_reason,nil);eq(p.subgoal_transitions,2)
for i=1,3 do p:rejected('tile_not_visible_walkable') end
p:observe(obs(32097,32208),91000)
eq(p.replan_reason,'repeated_blocked_move')
local oscillating=P.new()
oscillating:observe(obs(1,1),1000)
for i=1,5 do oscillating:observe(obs(1,1),1000+i) end
eq(oscillating.replan_reason,nil) -- stationary polling is not backtracking
for i=1,5 do
  oscillating:observe(obs(2,1),2000+i*2)
  oscillating:observe(obs(1,1),2001+i*2)
end
eq(oscillating.replan_reason,'position_oscillation')
eq(select(2,AgentCore.validate({action='attack',creatureId=42},obs())),
  'creature_not_visible')

local lootObs=obs()
local corpseKey='t:32097:32208:7:1'
lootObs.tiles['32097:32208:7'].things={{key=corpseKey,
  name='dead rat',container=true}}
lootObs.items[corpseKey]={item=true,container=true,count=1}
lootObs.containers={{id=1,name='bag',capacity=4,sourcePlace='carried',items={}},
  {id=2,name='dead rat',capacity=2,sourcePlace='tile',items={{key='c:2:0',
    id=3031,name='gold coin',count=2,container=false}}}}
lootObs.items['c:2:0']={item=true,count=2}
lootObs.destinations['c:1:0']={place='container',id=1,slot=0}
local actions=B.affordances(lootObs)
local foundOpen,foundLoot=false,false
for _,action in ipairs(actions) do
  if action.action=='open_container' and action.item==corpseKey then
    foundOpen=true
  end
  if action.action=='move_item' and action.item=='c:2:0'
     and action.destination=='c:1:0' then foundLoot=true end
end
assert(foundOpen and foundLoot,'visible corpse and loot must be actionable')

local setup=AgentPreposition.new({x=32096,y=32208,z=7})
local setupAction
setup:decide(obs(32097,32215),function(intent) setupAction=intent end)
eq(setupAction.action,'move');eq(setupAction.direction,0)
setup:decide(obs(32096,32208),function(intent) setupAction=intent end)
eq(setupAction,nil)
eq(select(2,AgentCore.validate({action='attack',creatureId=42},lootObs)),
  'creature_not_visible')

local answer={goal='earn resources',summary='Look for a known hunt.',
  goal_status='new',high_level_goal='earn resources',goal_reason='low cash',
  current_plan={'reach a suitable area','hunt visible creatures'},
  current_subgoal='reach area',target_region_or_landmark='rat area',
  target_position={x=32097,y=32208,z=7},progress_metric='distance',
  progress_kind='resources',
  progress_assessment='beginning',replan_condition='blocked route',
  selected_intent={action='move',direction=1},horizon=1}
local request
local brain=B.new({infer=function(req,cb)
  request=req;cb(AgentSchema.encode(answer),nil,1)
end,model='qwen3:4b',clock=function() return 1000 end,
  veteran002=true,localKnowledge=localKnowledge})
local intent,meta
brain:decide(obs(),function(i,e,m) assert(not e);intent=i;meta=m end,1000)
eq(intent.action,'move');eq(meta.planner.current_goal,'earn resources')
assert(request.veteran002)
assert(request.system:find('direction 0 is north',1,true))
assert(request.system:find('1 is east',1,true))
assert(request.system:find('unknown name or ID',1,true))
assert(request.user:find('available_actions',1,true))
assert(request.user:find('real33d_772_static_knowledge',1,true))
assert(request.user:find('movement_only_streak',1,true))
assert(not request.user:find('ACCOUNT_A_PASSWORD',1,true))
print('REAL33D_VETERAN_PLANNER knowledge, persistence and safety PASS')
