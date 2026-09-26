-- Public, player-level landmarks. Coordinates are historical leads, never
-- navigable tiles or proof of current occupants. No server/runtime source.
AgentWorldKnowledge = {}
local W = AgentWorldKnowledge
W.SCHEMA = 'real33d.agent.static_world_knowledge/1'
W.VERSION = '2026-09-25-v1'
W.SOURCES = {
  oracle = 'https://www.tibiawiki.com.br/index.php?stableid=355562&title=The_Oracle',
  cipfried = 'https://www.tibiawiki.com.br/index.php?stableid=179425&title=Cipfried',
  academy = 'https://www.tibiawiki.com.br/wiki/Rookgaard_Academy',
  thais = 'https://www.tibiawiki.com.br/Thais',
  thais_street = 'https://www.tibiawiki.com.br/wiki/Temple_Street',
  obi = 'https://www.tibiawiki.com.br/wiki/Obi',
  tom = 'https://www.tibiawiki.com.br/wiki/Tom',
  al_dee = 'https://www.tibiawiki.com.br/wiki/Al_Dee',
  troll_cave = 'https://www.tibiawiki.com.br/wiki/Troll_Cave_%28Rookgaard%29',
}
W.RECORDS = {
  {id='rookgaard.oracle', name='The Oracle chamber, Rookgaard Academy',
   x=32104,y=32189,z=6,radius=6,cue='The Oracle',source='oracle',
   text='The Oracle is a progression NPC in the upper academy area of Rookgaard, not a hunting creature or a source of supplies. A level-one character usually needs training and resources before the Oracle is useful. Exact travel prerequisites must be confirmed in play.'},
  {id='rookgaard.academy',name='Rookgaard Academy',
   x=32104,y=32193,z=7,radius=5,cue='Seymour',source='academy',
   text='The academy is north of Rookgaard temple. Its upper floor contains The Oracle.'},
  {id='rookgaard.temple',name='Rookgaard temple',
   x=32097,y=32219,z=7,radius=5,cue='Cipfried',source='cipfried',
   text='Cipfried is associated with Rookgaard temple. The academy lies to its north.'},
  {id='rookgaard.obi',name='Obi weapon shop',
   x=32110,y=32203,z=7,radius=4,cue='Obi',source='obi',
   text='Obi is known as a weapon merchant east of Rookgaard Academy. Verify current offers by talking to him.'},
  {id='rookgaard.tom',name='Tom the tanner',
   x=32085,y=32198,z=7,radius=4,cue='Tom',source='tom',
   text='Tom is known as a tanner west of Rookgaard Academy. Verify what he buys in a current conversation.'},
  {id='rookgaard.al_dee',name='Al Dee tool shop',
   x=32063,y=32182,z=7,radius=4,cue='Al Dee',source='al_dee',
   text='Al Dee is known as a tool merchant west of the central Rookgaard shops. Verify current offers.'},
  {id='rookgaard.troll_cave',name='Rookgaard Troll Cave entrance',
   x=32094,y=32138,z=7,radius=5,cue=nil,source='troll_cave',
   text='A cave north of Rookgaard is known for low-level trolls and other dangers deeper below. Current occupants are unknown.'},
  {id='thais.temple',name='Thais temple',
   x=32369,y=32241,z=7,radius=5,cue='Quentin',source='thais',
   text='Quentin is associated with the temple in central Thais, near Temple Street.'},
}

local function visibleCue(obs,cue)
  if not cue then return false end
  for _,creature in ipairs(obs.creatureList or {}) do
    if creature.name == cue then return true end
  end
  return false
end

-- Regional leads are public background. Only lookup() reports a local match.
function W.retrieve(obs,limit)
  local p=obs and obs.player and obs.player.position
  if not p then return {} end
  local candidates={}
  for _,record in ipairs(W.RECORDS) do
    local distance=math.abs(p.x-record.x)+math.abs(p.y-record.y)
    local sameRegion=(record.id:find('rookgaard.',1,true)==1 and
      math.abs(p.x-32090)<180 and math.abs(p.y-32190)<180)
      or (record.id:find('thais.',1,true)==1 and
      math.abs(p.x-32369)<180 and math.abs(p.y-32215)<180)
    if sameRegion then
      local localMatch=p.z==record.z and distance<=record.radius
      local cue=localMatch and visibleCue(obs,record.cue)
      candidates[#candidates+1]={id=record.id,name=record.name,
        source=record.source,text=record.text,
        reference={x=record.x,y=record.y,z=record.z},
        approximate_distance=distance,
        bearing=(record.y<p.y and 'north' or record.y>p.y and 'south' or '')..
          (record.x<p.x and 'west' or record.x>p.x and 'east' or ''),
        visible_cue=cue and true or false,
        match=localMatch and (cue and 'position_and_visible_cue' or 'position_only')
          or 'regional_lead',distance=distance}
    end
  end
  table.sort(candidates,function(a,b)
    local order={position_and_visible_cue=1,position_only=2,regional_lead=3}
    if order[a.match]~=order[b.match] then return order[a.match]<order[b.match] end
    if a.distance~=b.distance then return a.distance<b.distance end
    return a.id<b.id
  end)
  local result={}
  for i=1,math.min(limit or 6,#candidates) do
    candidates[i].distance=nil
    result[#result+1]=candidates[i]
  end
  return result
end

function W.lookup(obs)
  local found={}
  if not obs or not obs.player or not obs.player.position then return found end
  local p=obs.player.position
  for _,record in ipairs(W.RECORDS) do
    if p.z==record.z and math.abs(p.x-record.x)+math.abs(p.y-record.y)<=record.radius then
      found[#found+1]={id=record.id,name=record.name,source=record.source,
        text=record.text,reference={x=record.x,y=record.y,z=record.z},
        visible_cue=visibleCue(obs,record.cue),
        match=visibleCue(obs,record.cue) and 'position_and_visible_cue' or 'position_only'}
    end
  end
  table.sort(found,function(a,b) return a.id<b.id end)
  return found
end

return W
