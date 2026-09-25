-- Replay the bounded no-provider QA probe. No model or server is contacted.
dofile('agent/real33d2d/modules/real33d_agent/agent_schema.lua')
local path=assert(arg[1],'provider failure trace required')
local counts,start,retries={},nil,{}
for line in io.lines(path) do
  local event=assert(AgentSchema.decode(line))
  counts[event.event]=(counts[event.event] or 0)+1
  if event.event=='session_start' then start=event end
  if event.event=='provider_failure' then
    assert(event.provider=='ollama' and event.reason)
    assert(type(event.retry_after_ms)=='number' and event.retry_after_ms>=0
      and event.retry_after_ms<=60000)
    retries[#retries+1]=event.retry_after_ms
  end
end
assert(start and start.mode=='aldric' and start.mock_disabled==true)
assert(#retries>=2)
assert(retries[2]>=retries[1])
assert(not counts.intent and not counts.dispatch and not counts.brain_decision)
print(string.format('REAL33D_ALDRIC_PROVIDER_FAILURE failures=%d intents=0 dispatches=0 PASS',#retries))
