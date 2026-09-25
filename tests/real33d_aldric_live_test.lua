-- Replay the retained Aldric JSONL trace; this never calls the provider.
-- Usage: luajit tests/real33d_aldric_live_test.lua evidence/agent/aldric/live.jsonl
dofile('agent/real33d2d/modules/real33d_agent/agent_schema.lua')
local S=AgentSchema
local path=assert(arg[1],'live JSONL path required')
local events,byObservation={},{}
for line in io.lines(path) do
  local event,reason=S.decode(line)
  assert(event,path..': '..tostring(reason))
  events[#events+1]=event
  if event.event=='observation' then
    byObservation[event.observation_id]=event.observation
    assert(event.observation.memory==nil)
    assert(event.observation.server==nil and event.observation.spawn==nil)
  end
end
local starts,ends,decisions,intents,dispatches,results,failures={},{},{},{},{},{},{}
local validation={}
for _,event in ipairs(events) do
  local kind=event.event
  if kind=='session_start' then starts[#starts+1]=event
  elseif kind=='session_end' then ends[#ends+1]=event
  elseif kind=='brain_decision' then decisions[#decisions+1]=event
  elseif kind=='intent' then intents[#intents+1]=event
  elseif kind=='dispatch' then dispatches[#dispatches+1]=event
  elseif kind=='result' then results[#results+1]=event
  elseif kind=='provider_failure' then failures[#failures+1]=event
  elseif kind=='validation' then
    local key=event.correlation_id
    validation[key]=validation[key] or {}
    validation[key][event.stage]=event
  end
  assert(kind~='protocol_error')
end
assert(#starts==1 and #ends==1,'one normal live session required')
local start,finish=starts[1],ends[1]
assert(start.mode=='aldric' and start.brain=='ollama' and start.provider=='ollama')
assert(start.mock_disabled==true and start.model and start.knowledge_schema)
assert(start.memory.state=='loaded' and start.memory.records>0)
assert(finish.memory_saved and finish.memory_records>=start.memory.records)
assert(#decisions>=3,'several real model decisions required')
local goals={}
local namedNpcGoals=0
for _,decision in ipairs(decisions) do
  assert(decision.provider=='ollama' and decision.model==start.model)
  assert(decision.goal and decision.summary and decision.latency_ms)
  assert(type(decision.horizon)=='number' and decision.horizon>=1
    and decision.horizon<=4)
  assert(decision.knowledge_ids and #decision.knowledge_ids>0)
  assert(decision.memory_ids and #decision.memory_ids>0)
  assert(byObservation[decision.observation_id])
  local observed=byObservation[decision.observation_id]
  for _,name in ipairs({'Seymour','The Oracle'}) do
    if decision.goal:find(name,1,true) then
      local seen=false
      for _,creature in ipairs(observed.visible.creatures) do
        if creature.name==name then seen=true end
      end
      assert(seen,'NPC goal must be grounded in current visibility')
      namedNpcGoals=namedNpcGoals+1
    end
  end
  goals[decision.goal]=true
end
local goalCount=0
for _ in pairs(goals) do goalCount=goalCount+1 end
assert(goalCount>=2,'genuinely different model goals required')
local llmActions,tacticalActions,authoritativeMoves,openedContainer=0,0,0,false
for _,intent in ipairs(intents) do
  assert(byObservation[intent.observation_id])
  if intent.source=='llm' then llmActions=llmActions+1 end
  if intent.source=='tactical' then tacticalActions=tacticalActions+1 end
  assert(intent.source=='llm' or intent.source=='tactical')
  local stages=validation[intent.correlation_id]
  assert(stages and (stages.schema or stages.freshness),'every intent needs a gate')
  if stages.freshness then assert(not stages.freshness.accepted) end
  if stages.schema and stages.schema.accepted then assert(stages.state,'state validation missing') end
  if stages.state and stages.state.accepted then assert(stages.budget,'budget missing') end
end
assert(llmActions>0,'a model proposal must reach the action gate')
assert(tacticalActions>0,'bounded local movement must run without a model call')
for _,dispatch in ipairs(dispatches) do
  assert(dispatch.accepted)
  if dispatch.action=='move' then assert(dispatch.path=='g_game.walk') end
  assert(type(dispatch.path)=='string' and dispatch.path:match('^g_game%.'))
  local stages=assert(validation[dispatch.correlation_id])
  assert(stages.schema.accepted and stages.state.accepted and stages.budget.accepted)
end
for _,result in ipairs(results) do
  assert(byObservation[result.observed_in])
  if result.action=='move' and result.authoritative_change then
    for _,change in ipairs(result.changed) do
      if change.field=='x' or change.field=='y' or change.field=='z' then
        authoritativeMoves=authoritativeMoves+1
        break
      end
    end
  end
  if result.action=='use' and result.authoritative_change then
    for _,change in ipairs(result.changed) do
      if change.field=='containers' then openedContainer=true end
    end
  end
end
assert(authoritativeMoves>0,'Fusion32-confirmed autonomous movement required')
print(string.format('REAL33D_ALDRIC_LIVE decisions=%d goals=%d llm_actions=%d tactical_actions=%d dispatches=%d moves=%d opened_container=%d npc_goals=%d provider_failures=%d PASS',
  #decisions,goalCount,llmActions,tacticalActions,#dispatches,authoritativeMoves,
  openedContainer and 1 or 0,namedNpcGoals,#failures))
