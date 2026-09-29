-- Durable strategic state and diagnostics. No goal, hunt, route or economic
-- decision is selected here; only the model may choose those.
AgentVeteranPlanner={}
local P=AgentVeteranPlanner

local function key(p) return p.x..':'..p.y..':'..p.z end
local function distance(a,b)
  if not a or not b or a.z~=b.z then return nil end
  return math.abs(a.x-b.x)+math.abs(a.y-b.y)
end
local function short(s,n)
  return type(s)=='string' and #s>0 and #s<=n
    and not s:find('[%z\1-\31]')
end

function P.new()
  local self={current_goal=nil,goal_reason=nil,current_plan=nil,
    current_subgoal=nil,target_region_or_landmark=nil,target_position=nil,
    progress_metric=nil,progress_kind=nil,started_at=nil,last_progress_at=nil,
    failed_attempts=0,replan_reason=nil,movement_only_streak=0,
    repeated_position_count=0,blocked_moves=0,position_counts={},
    last_position=nil,last_distance=nil,last_signature=nil,
    last_action=nil,subgoal_transitions=0,
    last_goal_status=nil,last_replan_trigger=nil,
    last_progress_assessment=nil,current_risk=false}
  function self:observe(obs,now)
    local p=obs.player.position
    local k=key(p)
    local previous=self.last_position
    -- A stationary client may yield many observations. Count revisits after
    -- movement, not timer ticks, or idling falsely looks like oscillation.
    if previous~=k then
      self.position_counts[k]=(self.position_counts[k] or 0)+1
      self.repeated_position_count=self.position_counts[k]
      self.last_position=k
    end
    local d=distance(p,self.target_position)
    if d and (not self.last_distance or d<self.last_distance) then
      self.last_progress_at=now
      self.last_distance=d
    end
    local signature={hp=obs.player.hp,capacity=obs.player.freeCapacity,
      level=obs.player.level,containers=#obs.containers,
      attack=obs.attackId or 0,creatures=#obs.creatureList}
    self.current_risk=obs.player.hp<obs.player.maxHp*0.8
    for _,creature in ipairs(obs.creatureList) do
      if creature.id~=obs.player.id and creature.monster then
        self.current_risk=true
      end
    end
    local old=self.last_signature
    if old and (signature.level>old.level or signature.capacity~=old.capacity
       or signature.containers~=old.containers or signature.attack~=old.attack
       or signature.creatures~=old.creatures) then
      self.last_progress_at=now
      self.movement_only_streak=0
    end
    self.last_signature=signature
    if self.blocked_moves>=3 then self.replan_reason='repeated_blocked_move'
    elseif self.movement_only_streak>=12 and self.last_progress_at
       and now-self.last_progress_at>60000 then
      self.replan_reason='movement_only_streak'
    elseif self.repeated_position_count>=5 then self.replan_reason='position_oscillation'
    elseif self.movement_only_streak>=8 and self.last_progress_at
       and now-self.last_progress_at>60000 then
      self.replan_reason='no_progress_toward_subgoal'
    end
  end
  function self:accept(d,now,allowedTargets)
    if d.goal_status~='new' and d.goal_status~='maintain'
       and d.goal_status~='replan' and d.goal_status~='completed' then
      return nil,'provider_invalid_goal_status'
    end
    if not short(d.high_level_goal,120) then
      return nil,'provider_invalid_high_level_goal'
    end
    if not short(d.goal_reason,180) then return nil,'provider_invalid_goal_reason' end
    if type(d.current_plan)~='table' or #d.current_plan<1
       or #d.current_plan>4 then return nil,'provider_invalid_plan_steps' end
    if not short(d.current_subgoal,120) then
      return nil,'provider_invalid_subgoal'
    end
    if not short(d.progress_assessment,180) then
      return nil,'provider_invalid_progress_assessment'
    end
    if not short(d.replan_condition,180) then
      return nil,'provider_invalid_replan_condition'
    end
    if not short(d.progress_metric,120) then
      return nil,'provider_invalid_progress_metric'
    end
    if d.progress_kind~='resources' and d.progress_kind~='level'
       and d.progress_kind~='skills' and d.progress_kind~='equipment'
       and d.progress_kind~='supplies' and d.progress_kind~='survival' then
      return nil,'provider_invalid_progress_kind'
    end
    if d.progress_kind=='survival' and not self.current_risk then
      return nil,'provider_survival_without_observed_risk'
    end
    for _,step in ipairs(d.current_plan) do
      if not short(step,120) then return nil,'provider_invalid_plan_step' end
    end
    local target=d.target_position
    -- Omitting an established target while maintaining the same subgoal does
    -- not silently abandon it or reset the lack-of-progress history.
    if target==nil and self.current_goal==d.high_level_goal
       and self.current_subgoal==d.current_subgoal then
      target=self.target_position
    end
    if target~=nil and (type(target)~='table' or type(target.x)~='number'
       or type(target.y)~='number' or type(target.z)~='number'
       or target.x~=math.floor(target.x) or target.y~=math.floor(target.y)
       or target.z~=math.floor(target.z)) then
      return nil,'provider_invalid_target_position'
    end
    if target and allowedTargets then
      local verified=self.target_position and key(target)==key(self.target_position)
      for _,row in ipairs(allowedTargets) do
        local p=row.coordinates
        if p and p[1]==target.x and p[2]==target.y
           and p[3]==target.z then verified=true; break end
      end
      if not verified then return nil,'provider_unverified_target_position' end
    end
    if self.current_goal and d.goal_status=='maintain'
       and d.high_level_goal~=self.current_goal then
      return nil,'provider_goal_changed_without_replan'
    end
    local changed=not self.current_goal
      or d.high_level_goal~=self.current_goal
      or d.current_subgoal~=self.current_subgoal
      or ((target or self.target_position) and
          (not target or not self.target_position or
           key(target)~=key(self.target_position)))
    if self.current_goal and d.goal_status=='replan' and not changed then
      return nil,'provider_replan_without_change'
    end
    if self.replan_reason and d.selected_intent.action=='move' then
      if d.goal_status~='replan' or not changed then
        return nil,'provider_replan_required'
      end
    end
    local trigger=self.replan_reason
    local newGoal=not self.current_goal or d.high_level_goal~=self.current_goal
    local newSubgoal=d.current_subgoal~=self.current_subgoal
    if newGoal then self.started_at=now end
    if newSubgoal then self.subgoal_transitions=self.subgoal_transitions+1 end
    if changed then
      self.movement_only_streak=0
      self.position_counts={}
      self.repeated_position_count=0
      self.blocked_moves=0
      self.last_distance=nil
      self.last_progress_at=now
      self.replan_reason=nil
    end
    self.current_goal=d.high_level_goal
    self.goal_reason=d.goal_reason
    self.current_plan=d.current_plan
    self.current_subgoal=d.current_subgoal
    self.target_region_or_landmark=d.target_region_or_landmark
    self.target_position=target
    self.progress_metric=d.progress_metric
    self.progress_kind=d.progress_kind
    self.last_goal_status=d.goal_status
    self.last_replan_trigger=trigger
    self.last_progress_assessment=d.progress_assessment
    return true
  end
  function self:recordAction(intent)
    if intent.action=='move' then
      self.movement_only_streak=self.movement_only_streak+1
    else self.movement_only_streak=0 end
    self.last_action=intent.action
  end
  function self:rejected(reason)
    self.failed_attempts=self.failed_attempts+1
    if reason=='tile_not_visible_walkable' or reason=='dispatch_failed' then
      self.blocked_moves=self.blocked_moves+1
    end
  end
  function self:snapshot()
    return {current_goal=self.current_goal,goal_reason=self.goal_reason,
      current_plan=self.current_plan,current_subgoal=self.current_subgoal,
      target_region_or_landmark=self.target_region_or_landmark,
      target_position=self.target_position,progress_metric=self.progress_metric,
      progress_kind=self.progress_kind,current_risk=self.current_risk,
      started_at=self.started_at,last_progress_at=self.last_progress_at,
      failed_attempts=self.failed_attempts,replan_reason=self.replan_reason,
      movement_only_streak=self.movement_only_streak,
      repeated_position_count=self.repeated_position_count,
      blocked_moves=self.blocked_moves,subgoal_transitions=self.subgoal_transitions,
      last_goal_status=self.last_goal_status,
      last_replan_trigger=self.last_replan_trigger,
      last_progress_assessment=self.last_progress_assessment}
  end
  return self
end

return P
