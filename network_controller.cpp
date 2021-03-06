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
#include "misc.h"
#include "parameters.h"
#include <math.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#undef LOG_LLPACKET
#undef LOG_PKTLOSS
#undef DEBUG_PKTLOSS

#ifdef LOG_LLPACKET
#define LOG_LLPACKETR 1
#define LOG_LLPACKETS 1
#endif

using namespace sf;

static void getPlayerPosition(Player *player, PlayerPosition &pos);
static void getPlayerPosition(Player *player, ApproxPlayerPosition &pos);

static const char *NMF_PlayerPosition = "uusscc";
static const char *NMF_MissilePosition = "uussspp";
static const char *NMF_PlayerMovement = "8" "uffff" "ff";
static const char *NMF_BlockHeader = "Sfu" "ffff" "fu";
static const char *NMF_BlockPoint = "ss";
static const char *NMF_PlayerScore = "uu";

/********************** conversion helpers *********************/
static unsigned short fl2us(double fl, double maxval) {
	Uint32 i = static_cast<Uint32>(fl/maxval*65536.0);
	if (i >= 65536) {i = 65535;} /* overflow... set to max position */
	return static_cast<unsigned short>(i);
}
static Uint8 fl2b(double fl, double maxval) {
	return static_cast<Uint8>(fl/maxval*256.0);
}
static double us2fl(unsigned short us, double maxval) {
	Uint32 u = us;
	if (u == 65535) u = 65536; /* assume overflow */
	return us*maxval/65536.0;
}
static double b2fl(Uint8 b, double maxval) {
	return b*maxval/256.0;
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
	client->requestMissileCreation(ml);
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
	PlayerPosition ppos;
	getPlayerPosition(getPlayer(), ppos);
	client->reportPlayerSpawned(ppos);
}


/**************************** Helper functions ************************/
RemoteClientInfo::RemoteClientInfo(const RemoteClient &rc):client(rc) {
}
bool operator ==(const RemoteClient &a, const RemoteClient &b) {
	return a.addr == b.addr && a.port == b.port;
}
std::ostream &operator <<(std::ostream &out, const RemoteClient &a) {
	out << a.addr.toString() << ':' << a.port;
	return out;
}
static void getPlayerPosition(Player *player, PlayerPosition &pos) {
	pos.playerUID   = player->getUID();
	pos.x           = player->position.x;
	pos.y           = player->position.y;
	pos.tank_angle  = player->getTankAngle();
	pos.canon_angle = player->getCanonAngle();
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
	return pkt.getDataSize()+extra >= parameters.UDP_MTU();
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
			/* use a 1-byte, 3-bytes or 5-bytes representation */
			if (x <= 127) pkt << (Uint8)x;
			else if (x < 127*65536L) {
				pkt << Uint8(128+(x/65536)); /* up to 254 */
				pkt << Uint8((x%65536)/256);
				pkt << Uint8(x%256);
			}
			else {
				pkt << Uint8(255);
				pkt << Uint8(((x>>24) & 0xFF));
				pkt << Uint8((x>>16) & 0xFF);
				pkt << Uint8((x >> 8) & 0xFF);
				pkt << Uint8(x & 0xFF);
			}
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
			if (x0 <= 127) {
				x=x0;
			} else if (x0 <= 254) {
				x0 -= 128;
				Uint8 x1, x2;
				if (!((pkt >> x1) && (pkt >> x2))) return false;
				x = (x0<<16) + (x1 << 8) + x2;
			} else { /* x0 == 255 */
				Uint8 x1, x2, x3;
				if (!((pkt >> x0) && (pkt >> x1) && (pkt >> x2) && (pkt >> x3))) return false;
				x = (x0<<24) + (x1 << 16) + (x2 << 8) + x3;
			}
			*(Uint32*)dat = x;
			dat += sizeof(Uint32);
		} else if (c == 'S') {
			Uint8 c=-1;
			std::string str;
			while ((pkt >> c) && c != 0) str.push_back((char)c);
			if (c != 0) return false;
			*(char**)dat = cstrdup(str.c_str()); /* FIXME: memory leak if Input fail */
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
	if (c2s_time.getElapsedTime().asMicroseconds() < parameters.C2S_Packet_interval_US()) return false;
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
static bool must_send_now(Message *msg) {
	return !(msg->must_acknowledge && msg->already_sent && msg->lastTransmission.getElapsedTime().asMilliseconds() <  parameters.resendPacketAfterMS());
}
bool NetworkClient::transmitMessage(Message *msg) {
	Packet pkt;
	msg->already_sent = true;
	msg->lastTransmission.restart();
	if (!msg->OutputToPacket(pkt)) return false;
	return transmitPacket(pkt, &msg->client);
}
static void emitted(Message *msg) {
#ifdef LOG_PKTLOSS
	if (msg->must_acknowledge) {
		if (msg->already_sent) {
			fprintf(stderr, "Info: Resending lost packet %u\n", msg->mseqid);
		} else {
			fprintf(stderr, "Info: Sending important message %u that may be emitted again\n", msg->mseqid);
		}
	} else if (msg->type == NMT_Acknowledge) {
		fprintf(stderr, "Info: Sending acknowledge message to %s:%u\n"
			,msg->client.addr.toString().c_str(), msg->client.port);
	}
#endif
}
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
		if (!must_send_now(msg)) {
			out.push_back(msg);
			messages.erase(messages.begin()+i);
			continue;
		}
		emitted(msg);
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
	for(size_t i=0; i < messages.size();i++) { /* now, deal with messages to non-standard clients */
		Message *msg = messages[i];
		if (!must_send_now(msg)) {out.push_back(msg);continue;}
		emitted(msg);
		transmitMessage(msg);
		if (msg->must_acknowledge) {
			out.push_back(msg);
		}
		else delete msg;
	}
	messages.clear();
	messages.swap(out);
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
void NetworkClient::Acknowledge(const Message &msg) {
	Uint32 seqid = msg.mseqid;
#ifdef LOG_PKTLOSS
	fprintf(stderr, "Info: Acknowleges packet %u to %s:%d\n"
		, seqid, msg.client.addr.toString().c_str(), msg.client.port);
#endif
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
void Polygon2AP(const TFPolygon &poly, ApproxPolygon &apoly, unsigned width, unsigned height) {
	apoly.resize(poly.size());
	for(size_t i=0; i < poly.size(); i++) {
		apoly[i].x = fl2us(poly[i].x, width);
		apoly[i].y = fl2us(poly[i].y, height);
	}
}
void AP2Polygon(const ApproxPolygon &apoly, TFPolygon &poly, unsigned width, unsigned height) {
	poly.resize(apoly.size());
	for(size_t i=0; i < apoly.size(); i++) {
		poly[i].x = us2fl(apoly[i].x, width);
		poly[i].y = us2fl(apoly[i].y, height);
	}
}
static Wall *blockM2wall(const BlockM &b, const Vector2d &size, Engine *engine) {
	TFPolygon poly;
	AP2Polygon(b.apolygon, poly, size.x, size.y);
	TextureDesc tex;
	tex.name = b.h.texture_name;
	tex.angle = b.h.textureAngle;
	const TextureScales &sc = b.h.scales;
	tex.xoff = sc.xoff; tex.yoff = sc.yoff;
	tex.xscale = sc.xscale; tex.yscale = sc.yscale;
	tex.mapping = (Mapping)b.h.mapping;
	tex.color = u2color(b.h.color);
	
	Wall *wall = new Wall(poly, b.h.angle, tex, engine);
	engine->add(wall);
	return wall;
}
void load_map_from_blocks(Engine *engine, unsigned width, unsigned height, std::vector<BlockM> &blocks) {
	engine->clear_entities();
	Vector2d size(width, height);

	for(size_t i=0; i < blocks.size(); i++) {
		Wall *wall = blockM2wall(blocks[i], size, engine);
		if (i == 0) wall->isMapBoundaries(true);
	}
	engine->defineMapSize(width, height);
	engine->freeze(false);
}
static void wall2blockM(Wall *wall, BlockM &b, const Vector2d &size) {
	Polygon2AP(wall->getStraightPolygon(), b.apolygon, size.x, size.y);
	b.h.texture_name = cstrdup(wall->getTextureName().c_str());
	b.h.angle = wall->getAngle();
	b.h.color = color2u(wall->getColor());
	b.h.scales = wall->getTextureScales();
	b.h.textureAngle = wall->getTextureAngle();
	b.h.mapping = wall->getMappingType();
}
void CollectMapBlocks(Engine *engine, std::vector<BlockM> &blocks) {
	BlockM b;
	Wall *iwall = dynamic_cast<Wall*>(engine->getMapBoundariesEntity());
	Vector2d size = engine->map_size();
	wall2blockM(iwall, b, size);
	blocks.push_back(b);
	
	for(Engine::EntitiesIterator it=engine->begin_entities(), e=engine->end_entities(); it != e; ++it) {
		Wall *wall = dynamic_cast<Wall*>(*it);
		if (!wall) continue;
		if (wall == iwall) continue;
		wall2blockM(wall, b, size);
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
static std::string get_server_name(void) {
#ifndef _WIN32
	char hostname[256];
	char domainname[256];
	if (gethostname(hostname, 256)==-1) strcpy(hostname, "(unknown)");
	if (getdomainname(domainname, 256)==-1 || strcmp(domainname, "(none)")==0) strcpy(domainname, "");
	std::string host;
	if (strcmp(domainname, "")==0) host = hostname; else host = std::string(hostname)+"."+domainname;
	if (getenv("USER")) {
		return std::string(getenv("USER")) + "@" + host;
	} else return host;
#else
	return "cool computer";
#endif
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
#ifdef LOG_PKTLOSS
		fprintf(stderr, "Info: Received ACK for packet %u\n", seqid);
#endif
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
		
		std::vector<BlockM> blocks;
		CollectMapBlocks(getEngine(), blocks);
		
		DefineMapM mh = {(unsigned short)sz.x, (unsigned short)sz.y,(unsigned)blocks.size()};
		fprintf(stderr, "Transmitting %d blocks to client\n", mh.block_count);
		Message *nmsg = new Message(&mh, NMT_S2C_DefineMap);
		nmsg->client = msg.client; /* send back DefineMap to client */
		
		for(size_t i = 0; i < blocks.size(); ++i) {
			nmsg->Append(&blocks[i].h, NMF_BlockHeader);
			nmsg->AppendV(blocks[i].apolygon, NMF_BlockPoint);
			free(blocks[i].h.texture_name);
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
		NewPlayerM newp;
		Player *pl = NULL;
		if (!msg.Input(&newp, messages_structures[msg.type].format)) return false;
		if (newp.is_yours) {
			/* look at the pending player creation request */
			for(size_t i=0; i < wpc.size(); i++) {
				if (wpc[i].seqid == newp.seqid) {
					pl = new Player(new MasterController(this, wpc[i].controller), getEngine());
					wpc.erase(wpc.begin()+i);
					break;
				}
			}
		} else {
			/* another player joined */
			pl = new Player(new RemoteController(this, msg.client), getEngine());
		}
		if (!pl) return false;
		pl->setUID(newp.pos.playerUID);
		::setPlayerPosition(pl, newp.pos);
		pl->setColor(u2color(newp.color));
		getEngine()->add(pl);
		return true;
	} else if (msg.type == NMT_S2C_DefineMap) {
		if (pairs.size() >= 1) getEngine()->display(std::string("Successfully connected to server ")+tostring(pairs[0].client));
		DefineMapM mapdef;
		std::vector<BlockM> blocks;
		if (!msg.Input(&mapdef, messages_structures[msg.type].format)) return false;
		blocks.reserve(mapdef.block_count);
		fprintf(stderr, "Receiving %d blocks from server\n", mapdef.block_count);
		for(size_t i=0; i < mapdef.block_count; i++) {
			BlockM bl;
			if (!msg.Input(&bl.h, NMF_BlockHeader)) return false;
			if (!msg.InputV(bl.apolygon, NMF_BlockPoint)) return false;
			blocks.push_back(bl);
		}
		load_map_from_blocks(getEngine(), mapdef.width, mapdef.height, blocks);
		for(size_t i=0; i < blocks.size(); i++) free(blocks[i].h.texture_name);
		return true;
	} else if (msg.type == NMT_S2C_PlayerDeath) {
		PlayerDeathM pd;
		if (!msg.Input(&pd, messages_structures[msg.type].format)) return false;
		Player *killer = getEngine()->getPlayerByUID(pd.killerPlayer);
		Player *killed = getEngine()->getPlayerByUID(pd.killedPlayer);
		killed->killedBy(killer);
		killer->killedPlayer(killed);
		return true;
	} else if (msg.type == NMT_RequestDisconnection) {
		fprintf(stderr, "Pair requested disconnection\n");
		if (isServer()) disconnectClient(msg.client);
		if (isClient()) serverDisconnected();
		return true;
	} else if (msg.type == NMT_S2C_SpawnPlayer) {
		PlayerPosition ppos;
		if (!msg.Input(&ppos, messages_structures[msg.type].format)) return false;
		setPlayerPosition(ppos);
		return true;
	} else if (msg.type == NMT_C2S_GetServerInfo) {
		ServerInfoM si;
		std::string server_name = get_server_name();
		si.name = const_cast<char*>(server_name.c_str());
		Message *nmsg = new Message(&si, NMT_S2C_ServerInfo);
		nmsg->client = msg.client;
		willSendMessage(nmsg);
		return true;
	} else if (msg.type == NMT_S2C_ServerInfo) {
		ServerInfoM sim;
		if (!msg.Input(&sim, messages_structures[msg.type].format)) return false;
		ServerInfo si;
		si.name = std::string(sim.name);
		si.remote = msg.client;
		free(sim.name);
		bool is_known = false;
		for(size_t i=0; i < discoveredServers.size(); ++i) {
			if (discoveredServers[i].remote == si.remote) {
				is_known = true;
				break;
			}
		}
		if (!is_known) discoveredServers.push_back(si);
		return true;
	}
	return false;
}
void NetworkClient::dropOldReceivedMessages(void) {
	for(size_t i=0; i < recMessages.size();) {
		if (recMessages[i].receptionTime.getElapsedTime().asSeconds() >= parameters.maxDupPacketTimeSecs()) {
			recMessages.erase(recMessages.begin()+i);
		} else i++;
	}
}
bool NetworkClient::isDuplicate(const Message &msg) {
	Uint32 seqid = msg.mseqid;
	for(size_t i=0; i < recMessages.size(); i++) {
		if (recMessages[i].seqid == seqid) return true;
	}
	return false;
}
void NetworkClient::declareNewReceivedMessage(const Message &msg) {
#ifdef LOG_PKTLOSS
	fprintf(stderr, "Info: Important message %u that may be received again\n", msg.mseqid);
#endif
	recMessages.push_back(ReceivedMessage(msg.mseqid));
}
bool NetworkClient::receiveFromServer() {
	sf::Packet pkt;
	RemoteClient client;
	dropOldReceivedMessages();
	if (!receivePacket(pkt, &client)) return false;
	Message msg;
	while ((!isLocal()) && msg.DefineInput(pkt)) {
		msg.client = client;
		if (msg.must_acknowledge) Acknowledge(msg); /* even dup packets are acknowledged */
		if (msg.must_acknowledge && isDuplicate(msg)) {
#ifdef LOG_PKTLOSS
			fprintf(stderr, "Info: Duplicate packet %u received\n", msg.mseqid);
#endif
			continue;
		}
		if (msg.type >= NMT_Last) continue; /* unknown packet */
		if (msg.must_acknowledge) declareNewReceivedMessage(msg);
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
void NetworkClient::cleanupClients(void) {
	if (isLocal()) return;
	if (cleanup_clock.getElapsedTime().asMilliseconds() < parameters.clientsCleanupIntervalMS()) return;
	cleanup_clock.restart();
	/* find clients (or server) having lost connection */
	for(size_t i=0; i < pairs.size();) {
		if (pairs[i].lastPacketTime.getElapsedTime().asMicroseconds()/1000000
		           >= parameters.connectionTimeoutInSecs()
			&& pairs[i].client.addr != IpAddress::Broadcast) {
			RemoteClient client = pairs[i].client;
			fprintf(stderr, "Error: Pair %s:%d timeout\n"
				,client.addr.toString().c_str(), client.port);
			if (isServer() || i > 0) {
				disconnectClient(client);
			} else if (isClient() && i == 0) {
				serverDisconnected();
			}
			/*pairs.erase(pairs.begin()+i);*/
		} else i++;
	}
}
void NetworkClient::removePair(const RemoteClient &client) {
	for(size_t i=0; i < pairs.size(); i++) {
		if (client == pairs[i].client) {
			pairs.erase(pairs.begin()+i);
			break;
		}
	}
}
void NetworkClient::removeRemote(const RemoteClient &client) {
	for(Engine::EntitiesIterator it=engine->begin_entities(), e=engine->end_entities(); it != e; ++it) {
		Player *pl=dynamic_cast<Player*>(*it);
		if (!pl) continue;
		RemoteController *rctrl = dynamic_cast<RemoteController*>(pl->getController());
		if (rctrl && rctrl->getMasterClient() == client) {
			getEngine()->destroy(pl);
		}
	}
}
void NetworkClient::disconnectClient(const RemoteClient &client) {
	if (!isServer()) return;
	removeRemote(client);
	removePair(client);
}
void NetworkClient::serverDisconnected(void) {
	if (!isClient()) return;
	RemoteClient cl = pairs[0].client;
	/* MasterControllers are de-capsulated */
	for(Engine::EntitiesIterator it=engine->begin_entities(), e=engine->end_entities(); it != e; ++it) {
		Player *pl=dynamic_cast<Player*>(*it);
		if (!pl) continue;
		MasterController *rctrl = dynamic_cast<MasterController*>(pl->getController());
		if (rctrl) {
			Controller *c = rctrl->stealOrigController();
			pl->setController(c);
			delete rctrl;
		}
	}
	transmitMessageSet(c2sMessages); /* ensures that the ACK message is sent for NMT_RequestDisconnection */
	/* cleanup */
	RemoteClient noname;
	removeRemote(cl);
	removeRemote(noname);
	clear_all();
}
void NetworkClient::clear_all(void) {
	pairs.clear();
	for(size_t i=0; i < c2sMessages.size(); ++i) delete c2sMessages[i];
	c2sMessages.clear();
	wpc.clear();
	recMessages.clear();
	plmovements.clear();
	c2s_time.restart();
	cleanup_clock.restart();
	last_pm_seqid = 0;
	discoveredServers.clear();
	discovery_clock.restart();
	discovery_period_clock.restart();
}
UdpConnection::UdpConnection() {
	sock.setBlocking(false);
}
UdpConnection::UdpConnection(unsigned short localPort) {
	rebind(localPort);
	sock.setBlocking(false);
}
void UdpConnection::rebind(unsigned short localPort) {
	int i=parameters.udpPortRange();
	sock.unbind();
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
#ifdef DEBUG_PKTLOSS
	if (get_random(2)<=1) return false; /* Explicit packet dropping for testing purposes */
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

NetworkClient::NetworkClient(Engine *engine):engine(engine) {
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

void NetworkClient::requestMissileCreation(Missile *ml) {
	Player *pl = ml->getOwner();
	RequestNewMissileM newm={0};
	newm.origPlayer = pl->getUID();
	newm.x = ml->position.x;
	newm.y = ml->position.y;
	newm.origAngle = ml->getAngle();
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
		std::vector<ApproxPlayerPosition> ppos;
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
				ppos.push_back(ppos0);
				continue;
			}
			Missile *ml=dynamic_cast<Missile*>(*it);
			if (ml) {
				if (getMissilePosition(ml, mpos0)) mpos.push_back(mpos0);
				continue;
			}
		}
		Message *nmsg = new Message(NULL, NMT_S2C_ReportPMPositions);
		nmsg->AppendV(ppos, NMF_PlayerPosition);
		nmsg->AppendV(mpos, NMF_MissilePosition);
		nmsg->AppendV(pscore, NMF_PlayerScore);
		nmsg->client = cl;
		willSendMessage(nmsg);
	}
}
bool NetworkClient::requestConnection(const RemoteClient &server) {
	clear_all();
	is_server = false;
	remote.rebind(parameters.clientPort());
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
	engine->freeze(true);
	Entity::useUpperUID();
	getEngine()->display(std::string("Connecting to ")+tostring(server));
	return true;
}
void NetworkClient::requestDisconnection(void) {
	if (isLocal()) return;
	sf::Clock wait_ack;
#ifdef LOG_PKTLOSS
	fprintf(stderr, "Request disconnection at port %d\n", remote.sock.getLocalPort());
#endif
	for(size_t i=0; i < pairs.size(); i++) {
		Message *nmsg = new Message(NULL, NMT_RequestDisconnection);
		nmsg->client = pairs[i].client;
		willSendMessage(nmsg);
	}
	bool message_found = true;
	while (message_found && wait_ack.getElapsedTime().asMilliseconds() < parameters.disconnectionTimeoutMS()) {
		receiveFromServer();
		transmitToServer();
		message_found = false;
		/* search whether all disconnection messages have been acknowledged */
		for(size_t i=0; i < c2sMessages.size(); ++i) {
			if (c2sMessages[i]->type == NMT_RequestDisconnection) {
				message_found = true;
				break;
			}
		}
		sf::sleep(milliseconds(1));
	}
}
void NetworkClient::declareAsServer(void) {
	clear_all();
	is_server = true;
	remote.rebind(parameters.serverPort());
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
	getEngine()->display(std::string("Server created on port ")+tostring(remote.sock.getLocalPort()));
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
bool NetworkClient::isServer(void) const {return is_server;}
bool NetworkClient::isClient(void) const {return (!is_server) && pairs.size() > 0;}
bool NetworkClient::isLocal (void) const {return (!is_server) && pairs.size() == 0;}
NetworkClient::~NetworkClient() {
	clear_all();
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
void NetworkClient::reportPlayerSpawned(const PlayerPosition &ppos) {
	for(size_t i=0; i < pairs.size(); ++i) {
		Message *nmsg = new Message(&ppos, NMT_S2C_SpawnPlayer);
		nmsg->client = pairs[i].client;
		willSendMessage(nmsg);
	}
}
void NetworkClient::discoverServers(bool wait_now) {
	if (!isLocal()) return;
	RemoteClient cl;
	cl.port = parameters.serverPort();
	cl.addr = sf::IpAddress::Broadcast;
	pairs.push_back(RemoteClientInfo(cl)); /* Now, isClient() becomes true */

	Message *nmsg = new Message(NULL, NMT_C2S_GetServerInfo);
	nmsg->client = cl;
	willSendMessage(nmsg);

	if (!wait_now) {
		discovery_clock.restart();
		discovery_period_clock.restart();
	}
	Clock timeout;
	Clock period;
	bool first_time = true;
	do {
		if (first_time || period.getElapsedTime().asMilliseconds() >= parameters.serverDiscoveryPeriodMS()) {
			Message *nmsg = new Message(NULL, NMT_C2S_GetServerInfo);
			nmsg->client = cl;
			willSendMessage(nmsg);
			period.restart();
			first_time = false;
		}
		if (!wait_now) return;
		receiveFromServer();
		transmitToServer();
		sf::sleep(milliseconds(10));
	} while (timeout.getElapsedTime().asMilliseconds() < parameters.serverDiscoveryTimeoutMS());
	pairs.clear();
}
bool NetworkClient::discoveringServers(void) {
	if (!isClient()) return false;
	if (pairs.size() != 1) return false;
	RemoteClient cl = pairs[0].client;
	if (cl.addr == sf::IpAddress::Broadcast) {
		return true;
	}
	return false;
}
bool NetworkClient::discoverMoreServers(void) {
	if (!discoveringServers()) return false;
	bool should_end = discovery_clock.getElapsedTime().asMilliseconds() >= parameters.serverDiscoveryTimeoutMS();
	if (discovery_period_clock.getElapsedTime().asMilliseconds() >= parameters.serverDiscoveryPeriodMS()) {
		Message *nmsg = new Message(NULL, NMT_C2S_GetServerInfo);
		nmsg->client = pairs[0].client;
		willSendMessage(nmsg);
		discovery_period_clock.restart();
	}
	return should_end;
}
void NetworkClient::endServerDiscovery(void) {
	if (!discoveringServers()) return;
	pairs.clear();
	/* now, isLocal() == true */
}
NetworkClient::ServerInfoIterator NetworkClient::begin_servers() {
	return discoveredServers.begin();
}
NetworkClient::ServerInfoIterator NetworkClient::end_servers() {
	return discoveredServers.end();
}
RemoteClient string2remote(const std::string &addr) {
	RemoteClient rc;
	size_t i = addr.find(":");
	rc.port = parameters.serverPort();
	if (i != std::string::npos) {
		long port;
		rc.addr = IpAddress(addr.substr(0, i));
		if (string2long(addr.substr(i+1).c_str(), &port)) {
			rc.port = port;
		}
	} else {
		rc.addr = IpAddress(addr);
	}
	return rc;
}
