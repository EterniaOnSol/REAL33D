-- One provider implementation behind Aldric's infer(request, callback)
-- contract. No observation, memory, validator or game API depends on Ollama.
AgentOllama = {}

-- Ollama constrains the outer decision shape; the existing AgentSchema and
-- AgentCore validator still decide whether a particular action is legal.
local DECISION_FORMAT = {
  type='object', required={'goal','summary','intent','horizon'},
  properties={
    goal={type='string',maxLength=120},
    summary={type='string',maxLength=180},
    intent={type='object',required={'action'},properties={
      action={type='string'},direction={type='integer',minimum=0,maximum=3},
      creatureId={type='integer'},text={type='string'},item={type='string'},
      targetItem={type='string'},targetCreatureId={type='integer'},
      destination={type='string'},offer={type='string'},count={type='integer'},
      fight={type='integer'},chase={type='integer'},safe={type='boolean'},
    }},
    horizon={type='integer',minimum=1,maximum=4},
  },
}

function AgentOllama.new(post, clock)
  assert(type(post)=='function','HTTP post function required')
  clock=clock or function() return 0 end
  return function(request,callback)
    local started=clock()
    local body={ model=request.model,stream=false,format=DECISION_FORMAT,think=false,
      options={temperature=0.35,num_predict=180,num_ctx=4096},
      messages={ {role='system',content=request.system},
                 {role='user',content=request.user} } }
    local ok=pcall(post,'http://127.0.0.1:11434/api/chat',body,
      function(response,failure)
        local latency=math.max(0,clock()-started)
        if failure or type(response)~='table' or type(response.message)~='table'
           or type(response.message.content)~='string' then
          callback(nil,'provider_error',latency)
          return
        end
        if response.model and response.model~=request.model then
          callback(nil,'provider_model_mismatch',latency)
          return
        end
        callback(response.message.content,nil,latency)
      end)
    if not ok then callback(nil,'provider_error',
      math.max(0,clock()-started)) end
  end
end

return AgentOllama
