// Offline read-only replay of observed pre-encryption server payloads.
// Production decoders and WorldState are linked without modification.
#include "fusion32/protocol772/movement.h"
#include "Real33DHandshakeFrame.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <iterator>
namespace p = fusion32::protocol772;
std::string quote(const std::string& s) {
 std::ostringstream o; o<<'"'; for(unsigned char c:s) {
  if(c=='"'||c=='\\')o<<'\\'<<c;
  else if(c<32)o<<"\\u"<<std::hex<<std::setw(4)<<std::setfill('0')<<int(c)<<std::dec;
  else o<<c;
 } o<<'"'; return o.str();
}
void pos(const p::MapPosition& v){std::cout<<'['<<v.x<<','<<v.y<<','<<v.z<<']';}
void item(const p::ItemThing& v){std::cout<<"{\"type_id\":"<<v.type_id<<",\"amount\":"<<(v.has_amount?int(v.amount):1)<<'}';}
void tile(const p::WorldState& s,const p::MapPosition& v){
 std::cout<<"{\"position\":";pos(v);std::cout<<",\"stack\":[";
 const auto* t=s.FindTile(v); bool first=true;
 if(t)for(const auto& thing:t->things){if(!first)std::cout<<',';first=false;
  if(thing.kind==p::MapThingKind::Item)item(thing.item);
  else std::cout<<"{\"creature_id\":"<<thing.creature.creature_id<<'}';
 }std::cout<<"]}";
}
void state(const p::WorldState& s){
 std::cout<<",\"stats\":{\"known\":"<<(s.stats.known?"true":"false")
 <<",\"hp\":"<<s.stats.hitpoints<<",\"max_hp\":"<<s.stats.max_hitpoints
 <<",\"mana\":"<<s.stats.mana<<",\"max_mana\":"<<s.stats.max_mana
 <<",\"level\":"<<s.stats.level<<",\"experience\":"<<s.stats.experience
 <<"},\"tile_count\":"<<s.tile_count()<<",\"thing_count\":"<<s.thing_count();
 std::cout<<",\"player_id\":"<<s.local_creature_id<<",\"anchor\":";pos(s.viewport_anchor);
 std::cout<<",\"visible_creatures\":[";bool first=true;
 for(auto id:s.visible_creature_ids()){if(!first)std::cout<<',';first=false;
  const auto& c=s.known_creatures.at(id);
  std::cout<<"{\"id\":"<<id<<",\"name\":"<<quote(c.name)<<",\"health\":"<<int(c.health_percent)<<",\"position\":";pos(c.position);std::cout<<'}';
 }std::cout<<"],\"inventory\":[";first=true;
 for(size_t i=1;i<s.inventory.size();++i)if(s.inventory[i].occupied){
  if(!first)std::cout<<',';first=false;std::cout<<"{\"slot\":"<<i<<",\"item\":";item(s.inventory[i].item);std::cout<<'}';
 }std::cout<<"],\"containers\":[";first=true;
 for(size_t i=0;i<s.containers.size();++i)if(s.containers[i].open){
  if(!first)std::cout<<',';first=false;const auto& c=s.containers[i];
  std::cout<<"{\"number\":"<<i<<",\"type_id\":"<<c.type_id<<",\"name\":"<<quote(c.name)<<",\"items\":[";
  bool f=true;for(const auto& v:c.objects){if(!f)std::cout<<',';f=false;item(v);}std::cout<<"]}";
 }std::cout<<']';
}
int main(int argc,char**argv){
 if(argc!=3)return 2;
 std::ifstream typesfile(argv[1]);std::string typetext((std::istreambuf_iterator<char>(typesfile)),{});
 auto types=p::LoadObjectTypeTableFromObjectsSrv(typetext);if(!types.ok()||types.table.empty())return 3;
 std::ifstream packets(argv[2]);std::map<std::string,p::WorldState> worlds;
 std::string line;size_t commands=0,errors=0,anomalies=0;
 while(std::getline(packets,line)){
  std::istringstream row(line);std::string utc,dir,conn,hex;
  std::getline(row,utc,'\t');std::getline(row,dir,'\t');std::getline(row,conn,'\t');std::getline(row,hex);
  if(dir!="OUT")continue;
  std::vector<uint8_t>b;for(size_t i=0;i+1<hex.size();i+=2)b.push_back(uint8_t(std::stoul(hex.substr(i,2),nullptr,16)));
  auto& s=worlds[conn];size_t at=0;
  while(at<b.size()){
   auto h=Real33D::HandshakeCommandAt(b[at],b.size()-at);
   if(h.Length){
    if(h.Command==Real33D::HandshakeCommand::InitGame){s=p::WorldState{};s.local_creature_id=Real33D::InitGameCreatureId(b.data()+at);}
    std::cout<<"{\"utc\":"<<quote(utc)<<",\"connection\":"<<quote(conn)<<",\"opcode\":"<<int(b[at])<<",\"raw_hex\":"<<quote(hex.substr(at*2,h.Length*2))<<",\"anomalies\":0";
    state(s);std::cout<<"}\n";++commands;at+=h.Length;continue;
   }
   const auto d=p::DecodeServerUpdate(b,at,s.viewport_anchor,types.table);
   if(!d.ok()||d.update.bytes_consumed==0){
    ++errors;std::cout<<"{\"utc\":"<<quote(utc)<<",\"error\":"<<quote(d.detail)<<",\"opcode\":"<<int(b[at])<<",\"offset\":"<<at<<"}\n";break;
   }
   const auto& u=d.update;std::cout<<"{\"utc\":"<<quote(utc)<<",\"connection\":"<<quote(conn)<<",\"opcode\":"<<int(u.opcode)<<",\"name\":"<<quote(u.name)<<",\"raw_hex\":"<<quote(hex.substr(at*2,u.bytes_consumed*2));
   bool affected=false;p::MapPosition v;
   if(u.kind==p::ServerUpdateKind::AddField){affected=true;v=u.add_field.position;}
   if(u.kind==p::ServerUpdateKind::ChangeField){affected=true;v=u.change_field.position;std::cout<<",\"stack_index\":"<<int(u.change_field.stack_index);}
   if(u.kind==p::ServerUpdateKind::DeleteField){affected=true;v=u.delete_field.position;std::cout<<",\"stack_index\":"<<int(u.delete_field.stack_index);}
   if(u.kind==p::ServerUpdateKind::MoveCreature){affected=true;v=u.move_creature.origin;std::cout<<",\"destination\":";pos(u.move_creature.destination);}
   if(affected){std::cout<<",\"before\":";tile(s,v);}
   if(u.opcode==140)std::cout<<",\"creature_id\":"<<u.creature_attribute.creature_id<<",\"health\":"<<int(u.creature_attribute.health_percent);
   if(u.kind==p::ServerUpdateKind::Message)std::cout<<",\"message\":"<<quote(u.message.text)<<",\"message_mode\":"<<int(u.message.mode);
   auto applied=p::ApplyServerUpdate(&s,u,types.table);anomalies+=applied.anomalies.size();
   std::cout<<",\"anomalies\":"<<applied.anomalies.size();
   if(affected){std::cout<<",\"after\":";tile(s,v);}
   state(s);std::cout<<"}\n";
   at+=u.bytes_consumed;++commands;
  }
 }
 std::cerr<<"commands="<<commands<<" errors="<<errors<<" anomalies="<<anomalies<<'\n';
 return errors||anomalies?1:0;
}
