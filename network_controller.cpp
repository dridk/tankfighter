#include <SFML/Network/Packet.hpp>
#include "network_controller.h"
#include <stdlib.h>
#include <vector>
#include <string>
#include "entity.h"
#include "player.h"
#include "controller.h"
#include "wall.h"
#include "missile.h"
#include "engine.h"
#include <string.h>
#include <stdio.h>

#undef LOG_LLPACKET

#ifdef LOG_LLPACKET
#define LOG_LLPACKETR 1
#define LOG_LLPACKETS 1
#endif

using namespace sf;

static void getPlayerPosition(Player *player, PlayerPosition &pos);
static void getPlayerPosition(Player *player, ApproxPlayerPosition &pos);

const unsigned NetworkClient::C2S_Packet_interval = 10000; /* 10 milliseconds, 100 fps */
static const char *NMF_PlayerPosition = "uusscc";
static const char *NMF_MissilePosition = "uussspp";
static const char *NMF_PlayerMovement = "8" "uffff" "ff";
static const char *NMF_Block = "ssssS";
static const char *NMF_PlayerScore = "uu";

/***************************** MasterController ****************************/
MasterController::MasterController(NetworkClient *client, Controller *phy):phy(phy),client(client) {
}
MasterController::~MasterController() {
	delete phy;
}
Controller *MasterController::getOrigController(void) {
	return phy;
}
Controller *MasterController::stealOrigController(void) {
	Controller *r = phy;
	phy=NULL;
	return r;
}
void MasterController::reportMissileMovement(Missile *missile, MissileControllingData &mcd) {
	/* On master controller client, missile movements are reported by Server through ReportPMPositions messages, but we can still extrapolate movement between two server requests */
	/* On master controller server, missile movements are computed and broadcasted to clients */
	/* If the controller is non-standard, the server uses it, while the client extrapolate with standard rules */
	if (client->isServer()) phy->reportMissileMovement(missile, mcd);
	else Controller::reportMissileMovement(missile, mcd);
}
void MasterController::reportPlayerMovement(Player *player, PlayerControllingData &pcd) {
	/* On master controller, player movements are always completely computed */
	PlayerMovement plpos;
	phy->reportPlayerMovement(player, pcd);
	getPlayerPosition(player, plpos.latestPosition);
	plpos.tmEnd = 0;
	plpos.latestSpeed_x = pcd.movement.x;
	plpos.latestSpeed_y = pcd.movement.y;
	client->reportPlayerMovement(plpos);
}
bool MasterController::missileCreation(Missile *ml) {
	if (!client->isClient()) return phy->missileCreation(ml);
	client->requestMissileCreation(ml->getOwner());
	return false;
}
bool MasterController::missileCollision(Missile *ml, Player *other) {
	if (client->isClient()) return false;
	if (client->isServer()) {
		client->reportPlayerDeath(ml->getOwner(), other);
	}
	return true;
}


/******************* RemoteController ******************/
RemoteController::RemoteController(NetworkClient *client, const RemoteClient &masterClient):client(client),masterClient(masterClient) {
}
RemoteClient RemoteController::getMasterClient() const {return masterClient;}
void RemoteController::reportMissileMovement(Missile *missile, MissileControllingData &mcd) {
	/* On server, we MUST extrapolate missile movements */
	/* On client, we can extrapolate missile movements even although they're frequently updated by the server */
	/*if (client->isServer() || !client->isServer()) */
		Controller::reportMissileMovement(missile, mcd);
}
void RemoteController::reportPlayerMovement(Player *player, PlayerControllingData &pcd) {
	/* On server, we update player position in C2S_ReportPlayersMovement messages, and so, we've nothing to do there: FIXME next: extrapolate movement */
	/* On client, we update player position in C2S_ReportPMPositions messages, and so, we've nothing to do there */
}
RemoteController::~RemoteController() {
}
bool RemoteController::missileCollision(Missile *ml, Player *other) {
	if (client->isClient()) return false;
	if (client->isServer()) {
		client->reportPlayerDeath(ml->getOwner(), other);
	}
	return true;
}

void RemoteController::teleported(void) {
	ApproxPlayerPosition ppos;
	getPlayerPosition(getPlayer(), ppos);
	client->reportPlayerPosition(ppos);
}


/**************************** Helper functions ************************/
RemoteClientInfo::RemoteClientInfo(const RemoteClient &rc):client(rc) {
}
bool operator ==(const RemoteClient &a, const RemoteClient &b) {
	return a.addr == b.addr && a.port == b.port;
}
static void getPlayerPosition(Player *player, PlayerPosition &pos) {
	pos.playerUID   = player->getUID();
	pos.x           = player->position.x;
	pos.y           = player->position.y;
	pos.tank_angle  = player->getTankAngle();
	pos.canon_angle = player->getCanonAngle();
}
static unsigned short fl2us(double fl, double maxval) {
	return static_cast<unsigned short>(fl/maxval*65536.0);
}
static Uint8 fl2b(double fl, double maxval) {
	return static_cast<Uint8>(fl/maxval*256.0);
}
static double us2fl(unsigned short us, double maxval) {
	return us*maxval/65536.0;
}
static double b2fl(Uint8 b, double maxval) {
	return b*maxval/256.0;
}
static void getPlayerPosition(Player *player, ApproxPlayerPosition &pos) {
	Vector2d map_size = player->getEngine()->map_size();
	pos.playerUID   = player->getUID();
	pos.x           = fl2us(player->position.x, map_size.x);
	pos.y           = fl2us(player->position.y, map_size.y);
	pos.score       = player->getScore();
	pos.tank_angle  = fl2b(player->getTankAngle(), 2*M_PI);
	pos.canon_angle = fl2b(player->getCanonAngle(), 2*M_PI);
}
static bool getMissilePosition(Missile *ml, MissilePosition &pos) {
	if (!ml->getOwner()) return false;
	Vector2d map_size = ml->getOwner()->getEngine()->map_size();
	pos.missileUID  = ml->getUID();
	pos.origPlayer  = ml->getOwner()->getUID();
	pos.cur_x       = fl2us(ml->position.x, map_size.x);
	pos.cur_y       = fl2us(ml->position.y, map_size.y);
	pos.curAngle    = fl2us(ml->getAngle(), 2*M_PI);
	return true;
}

size_t format_size(const char *s) {
	size_t sz=0;
	while(*s) {
		char c=*s++;
		if (c=='c' || c=='b') sz++;
		else if (c=='f' || c=='u') sz+=4;
		else if (c=='8') sz+=8;
		else if (c=='s') sz+=2;
		else if (c=='S') sz+=sizeof(char*);
		else if (c=='p') sz++;
	}
	return sz;
}

/************************** Raw UDP packet I/O **********************/
static bool overflows(const Packet &pkt, unsigned extra=0) {
	return pkt.getDataSize()+extra >= 512;
}

bool Output(Packet &pkt, const char *signature, const void * const data) {
	const char *p=signature;
	const char *dat = static_cast<const char*>(data);
	if (overflows(pkt, strlen(signature))) return false;
	while(*p) {
		char c= *p++;
		if (c == 'c') {pkt << *(Uint8*)dat;dat +=sizeof(Uint8);}
		else if (c == 'b') {pkt << *(bool*)dat;dat +=sizeof(bool);}
		else if (c == 's') {pkt << *(Uint16*)dat;dat +=sizeof(Uint16);}
		else if (c == 'u') {
			Uint32 x = *(Uint32*)dat;
			if (x < 127) pkt << (Uint8)x;
			else {pkt << Uint8(128+((x>>24) & 0xFF)); pkt << Uint8((x>>16) & 0xFF); pkt << Uint8((x >> 8) & 0xFF); pkt << Uint8(x & 0xFF);}
			dat += sizeof(Uint32);
		}
		else if (c == 'f') {pkt << *(float*)dat;dat += sizeof(float);}
		else if (c == 'S') {
			const char * out = *(const char*const*)dat;
			if (overflows(pkt, strlen(out)+1)) return false;
			pkt.append(out, strlen(out)+1);
			dat += sizeof(const char*);
		} else if (c == '8') {
			Uint64 i8 = *(Uint64*)dat;
			pkt << (Uint32)(i8 >> 32);
			pkt << (Uint32)(i8 & 0xFFFFFFFF);
			dat += sizeof(Uint64);
		} else if (c == 'p') dat ++;
	}
	return true;
}
bool Input(Packet &pkt, const char *signature, void * const data) {
	const char *p=signature;
	char *dat = static_cast<char*>(data);
	while(*p) {
		char c=*p++;
#define CONDOUT(ch,type) if (c == ch) {if (!(pkt >> *(type*)dat)) return false; dat+=sizeof(type);}
		CONDOUT('c', Uint8)
		else CONDOUT('b',bool)
		else CONDOUT('f',float)
		else CONDOUT('s',short)
		else if (c == 'u') {
			Uint8 x0;
			Uint32 x=0;
			if (!(pkt >> x0)) return false;
			if (x0 < 127) {
				x=x0;
			} else {
				x0 -= 128;
				Uint8 x1, x2, x3;
				if (!((pkt >> x1) && (pkt >> x2) && (pkt >> x3))) return false;
				x = (x0<<24) + (x1 << 16) + (x2 << 8) + x3;
			}
			*(Uint32*)dat = x;
			dat += sizeof(Uint32);
		} else if (c == 'S') {
			Uint8 c=-1;
			std::string str;
			while ((pkt >> c) && c != 0) str.push_back((char)c);
			if (c != 0) return false;
			*(char**)dat = strdup(str.c_str()); /* FIXME: memory leak if Input fail */
			dat += sizeof(char*);
		} else if (c == '8') {
			Uint32 a1, a2;
			if (!((pkt >> a1) && (pkt >> a2))) return false;
			Uint64 b1 = a1, b2 = a2;
			*(Uint64*)dat = (b1 << 32) + b2;
			dat += sizeof(Uint64);
		} else if (c == 'p') dat ++;
#undef CONDOUT
	}
	return true;
}


/************************ Message class *****************/
bool Message::Input(void *data, const char *format) {
	return ::Input(packet, format, data);
}
bool Message::InputVector(void *data, const char *format, size_t count) {
	char *p=static_cast<char*>(data);
	size_t sz = format_size(format);
	if (count >= 256) {
		fprintf(stderr, "Error in received packet: array size is limited to 256 in UDP packet\n");
		return false;
	}
	for(size_t i=0; i < count;i++) {
		if (!Input(p, format)) return false;
		p += sz;
	}
	return true;
}
bool Message::OutputToPacket(sf::Packet &opacket) {
	size_t dsz = packet.getDataSize();
	if (dsz >= 32000) return false;
	struct {
		Uint32 mseqid;
		Uint32 size;
		Uint8 type; /* High bit is must_acknowledge */
	} msg = {mseqid, (Uint32)dsz, (Uint8)(type | (must_acknowledge*128))};
#if LOG_LLPACKETS
	fprintf(stderr, "[sending message seqid %08X sz %04X ack %02X type %02X]\n"
		,(unsigned)mseqid, (unsigned)dsz
		, (unsigned)must_acknowledge, (unsigned)type);
#endif
	if (!::Output(opacket, "uuc", &msg)) return false;
	if (overflows(opacket, dsz)) return false;
	opacket.append(packet.getData(), dsz);
	return true;
}
static int seqid = 1;
Message::Message() {
	already_sent = false;
	mseqid = seqid++;
	must_acknowledge = false;
	type = NMT_None;
	client.addr = IpAddress(0,0,0,0);
	client.port = 0;
}
Message::Message(const void *data, NetworkMessageType type0) {
	already_sent = false;
	mseqid = seqid++;
	must_acknowledge = false;
	type = NMT_None;
	client.addr = IpAddress(0,0,0,0);
	client.port = 0;
	type = type0;
	DefineOutput(data, type0);
}
void Message::DefineOutput(const void *data, NetworkMessageType type0) {
	const MessageStructure &ms = messages_structures[type];
	must_acknowledge = ms.must_acknowledge;
	mseqid = seqid++;
	type = type0;
	Append(data, ms.format);
}

bool Message::Append(const void *data, const char *format) {
	return ::Output(packet, format, data);
}
bool Message::Append(const void *data, const char *item_format, size_t item_count) {
	Uint32 ic = item_count;
	const char *p = static_cast<const char*>(data);
	size_t item_size = format_size(item_format);
	if (!Append(&ic, "u")) return false;
	for(size_t i=0; i < ic; i++) {
		if (!Append(p, item_format)) return false;
		p += item_size;
	}
	return true;
}

/******************************** NetworkClient class ******************************/
bool NetworkClient::transmitToServer() { /* returns false if no packet is transmitted to the server */
	if (pairs.size() == 0 && !is_server) {
		/* You must first request server connection */
		return false;
	}
	if (c2s_time.getElapsedTime().asMicroseconds() < C2S_Packet_interval) return false;
	if (is_server) reportPlayerAndMissilePositions();
	c2s_time.restart();
	if (!is_server) {
		Message *nmsg=new Message(NULL, NMT_C2S_ReportPlayersMovements);
		nmsg->AppendV(plmovements, NMF_PlayerMovement);
		willSendMessage(nmsg);
	}
	plmovements.resize(0);
	return transmitMessageSet(c2sMessages);
}
bool PacketAppend(Packet &pkt, const void *data, size_t size) {
	if (overflows(pkt, size)) return false;
	pkt.append(data, size);
	return true;
}
bool PacketAppend(Packet &pkt, const Packet &pkt2) {
	return PacketAppend(pkt, pkt2.getData(), pkt.getDataSize());
}
static int resendPacketAfterMS = 2000;
bool NetworkClient::transmitMessageSet(std::vector<Message*> &messages) {
	sf::Packet pkt;
	std::vector<Message*> out;
	for (size_t cli=0; cli <= pairs.size(); cli++) {
	RemoteClient cl;
	if (cli < pairs.size()) cl = pairs[cli].client;
	if (cli == pairs.size() && is_server) {
		continue;
	}
	for (size_t i=0; i < messages.size();) {
		Message * msg = messages[i];
		if (!(msg->client == cl)) {i++;continue;}
		if (msg->must_acknowledge && msg->already_sent && msg->lastTransmission.getElapsedTime().asMilliseconds() <  resendPacketAfterMS)
			{i++;continue;}
		if (!msg->OutputToPacket(pkt)) {
			transmitPacket(pkt, &cl);
			pkt=sf::Packet();
			msg->OutputToPacket(pkt);
		}
		msg->already_sent = true;
		msg->lastTransmission.restart();
		if (msg->must_acknowledge) {
			out.push_back(msg);
		} else {
			delete msg;
		}
		messages.erase(messages.begin()+i);
	}
	if (pkt.getDataSize() > 0) {transmitPacket(pkt, &cl);}
	}
	messages.swap(out);
	out.clear();
	return true;
}
bool Message::DefineInput(sf::Packet &ipacket) {
	struct {
		Uint32 mseqid;
		Uint32 size;
		Uint8 type;
	} msg;
	if (!::Input(ipacket, "uuc", &msg)) return false;
	mseqid = msg.mseqid;
	must_acknowledge = !!(msg.type & 128);
	type = (NetworkMessageType)(msg.type & 127);
	if (msg.size >= 32000) return false;
	std::vector<Uint8> data(msg.size);
	for(size_t i=0;i < msg.size; i++) {
		if (!(ipacket >> data[i])) return false;
	}
	packet = sf::Packet();
	packet.append(&data[0], data.size());
#ifdef LOG_LLPACKETR
	fprintf(stderr, "[receiving message seqid %08X sz %04X ack %02X type %02X]\n"
		,(unsigned)mseqid, (unsigned)msg.size
		, (unsigned)must_acknowledge, (unsigned)type);
#endif
	return true;
}
void NetworkClient::willSendMessage(Message *msg) {
	c2sMessages.push_back(msg);
}
void NetworkClient::Acknowledge(const Message &msg) { /* FIXME: Eliminate duplicate packets */
	Uint32 seqid = msg.mseqid;
	Message *newm = new Message(&seqid, NMT_Acknowledge);
	newm->client = msg.client;
	willSendMessage(newm);
}
void setPlayerPosition(Player *player, PlayerPosition &ppos) {
	PlayerControllingData pcd;
	pcd.setPosition(Vector2d(ppos.x, ppos.y));
	pcd.setCanonAngle(ppos.canon_angle);
	pcd.setTankAngle(ppos.tank_angle);
	player->applyPCD(pcd);
}
void setPlayerPosition(Player *player, ApproxPlayerPosition &ppos) {
	PlayerControllingData pcd;
	Engine *engine = player->getEngine();
	Vector2d map_size = engine->map_size();
	double x = us2fl(ppos.x, map_size.x);
	double y = us2fl(ppos.y, map_size.y);
	pcd.setPosition(Vector2d(x,y));
	pcd.setCanonAngle(b2fl(ppos.canon_angle,2*M_PI));
	pcd.setTankAngle (b2fl(ppos.tank_angle, 2*M_PI));
	pcd.setScore(ppos.score);
	player->applyPCD(pcd);
}
void NetworkClient::setPlayerPosition(PlayerPosition &ppos) {
	Player *pl = getEngine()->getPlayerByUID(ppos.playerUID);
	if (!pl) return; /* player not yet created (packet will be probably received soon) */
	::setPlayerPosition(pl, ppos);
}
void NetworkClient::setPlayerPosition(ApproxPlayerPosition &ppos) {
	Player *pl = getEngine()->getPlayerByUID(ppos.playerUID);
	if (!pl) return;
	::setPlayerPosition(pl, ppos);
}

void NetworkClient::setMissilePosition(MissilePosition &mpos) {
	Missile *ml = getEngine()->getMissileByUID(mpos.missileUID);
	if (!ml) { /* create missile if it doesn't exist yet */
		Player *pl = getEngine()->getPlayerByUID(mpos.origPlayer);
		/* Creation of missiles can apply to RemoteController as well as MasterControllers */
		if (!pl) return;
		/* if (!getEngine()->canCreateMissile(pl)) return; */
		ml = new Missile(pl);
		ml->setUID(mpos.missileUID);
		getEngine()->add(ml);
	}
	Vector2d map_size = getEngine()->map_size();
	ml->setPosition(Vector2d(us2fl(mpos.cur_x, map_size.x), us2fl(mpos.cur_y, map_size.y)));
	ml->setAngle(us2fl(mpos.curAngle, 2*M_PI));
}
void load_map_from_blocks(Engine *engine, unsigned width, unsigned height, std::vector<Block> &blocks) {
	engine->defineMapBoundaries(width, height);
	for(size_t i=0; i < blocks.size(); i++) {
		Block &b=blocks[i];
		fprintf(stderr, "Wall: %i %i %i %i %s\n"
			,b.x,b.y,b.width,b.height
			,b.texture_name);
		engine->add(new Wall(b.x, b.y, b.width, b.height, b.texture_name, engine));
	}
}
void CollectMapBlocks(Engine *engine, std::vector<Block> &blocks) {
	Wall *iwall = dynamic_cast<Wall*>(engine->getMapBoundariesEntity());
	for(Engine::EntitiesIterator it=engine->begin_entities(), e=engine->end_entities(); it != e; ++it) {
		Wall *wall = dynamic_cast<Wall*>(*it);
		if (!wall) continue;
		if (wall == iwall) continue;
		Block b;
		b.x = wall->position.x;
		b.y = wall->position.y;
		Vector2d sz = wall->getSize();
		b.width = sz.x;
		b.height = sz.y;
		b.texture_name = strdup(wall->getTextureName().c_str());
		blocks.push_back(b);
	}
}
bool hasRemoteController(Player *pl) {
	return dynamic_cast<RemoteController*>(pl->getController());
}
template <class EType>
void killAllEntities(Engine *engine, bool killFlag, bool (*pred)(EType *entity) = NULL) {
	for(Engine::EntitiesIterator it=engine->begin_entities(), e=engine->end_entities(); it != e; ++it) {
		EType *en = dynamic_cast<EType*>(*it);
		if (en && ((!pred) || pred(en))) en->setKilled(killFlag);
	}
}
static Color u2color(Uint32 c) {
	Color cc;
	cc.r = c & 0xFF;
	cc.g = (c >> 8 ) & 0xFF;
	cc.b = (c >> 16) & 0xFF;
	cc.a = (c >> 24) & 0xFF;
	return cc;
}
static Uint32 color2u(Color c) {
	return c.r + (c.g << 8) + (c.b << 16) + (c.a << 24);
}
void NetworkClient::reportNewPlayer(Player *pl, Uint32 toseqid, const RemoteClient &creator, const RemoteClient &target) {
	if (!is_server) return;
	RemoteClient nulcreator;
	NewPlayerM mh;
	getPlayerPosition(pl, mh.pos);
	mh.color = color2u(pl->getColor());
	mh.seqid = toseqid;
	/* send back one NewPlayer message for each pair */
	for(size_t i=0; i < pairs.size(); i++) {
		if ((!(target == nulcreator)) && !(target == pairs[i].client)) continue;
		mh.is_yours =  (pairs[i].client == creator && !(creator == nulcreator));
		Message *nmsg = new Message(&mh, NMT_S2C_NewPlayer);
		nmsg->client = pairs[i].client;
		willSendMessage(nmsg);
	}
}
void NetworkClient::reportNewPlayer(Player *player, const RemoteClient *target) {
	RemoteClient none;
	fprintf(stderr, "[reportNewPlayer master on server]\n");
	reportNewPlayer(player, 0, none, (target?*target:none));
}
void NetworkClient::setPlayerScore(const PlayerScore &score) {
	Player *pl = getEngine()->getPlayerByUID(score.playerUID);
	if (pl) pl->setScore(score.score);
}
bool NetworkClient::treatMessage(Message &msg) {
#ifdef LOG_PACKET
	fprintf(stderr, "[treatMessage %s]\n", messages_structures[msg.type].name);
#endif
	if (is_server    && !messages_structures[msg.type].C2S_allowed) return false;
	if ((!is_server) && !messages_structures[msg.type].S2C_allowed) return false;
	if (msg.type == NMT_Acknowledge) {
		Uint32 seqid;
		if (!msg.Input(&seqid, messages_structures[msg.type].format)) return false;
		for(size_t i=0; i < c2sMessages.size(); ++i) {
			if (c2sMessages[i]->mseqid == seqid) {
				Message *dmsg = c2sMessages[i];
				c2sMessages.erase(c2sMessages.begin()+i);
				delete dmsg;
				break;
			}
		}
		return true;
	} else if (msg.type == NMT_C2S_RequestConnection) {
#ifdef LOG_PACKET
		fprintf(stderr, "[request connection from client port %d]\n", msg.client.port);
#endif
		pairs.push_back(RemoteClientInfo(msg.client)); /* accept new client */
		Vector2d sz = getEngine()->map_size();
		DefineMapM mh = {(unsigned short)sz.x, (unsigned short)sz.y};
		Message *nmsg = new Message(&mh, NMT_S2C_DefineMap);
		nmsg->client = msg.client; /* send back DefineMap to client */
		std::vector<Block> blocks;
		CollectMapBlocks(getEngine(), blocks);
		nmsg->AppendV(blocks, NMF_Block);
		for(size_t i = 0; i < blocks.size(); ++i) {
			free(blocks[i].texture_name);
		}
		willSendMessage(nmsg);
		Engine *engine = getEngine();
		/* report all existing players in game to the new client */
		for(Engine::EntitiesIterator it = engine->begin_entities(), e=engine->end_entities(); it != e; ++it) {
			Player *pl = dynamic_cast<Player*>(*it);
			if (pl) reportNewPlayer(pl, &msg.client);
		}
		return true;
	} else if (msg.type == NMT_C2S_RequestNewPlayer) {
		RequestNewPlayerM rnp;
		if (!msg.Input(&rnp, messages_structures[msg.type].format)) return false;
		Player *pl = new Player(new RemoteController(this, msg.client), getEngine());
		if (rnp.preferred_color) {
			pl->setColor(u2color(rnp.color));
		}
		getEngine()->add(pl);
		RemoteClient none;
		reportNewPlayer(pl, msg.mseqid, msg.client, none);
		return true;
	} else if (msg.type == NMT_C2S_PlayerDisconnects) {
		PlayerDisconnectsM pdis;
		if (!msg.Input(&pdis, messages_structures[msg.type].format)) return false;
		Player *pl = getEngine()->getPlayerByUID(pdis.playerUID);
		getEngine()->destroy(pl);
	} else if (msg.type == NMT_C2S_RequestNewMissile) {
		RequestNewMissileM rnm;
		if (!msg.Input(&rnm, messages_structures[msg.type].format)) return false;
		Player *pl = getEngine()->getPlayerByUID(rnm.origPlayer);
		if (!pl) return false;
		if (!getEngine()->canCreateMissile(pl)) return true;
		Missile *ml = new Missile(pl);
		ml->setPosition(Vector2d(rnm.x, rnm.y));
		ml->setAngle(rnm.origAngle);
		getEngine()->add(ml);
		/* we don't need to send NewMissile messages as the ReportPMPositions message will take care of this information */
		return true;
	} else if (msg.type == NMT_C2S_ReportPlayersMovements) { /* each client side this type of message to the server */
		std::vector<PlayerMovement> pmov;
		if (!msg.InputV(pmov, NMF_PlayerMovement)) return false;
		for(size_t i=0; i < pmov.size(); ++i) {
			/* FIXME: Use additionnal speed info to extrapolate movement */
#if 0
			Player *pl = getEngine()->getPlayerByUID(pmov[i].latestPosition.playerUID);
			const PlayerPosition &pos = pmov[i].latestPosition;
			if (pl && dynamic_cast<MasterController*>(pl->getController())) {
				fprintf(stderr, "[Master Controller player got teleported to %g %g]\n"
					,pos.x, pos.y);
			}
#endif
			setPlayerPosition(pmov[i].latestPosition);
		}
		return true;
	} else if (msg.type == NMT_S2C_ReportPMPositions) {
		Int32 i = msg.mseqid - last_pm_seqid;
		if (i <= 0) return true; /* this late message is obsolete */
		std::vector<ApproxPlayerPosition> ppos;
		std::vector<MissilePosition> mpos;
		std::vector<PlayerScore> pscore;
		if (!msg.InputV(ppos, NMF_PlayerPosition)) return false;
		if (!msg.InputV(mpos, NMF_MissilePosition)) return false;
		if (!msg.InputV(pscore, NMF_PlayerScore)) return false;
		last_pm_seqid = msg.mseqid;
		killAllEntities<Player>(getEngine(), true, hasRemoteController);
		killAllEntities<Missile>(getEngine(), true);
		for(size_t i = 0; i < ppos.size(); i++) {
			Player *pl=getEngine()->getPlayerByUID(ppos[i].playerUID);
			if (pl) pl->setKilled(false);
			setPlayerPosition(ppos[i]);
		}
		for(size_t i = 0; i < mpos.size(); i++) {
			Missile *ml=getEngine()->getMissileByUID(mpos[i].missileUID);
			if (ml) ml->setKilled(false);
			setMissilePosition(mpos[i]); /* Note: create missile if it doesn't exist */
		}
		for(size_t i=0; i < pscore.size(); i++) {
			setPlayerScore(pscore[i]);
		}
		return true;
	} else if (msg.type == NMT_S2C_NewPlayer) {
		fprintf(stderr, "[NMT_S2C_NewPlayer]\n");
		NewPlayerM newp;
		Player *pl = NULL;
		if (!msg.Input(&newp, messages_structures[msg.type].format)) return false;
		fprintf(stderr, "[NMT_S2C_NewPlayer OK]\n");
		if (newp.is_yours) {
			/* look at the pending player creation request */
			for(size_t i=0; i < wpc.size(); i++) {
				if (wpc[i].seqid == newp.seqid) {
					pl = new Player(new MasterController(this, wpc[i].controller), getEngine());
					wpc.erase(wpc.begin()+i);
					fprintf(stderr, "[own player accepted]\n");
					break;
				}
			}
			fprintf(stderr, "[own player???]\n");
		} else {
			/* another player joined */
			fprintf(stderr, "[player %d joined]\n", newp.pos.playerUID);
			pl = new Player(new RemoteController(this, msg.client), getEngine());
		}
		if (!pl) return false;
		pl->setUID(newp.pos.playerUID);
		::setPlayerPosition(pl, newp.pos);
		pl->setColor(u2color(newp.color));
		getEngine()->add(pl);
		return true;
	} else if (msg.type == NMT_S2C_DefineMap) {
		DefineMapM mapdef;
		std::vector<Block> blocks;
		if (!msg.Input(&mapdef, messages_structures[msg.type].format)) return false;
		if (!msg.InputV(blocks, NMF_Block)) return false;
		load_map_from_blocks(getEngine(), mapdef.width, mapdef.height, blocks);
		for(size_t i=0; i < blocks.size(); i++) free(blocks[i].texture_name);
		return true;
	} else if (msg.type == NMT_S2C_PlayerDeath) {
		PlayerDeathM pd;
		if (!msg.Input(&pd, messages_structures[msg.type].format)) return false;
		Player *killer = getEngine()->getPlayerByUID(pd.killerPlayer);
		Player *killed = getEngine()->getPlayerByUID(pd.killedPlayer);
		killed->killedBy(killer);
		killer->killedPlayer(killed);
		return true;
	} else if (msg.type == NMT_C2S_RequestDisconnection) {
		fprintf(stderr, "Client requested disconnection\n");
		disconnectClient(msg.client);
		return true;
	}
	return false;
}
bool NetworkClient::receiveFromServer() {
	sf::Packet pkt;
	RemoteClient client;
	if (!receivePacket(pkt, &client)) return false;
	Message msg;
	while (msg.DefineInput(pkt)) {
		msg.client = client;
		if (msg.must_acknowledge) Acknowledge(msg);
		if (msg.type >= NMT_Last) continue; /* unknown packet */
		treatMessage(msg);
	}
	return true;
}
bool NetworkClient::transmitPacket(sf::Packet &pkt, const RemoteClient *client) {
	cleanupClients();
	if (NULL==client || (client->port == 0 && client->addr.toInteger() == 0)) {
		if (pairs.size() == 0) {
			fprintf(stderr, "Error: No pair to send packet\n");
			return true;
		}
		client = &pairs[0].client;
	}
	return remote.Send(pkt, client);
}
bool NetworkClient::receivePacket(sf::Packet &pkt, RemoteClient *client) {
	RemoteClient client0;
	bool success = remote.Receive(pkt, &client0);
	if (client) *client = client0;
	if (success) {
		for(size_t i=0; i < pairs.size(); i++) {
			if (pairs[i].client == client0) {
				pairs[i].lastPacketTime.restart();
				break;
			}
		}
	}
	return success;
}
static unsigned connectionTimeoutInSecs = 10;
void NetworkClient::cleanupClients(void) {
	if (!isServer()) return;
	if (cleanup_clock.getElapsedTime().asMilliseconds() < 1000) return;
	cleanup_clock.restart();
	/* find clients having lost connection */
	for(size_t i=0; i < pairs.size();) {
		if (pairs[i].lastPacketTime.getElapsedTime().asMicroseconds()/1000000 >= connectionTimeoutInSecs) {
			RemoteClient client = pairs[i].client;
			fprintf(stderr, "Error: Client %s:%d timeout\n"
				,client.addr.toString().c_str(), client.port);
			disconnectClient(client);
			/*pairs.erase(pairs.begin()+i);*/
		} else i++;
	}
}
void NetworkClient::disconnectClient(const RemoteClient &client) {
	for(size_t i=0; i < pairs.size(); i++) {
		if (client == pairs[i].client) {
			pairs.erase(pairs.begin()+i);
			break;
		}
	}
	for(Engine::EntitiesIterator it=engine->begin_entities(), e=engine->end_entities(); it != e; ++it) {
		Player *pl=dynamic_cast<Player*>(*it);
		if (!pl) continue;
		RemoteController *rctrl = dynamic_cast<RemoteController*>(pl->getController());
		if (rctrl && rctrl->getMasterClient() == client) {
			getEngine()->destroy(pl);
		}
	}
}
UdpConnection::UdpConnection(unsigned short localPort) {
	rebind(localPort);
	sock.setBlocking(false);
}
void UdpConnection::rebind(unsigned short localPort) {
	int i=100;
	while (i > 0 && !sock.bind(localPort)) {localPort++;i--;}
}
void logPacket(sf::Packet &packet) {
	const unsigned char *p=(const unsigned char*)packet.getData(),*e = p+packet.getDataSize();
	for(;p < e; p++) {
		fprintf(stderr, "%02X ", *p);
	}
	fprintf(stderr, "\n");
}
bool UdpConnection::Send(sf::Packet &packet, const RemoteClient *client) {
#ifdef LOG_LLPACKETS
	fprintf(stderr, "Sending packet: ");
	logPacket(packet);
#endif
	return sock.send(packet, client->addr, client->port) == sf::Socket::Done;
}
bool UdpConnection::Receive(sf::Packet &packet, RemoteClient *client) {
	IpAddress raddr;
	unsigned short rport;
	while (sock.receive(packet, raddr, rport) == sf::Socket::Done) {
#ifdef LOG_LLPACKETR
		fprintf(stderr, "Received packet: ");
		logPacket(packet);
#endif
		if (client) {
			client->addr = raddr;
			client->port = rport;
		}
		return true;
	}
	return false;
}

NetworkClient::NetworkClient(Engine *engine):engine(engine),remote(1330) {
	last_pm_seqid = 0;
	is_server = false;
}

void NetworkClient::requestPlayerCreation(Controller *controller, const Color *color) {
	RequestNewPlayerM rnp;
	if (color) {
		rnp.color = color2u(*color);
		rnp.preferred_color = true;
	} else {
		rnp.color = 0;
		rnp.preferred_color = false;
	}
	Message *nmsg = new Message(&rnp, NMT_C2S_RequestNewPlayer);
	PlayerCreation pc={nmsg->mseqid,controller};
	wpc.push_back(pc);
	willSendMessage(nmsg);
}

void NetworkClient::requestMissileCreation(Player *pl) {
	RequestNewMissileM newm={0};
	newm.origPlayer = pl->getUID();
	newm.x = pl->position.x;
	newm.y = pl->position.y;
	newm.origAngle = pl->getCanonAngle();
	Message *nmsg = new Message(&newm, NMT_C2S_RequestNewMissile);
	willSendMessage(nmsg);
}
void NetworkClient::reportPlayerAndMissilePositions(void) {
	/* for each RemoteControlled player, send PM to every client except the originating client (master)
	 * for each other player (including MasterControlled player, although this should not happen), send PM to every client
	 * Missile positions are sent to everybody, including the master player
	 */
	for(size_t icl=0; icl < pairs.size(); ++icl) {
		RemoteClient cl = pairs[icl].client;
		ApproxPlayerPosition ppos0;
		MissilePosition mpos0;
		/*std::vector<PlayerPosition> ppos;*/
		std::vector<MissilePosition> mpos;
		std::vector<PlayerScore> pscore;
		for(Engine::EntitiesIterator it=engine->begin_entities(), e=engine->end_entities(); it != e; ++it) {
			Player *pl=dynamic_cast<Player*>(*it);
			if (pl) {
				RemoteController *rctrl = dynamic_cast<RemoteController*>(pl->getController());
				if (rctrl && cl == rctrl->getMasterClient()) {
					PlayerScore ps;
					ps.playerUID = pl->getUID();
					ps.score     = pl->getScore();
					pscore.push_back(ps);
					continue; /* don't send the position to the master client itself, but give it its score anyway */
				}
				getPlayerPosition(pl, ppos0);
				reportPlayerPosition(ppos0);
				continue;
			}
			Missile *ml=dynamic_cast<Missile*>(*it);
			if (ml) {
				if (getMissilePosition(ml, mpos0)) mpos.push_back(mpos0);
				continue;
			}
		}
		Message *nmsg = new Message(NULL, NMT_S2C_ReportPMPositions);
		nmsg->AppendV(plpositions, NMF_PlayerPosition);
		plpositions.resize(0);
		nmsg->AppendV(mpos, NMF_MissilePosition);
		nmsg->AppendV(pscore, NMF_PlayerScore);
		nmsg->client = cl;
		willSendMessage(nmsg);
	}
}
bool NetworkClient::requestConnection(const RemoteClient &server) {
	remote.rebind(1329);
	pairs.push_back(RemoteClientInfo(server));
	RequestConnectionM rcm;
	Message *nmsg = new Message(&rcm, NMT_C2S_RequestConnection);
	Engine *engine = getEngine();
	willSendMessage(nmsg);
	for(Engine::EntitiesIterator it=engine->begin_entities(), e=engine->end_entities(); it != e; ++it) {
		Player *pl=dynamic_cast<Player*>(*it);
		if (!pl) continue;
		Controller *ctrl = pl->getController();
		if (dynamic_cast<RemoteController*>(ctrl)) continue;
		if (MasterController *mctrl = dynamic_cast<MasterController*>(ctrl)) {
			ctrl = mctrl->stealOrigController();
		} else {
			pl->setController(NULL); /* steals controller */
		}
		Color c = pl->getColor();
		requestPlayerCreation(ctrl, &c);
		getEngine()->destroy(pl);
	}
	engine->clear_entities();
	Entity::useUpperUID();
	return true;
}
static int disconnectionTimeoutMS = 2000;
void NetworkClient::requestDisconnection(void) {
	if (!isClient()) return;
	sf::Clock wait_ack;
	resendPacketAfterMS = 500;
	fprintf(stderr, "Request disconnection\n");
	willSendMessage(new Message(NULL, NMT_C2S_RequestDisconnection));
	bool message_found = true;
	while (message_found && wait_ack.getElapsedTime().asMilliseconds() < disconnectionTimeoutMS) {
		receiveFromServer();
		transmitToServer();
		message_found = false;
		/* search whether disconnection message has been removed (acknowledged) */
		for(size_t i=0; i < c2sMessages.size(); ++i) {
			if (c2sMessages[i]->type == NMT_C2S_RequestDisconnection) {
				message_found = true;
				break;
			}
		}
	}
}
void NetworkClient::declareAsServer(void) {
	is_server = true;
	remote.rebind(1330);
	for(Engine::EntitiesIterator it=engine->begin_entities(), e=engine->end_entities(); it != e; ++it) {
		Player *pl=dynamic_cast<Player*>(*it);
		if (!pl) continue;
		Controller *ctrl = pl->getController();
		if (dynamic_cast<RemoteController*>(ctrl)) continue;
		if (MasterController *mctrl = dynamic_cast<MasterController*>(ctrl)) {
			ctrl = mctrl->stealOrigController();
			pl->setController(NULL);
			delete mctrl;
		} else {
			pl->setController(NULL);
		}
		pl->setController(new MasterController(this, ctrl));
	}
}
void NetworkClient::reportPlayerMovement(const PlayerMovement &plpos) {
	if (isServer()) return;
	Player *pl = getEngine()->getPlayerByUID(plpos.latestPosition.playerUID);
	/* only MasterControlled player can report their movements to the server */
	if (!dynamic_cast<MasterController*>(pl->getController())) return;
	for(size_t i=0; i < plmovements.size(); ++i) {
		if (plmovements[i].latestPosition.playerUID == plpos.latestPosition.playerUID) {
			plmovements[i] = plpos;
			return;
		}
	}
	plmovements.push_back(plpos);
}
void NetworkClient::reportPlayerPosition(const ApproxPlayerPosition ppos) {
	if (isClient()) return;
	for(size_t i=0; i < plpositions.size(); ++i) {
		if (plpositions[i].playerUID == ppos.playerUID) {
			plpositions[i] = ppos;
			return;
		}
	}
	plpositions.push_back(ppos);
}
bool NetworkClient::isServer(void) const {return is_server;}
bool NetworkClient::isClient(void) const {return (!is_server) && pairs.size() > 0;}
bool NetworkClient::isLocal (void) const {return (!is_server) && pairs.size() == 0;}
NetworkClient::~NetworkClient() {
	for(size_t i=0; i < c2sMessages.size(); ++i)
		delete c2sMessages[i];
}

bool NetworkClient::isPendingPlayerConcerned(const Event &e) const {
	for (size_t i=0; i < wpc.size(); ++i) {
		LocalController *c=dynamic_cast<LocalController*>(wpc[i].controller);
		if (c && c->isConcerned(e)) return true;
	}
	return false;
}
void NetworkClient::reportPlayerDeath(Player *killer, Player *killed) {
	PlayerDeathM pd;
	pd.killerPlayer = killer->getUID();
	pd.killedPlayer = killed->getUID();
	for(size_t i=0; i < pairs.size(); i++) {
		Message *newm = new Message(&pd, NMT_S2C_PlayerDeath);
		newm->client = pairs[i].client;
		willSendMessage(newm);
	}
}
