-- Operator QA setup only. This is not Aldric's strategic Brain and never runs
-- during certification. It walks to an operator-supplied test origin through
-- currently visible, walkable tiles and the ordinary validated g_game path.
AgentPreposition = {}
local P = AgentPreposition
local DIR={{0,-1},{1,0},{0,1},{-1,0}}
local function key(x,y,z) return x..':'..y..':'..z end
local function distance(x,y,target)
  return math.abs(x-target.x)+math.abs(y-target.y)
end

function P.new(target)
  assert(type(target)=='table' and target.x and target.y and target.z)
  local self={target=target}
  function self:decide(obs,callback)
    local p=obs.player.position
    if p.z~=target.z or (p.x==target.x and p.y==target.y) then
      callback(nil,nil,{origin='qa_setup'})
      return
    end
    local queue={{x=p.x,y=p.y,first=nil,steps=0}}
    local seen={[key(p.x,p.y,p.z)]=true}
    local best,score=nil,distance(p.x,p.y,target)
    local head=1
    while head<=#queue do
      local node=queue[head]; head=head+1
      for direction=0,3 do
        local delta=DIR[direction+1]
        local x,y=node.x+delta[1],node.y+delta[2]
        local k=key(x,y,p.z)
        local tile=obs.tiles[k]
        if tile and tile.walkable and not seen[k] then
          seen[k]=true
          local nextNode={x=x,y=y,first=node.first or direction,
            steps=node.steps+1}
          queue[#queue+1]=nextNode
          local rank=distance(x,y,target)+nextNode.steps*0.01
          if rank<score then best,score=nextNode,rank end
        end
      end
    end
    if best then callback({action='move',direction=best.first},nil,
      {origin='qa_setup'})
    else callback(nil,'qa_no_visible_route',{origin='qa_setup'}) end
  end
  return self
end

return P
