-- Player-level static facts only. The local index is generated privately from
-- the installed, selected Fusion32 static archive; it is never a live view.
Real33DKnowledge = {}
local K = Real33DKnowledge
K.SCHEMA = 'real33d.knowledge.adapter/1'
K.COMPATIBILITY = { VERIFIED_772=true,HISTORICAL_MATCH=true,
  MODERN_ONLY=true,VERSION_MISMATCH=true,UNVERIFIED=true }

local function distance(a,p)
  return math.abs(a[1]-p.x)+math.abs(a[2]-p.y)
end

function K.loadLocal(path)
  if type(path)~='string' or path=='' then return {},'local_index_unconfigured' end
  local file=io.open(path,'rb')
  if not file then return {},'local_index_unavailable' end
  local raw=file:read('*a'); file:close()
  if #raw>200000 then return {},'local_index_oversize' end
  local data,reason=AgentSchema.decode(raw)
  if type(data)~='table' or data.schema~='real33d.knowledge.local_772/1'
     or type(data.records)~='table' then
    return {},'local_index_invalid:'..tostring(reason)
  end
  local result={}
  for _,r in ipairs(data.records) do
    if type(r)=='table' and type(r.knowledge_id)=='string'
       and type(r.subject)=='string' and type(r.domain)=='string'
       and type(r.coordinates)=='table' and type(r.coordinates[1])=='number'
       and type(r.coordinates[2])=='number' and type(r.coordinates[3])=='number'
       and r.compatibility=='VERIFIED_772' and type(r.source)=='string'
       and #result<200 then
      result[#result+1]=r
    end
  end
  return result,nil
end

-- Normalize an MCP answer. Modern TibiaWiki is always secondary until a
-- separate local compatibility check supplies a verified record.
function K.normalizeMcp(tool,query,entity,facts)
  if type(tool)~='string' or type(query)~='string'
     or type(entity)~='string' or type(facts)~='table' then
    return nil,'mcp_invalid_response'
  end
  return {knowledge_id='mcp.'..tool..'.'..entity,domain='reference',
    subject=entity,facts=facts,source='miltonhit/tibia_mcp:'..tool,
    source_version='TibiaWiki current snapshot',target_version='7.72',
    compatibility='UNVERIFIED',confidence='secondary_reference',
    query=query,source_classification='SECONDARY_REFERENCE'}
end

function K.new(options)
  options=options or {}
  local records,loadError=K.loadLocal(options.localFile)
  local self={records=records,loadError=loadError,mcpQuery=options.mcpQuery,
    mcpErrors=0}
  function self:retrieve(position,limit,domain)
    if type(position)~='table' then return {} end
    limit=math.max(0,math.min(tonumber(limit) or 8,12))
    local candidates={}
    for _,r in ipairs(self.records) do
      if (not domain or r.domain==domain) then
        local d=distance(r.coordinates,position)
        if d<=120 then
          candidates[#candidates+1]={knowledge_id=r.knowledge_id,
            domain=r.domain,subject=r.subject,facts=r.facts,
            coordinates=r.coordinates,source=r.source,
            source_sha256=r.source_sha256,
            source_version=r.source_version,target_version='7.72',
            compatibility=r.compatibility,confidence=r.confidence,
            source_classification='REAL33D_STATIC',
            distance=d,floor_delta=r.coordinates[3]-position.z}
        end
      end
    end
    table.sort(candidates,function(a,b)
      local af,bf=math.abs(a.floor_delta),math.abs(b.floor_delta)
      if af~=bf then return af<bf end
      if a.distance~=b.distance then return a.distance<b.distance end
      return a.knowledge_id<b.knowledge_id
    end)
    local result={}
    for i=1,math.min(limit,#candidates) do result[i]=candidates[i] end
    return result
  end
  function self:secondary(tool,query)
    if not self.mcpQuery then return {},'mcp_unavailable' end
    local ok,answer=pcall(self.mcpQuery,tool,query)
    if not ok or type(answer)~='table' then
      self.mcpErrors=self.mcpErrors+1
      return {},'mcp_unavailable'
    end
    local result={}
    for i=1,math.min(#answer,4) do
      local row=answer[i]
      local normalized=K.normalizeMcp(tool,query,row.entity,row.facts,false)
      if normalized then result[#result+1]=normalized end
    end
    return result,nil
  end
  return self
end

return K
