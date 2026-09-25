-- Curated player-level concepts for Aldric. This is static, read-only knowledge;
-- no map, NPC stock, price, creature position or current server fact lives here.
AgentKnowledge = {}
local K = AgentKnowledge
K.SCHEMA = 'real33d.agent.veteran_knowledge/1'
K.VERSION = '2026-09-24-v1'
K.SOURCES = {
  manual_characters = 'https://www.tibia.com/gameguides/?section=characters&subtopic=manual',
  manual_combat = 'https://www.tibia.com/gameguides/?section=combat&subtopic=manual',
  manual_world = 'https://www.tibia.com/gameguides/?section=world&subtopic=manual',
  manual_trading = 'https://www.tibia.com/gameguides/?section=controls_trading&subtopic=manual',
  manual_communication = 'https://www.tibia.com/gameguides/?section=controls_communication&subtopic=manual',
}

-- The official manual is current, not a 7.72 snapshot. Only durable concepts
-- are retained. Exact values, spell rules, NPC offerings and geography are
-- deliberately absent because their 7.72 applicability is unverified.
K.RECORDS = {
  { id='survival.hp', topics={'safety','health','combat'}, source='manual_characters',
    text='HP measures survival. Low HP increases the cost of continuing combat; retreat or recovery can be wiser than another attack.' },
  { id='survival.death', topics={'safety','progression'}, source='manual_characters',
    text='Death can cost progress and resources. Judge risk against likely reward before extending a hunt.' },
  { id='combat.position', topics={'combat','movement','safety'}, source='manual_combat',
    text='Position and escape space matter. Avoid being surrounded, and reassess when creatures or terrain change.' },
  { id='combat.target', topics={'combat'}, source='manual_combat',
    text='Attack and follow are different choices: attack engages a visible creature; follow tracks a visible creature without necessarily attacking.' },
  { id='combat.resources', topics={'combat','supplies'}, source='manual_combat',
    text='Combat can consume health, mana, ammunition and supplies. Conserve them when expected reward is poor.' },
  { id='combat.mode', topics={'combat','safety'}, source='manual_combat',
    text='Fight, chase and safety modes alter combat behavior. Check current mode before relying on a tactic.' },
  { id='skills.practice', topics={'progression','skills'}, source='manual_characters',
    text='Skills improve through suitable use; better skills and equipment can improve future combat efficiency.' },
  { id='vocation.roles', topics={'progression','skills','equipment'}, source='manual_characters',
    text='Vocations differ in health, mana and combat strengths. Adapt plans to the character actually observed; do not assume an unobserved vocation.' },
  { id='equipment.weapon', topics={'equipment','economy','progression'}, source='manual_characters',
    text='A stronger suitable weapon may improve hunting efficiency. Compare upgrade cost with available resources and current weapon.' },
  { id='equipment.armor', topics={'equipment','safety','economy'}, source='manual_characters',
    text='Armor and other equipment can improve survivability. Do not spend all resources on an upgrade if supplies are still needed.' },
  { id='inventory.containers', topics={'inventory','loot'}, source='manual_trading',
    text='Backpacks and containers organize carried items. Inspect open contents; unopened contents are unknown.' },
  { id='inventory.capacity', topics={'inventory','loot','economy'}, source='manual_characters',
    text='Carrying capacity limits what can be taken. Free space and free capacity matter before collecting loot or supplies.' },
  { id='inventory.food', topics={'inventory','supplies','health'}, source='manual_world',
    text='Food is a supply. Consider keeping useful food for recovery instead of treating every looted item as immediate consumption.' },
  { id='loot.assess', topics={'loot','economy'}, source='manual_world',
    text='Loot can support progression. Some items are useful to keep, some to consume, and some may be worth selling; exact value is uncertain until learned.' },
  { id='economy.profit', topics={'economy','combat','supplies'}, source='manual_trading',
    text='A hunt is worthwhile when expected progression and loot justify danger, time and supply cost. Reconsider an unprofitable hunt.' },
  { id='economy.gold', topics={'economy','inventory'}, source='manual_trading',
    text='Gold is money used for supplies and upgrades. Count only gold and items actually visible in owned inventory; unseen wealth is unknown.' },
  { id='economy.prepare', topics={'economy','supplies','safety'}, source='manual_world',
    text='Before travelling farther from safety, assess health, capacity, food and other supplies. Returning to town can preserve gains.' },
  { id='economy.trade', topics={'economy','npc'}, source='manual_trading',
    text='NPCs may buy and sell goods. An exact price, stock and trading option must be confirmed in current play; planning a purchase does not execute it.' },
  { id='economy.upgrade', topics={'economy','equipment','progression'}, source='manual_trading',
    text='Earning money for a better weapon or armor can improve future hunts. Balance that investment against immediate supplies.' },
  { id='world.explore', topics={'movement','explore','memory'}, source='manual_world',
    text='Explore cautiously using currently visible walkable tiles. A remembered place is a lead, never proof that a creature or item is there now.' },
  { id='world.npc', topics={'npc','social'}, source='manual_world',
    text='Inhabitants may offer information or trade. Conversation can reveal options, but availability is uncertain until observed.' },
  { id='social.chat', topics={'social','chat'}, source='manual_communication',
    text='Chat with nearby players and inhabitants can share information. Be courteous and avoid claiming knowledge of unseen events.' },
  { id='social.party', topics={'social','combat'}, source='manual_world',
    text='Parties and guilds can coordinate play. Cooperation is a possibility, not a substitute for assessing your own safety.' },
  { id='social.pvp', topics={'social','safety','combat'}, source='manual_combat',
    text='PvP can carry serious risk and world-specific rules. Avoid initiating it on assumptions; observe the situation and applicable rules first.' },
}

function K.get(id)
  for _, record in ipairs(K.RECORDS) do if record.id == id then return record end end
end

function K.retrieve(topics, limit)
  local wanted, ranked = {}, {}
  for _, topic in ipairs(topics or {}) do wanted[topic] = true end
  for _, record in ipairs(K.RECORDS) do
    local score = 0
    for _, topic in ipairs(record.topics) do if wanted[topic] then score = score + 1 end end
    if score > 0 then ranked[#ranked + 1] = { score = score, record = record } end
  end
  table.sort(ranked, function(a, b)
    if a.score ~= b.score then return a.score > b.score end
    return a.record.id < b.record.id
  end)
  local result = {}
  for i = 1, math.min(limit or 7, #ranked) do result[#result + 1] = ranked[i].record end
  return result
end

return K
