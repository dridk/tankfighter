#ifndef __NETWORK_CONTROLLER_H__
#define __NETWORK_CONTROLLER_H__
#include "coretypes.h"
#include <SFML/Network/UdpSocket.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Network/IpAddress.hpp>
#include <SFML/Network/Packet.hpp>
#include <string>
#include "controller.h"
#include <stdio.h>
#include <ostream>
#include <vector>
#include "wall.h"


class Engine;
using sf::IpAddress;
using sf::Uint32;
using sf::Uint16;
using sf::Uint8;
using sf::Vector2f;
using sf::Int64;
using sf::Color;
struct RemoteClient {
	RemoteClient():addr(0,0,0,0),port(0) {}
	IpAddress addr;
	unsigned short port;
};
bool operator ==(const RemoteClient &a, const RemoteClient &b);
RemoteClient string2remote(const std::string &addr);
std::ostream &operator <<(std::ostream &out, const RemoteClient &a);
class UdpConnection {
	public:
	UdpConnection(unsigned short localPort);
	UdpConnection();
	bool Send(sf::Packet &packet, const RemoteClient *client);
	bool Receive(sf::Packet &packet, RemoteClient *client);
	void rebind(unsigned short localPort);
	sf::UdpSocket sock;
};
enum NetworkMessageType {NMT_None, NMT_Acknowledge,
                        NMT_C2S_RequestConnection, NMT_C2S_ReportPlayersMovements, NMT_C2S_RequestNewPlayer, NMT_C2S_PlayerDisconnects, NMT_C2S_RequestNewMissile
                      , NMT_S2C_DefineMap, NMT_S2C_Refusal, NMT_S2C_PlayerDeath, NMT_S2C_NewPlayer, NMT_S2C_ReportPMPositions
		      , NMT_RequestDisconnection
		      , NMT_S2C_SpawnPlayer /* teleport player after death */
		      , NMT_C2S_GetServerInfo
		      , NMT_S2C_ServerInfo
		      , NMT_Last};

/* we don't need NMT_S2C_EntityDestroyed message as it's simply seen by client when ReportPMPositions don't report a player or missile */
/* Similarly, we don't need a NMT_S2C_NewMissile, as it's seen by client when ReportPMPositions report a new missile */

struct MessageStructure {
	NetworkMessageType type;
	const char *name;
	bool must_acknowledge;
	const char *format;
	bool C2S_allowed;
	bool S2C_allowed;
};


const MessageStructure messages_structures[]={
	{NMT_None,"Nop",false,"",true,true},
	{NMT_Acknowledge,"Ack",false,"u",true,true},
	{NMT_C2S_RequestConnection,"Request connection",true,"",true,false},
	{NMT_C2S_ReportPlayersMovements,"Report players movements",false,"",true,true},
	{NMT_C2S_RequestNewPlayer,"Request new player", true,"ub",true,false},
	{NMT_C2S_PlayerDisconnects,"Player disconnects", true,"u",true,false},
	{NMT_C2S_RequestNewMissile,"Request new missile", true,"8ufff",true,false},

	{NMT_S2C_DefineMap,"Define map", true,"ssu",false,true},
	{NMT_S2C_Refusal,"Refusal",true,"u",false,true},
	{NMT_S2C_PlayerDeath,"Player death",true,"uu",false,true},
	{NMT_S2C_NewPlayer,"Report player creation",true,"uffffuuub",false,true},
	{NMT_S2C_ReportPMPositions,"Report players & missiles positions", false,"",false,true},
	{NMT_RequestDisconnection,"Request client disconnection",true,"",true,true},
	{NMT_S2C_SpawnPlayer,"Spawn player",true,"uffff",false,true},
	{NMT_C2S_GetServerInfo, "Get server information",false,"",true,false},
	{NMT_S2C_ServerInfo, "Report server information",true,"S",false,true}
};


struct GetServerInfoM {
};
struct ServerInfoM {
	char *name;
};

size_t format_size(const char *s);

struct RefusalM {
	Uint32 seqid; /* refusal to the message defined by this seqid. Applies to C2S_RequestConnection, C2S_RequestNewPlayer and C2S_RequestNewMissile */
};
struct PlayerDeathM {
	Uint32 killerPlayer;
	Uint32 killedPlayer;
};
struct RequestDisconnectionM {
};
struct ReceivedMessage {
	ReceivedMessage(Uint32 mseqid):seqid(mseqid) {}
	sf::Clock receptionTime;
	Uint32 seqid;
};
class Message {
	public:
	Message();
	Message(const void *data, NetworkMessageType type);
	bool DefineInput(sf::Packet &ipacket);
	void DefineOutput(const void *data, NetworkMessageType type);
	bool Input(void *data, const char *format);
	bool InputVector(void *data, const char *format, size_t count);
	bool Append(const void *data, const char *format);
	bool Append(const void *data, const char *item_format, size_t item_count);
	template <class Item>
	bool AppendV(std::vector<Item> &items, const char *item_format) {
		return Append(&items[0], item_format, items.size());
	}
	template <class Item>
	bool InputV(std::vector<Item> &items, const char *item_format) {
		Uint32 cnt;
		if (!Input(&cnt, "u")) return false;
		if (cnt >= 256) {
			fprintf(stderr, "Error: Received malformed UDP packet with array length greater than 256\n");
			return false;
		}
		items.resize(cnt);
		return InputVector(&items[0], item_format, cnt);
	}
	RemoteClient client;
	Uint32 mseqid;
	bool must_acknowledge;
	enum NetworkMessageType type;
	sf::Clock lastTransmission;
	bool already_sent;

	sf::Packet packet;
	bool OutputToPacket(sf::Packet &opacket);
};
struct AcknowledgeM {
	Uint32 seqid;
};
struct RequestConnectionM {
};
struct PlayerPosition {
	Uint32 playerUID;
	float tank_angle, canon_angle;
	float x,y;
};
struct ApproxPlayerPosition { /* sent from server to client */
	Uint32 playerUID;
	Uint32 score;
	Uint16 x; /* 65536 = map_width */
	Uint16 y;
	Uint8 tank_angle; /* 256 = 2*pi radians */
	Uint8 canon_angle;
};
struct PlayerScore {
	Uint32 playerUID;
	Uint32 score;
};
struct MissilePosition {
	Uint32 missileUID;
	Uint32 origPlayer;
	Uint16 curAngle;
	Uint16 cur_x;
	Uint16 cur_y;
	Uint8 padding1, padding2;
};
struct PlayerMovement { /* sent from client to server to notify how the client moved between the previous packet and this one */
	Int64 tmEnd;
	PlayerPosition latestPosition;
	float latestSpeed_x, latestSpeed_y;
};
struct PMPositionsM {
	std::vector<ApproxPlayerPosition> players;
	std::vector<MissilePosition> missiles;
	std::vector<PlayerScore> additionnal_scores;
};
struct PlayerMovementsM {
	std::vector<PlayerMovement> movements;
};
struct RequestNewPlayerM {
	Uint32 color;
	bool preferred_color;
};
struct PlayerDisconnectsM {
	Uint32 playerUID;
};
struct RequestNewMissileM {
	Int64 origTime;
	Uint32 origPlayer;
	float x,y;
	float origAngle;
};
struct NewPlayerM {
	PlayerPosition pos;
	Uint32 color;
	Uint32 seqid; /* in response to this RequestNewPlayer packet */
	bool is_yours;
};
struct BlockPoint {
	unsigned short x, y;
};
typedef std::vector<BlockPoint> ApproxPolygon;
struct BlockHeader {
	char *texture_name;
	float angle;
	Uint32 color;
	TextureScales scales;
	float textureAngle;
	Uint32 mapping;
};
struct BlockM {
	BlockHeader h;
	ApproxPolygon apolygon;
};

struct DefineMapM {
	unsigned short width, height;
	unsigned block_count;
};/* followed by std::vector<Block> blocks; */
struct PlayerCreation {
	Uint32 seqid; /* seqid of the creation request */
	Controller *controller;
};

/* Send all physical controller info to server.
 * Receive movement directives from server and apply them
 */
struct RemoteClientInfo {
	RemoteClient client;
	sf::Clock lastPacketTime;
	RemoteClientInfo(const RemoteClient &rc);
};
struct ServerInfo {
	bool operator ==(const ServerInfo &oth) const {return name == oth.name && remote == oth.remote;}
	std::string name;
	RemoteClient remote;
};
class NetworkClient {
	public:
	typedef std::vector<ServerInfo>::iterator ServerInfoIterator;
	NetworkClient(Engine *engine);
	~NetworkClient();
	Engine *getEngine(void) {return engine;}
	
	bool isServer(void) const;
	bool isClient(void) const;
	bool isLocal(void) const;
	void declareAsServer(void);
	bool requestConnection(const RemoteClient &server);
	void requestDisconnection(void);
	void requestPlayerCreation(Controller *controller, const sf::Color *color = NULL);
	void requestMissileCreation(Missile *missile_template);
	void reportPlayerMovement(const PlayerMovement &plpos);
	void reportNewPlayer(Player *player, Uint32 toseqid, const RemoteClient &creator, const RemoteClient &target);
	void reportNewPlayer(Player *player, const RemoteClient *target = NULL);
	void reportPlayerDeath(Player *killing, Player *dying);
	void reportPlayerSpawned(const PlayerPosition &ppos);

	bool transmitToServer(); /* returns false if no packet is transmitted to the server */
	bool receiveFromServer(); /* returns false if nothing new has been received from the server */

	bool isPendingPlayerConcerned(const sf::Event &e) const;

	void discoverServers(bool wait_now);
	bool discoveringServers(void);
	bool discoverMoreServers(void);
	void endServerDiscovery(void);
	ServerInfoIterator begin_servers();
	ServerInfoIterator end_servers();

	private:
	sf::Clock discovery_clock, discovery_period_clock;
	void willSendMessage(Message *msg); /* used to report one message (structure is allocated by caller, freed by callee) */
	bool is_server;
	std::vector<ServerInfo> discoveredServers;
	std::vector<PlayerMovement> plmovements; /* used on client */
	std::vector<RemoteClientInfo> pairs;
	Engine *engine;
	sf::Clock c2s_time;
	UdpConnection remote;
	std::vector<Message*> c2sMessages;
	std::vector<ReceivedMessage> recMessages; /* received messages */
	std::vector<PlayerCreation> wpc;
	Uint32 last_pm_seqid;
	sf::Clock cleanup_clock;
	bool transmitPacket(sf::Packet &pkt, const RemoteClient *remote);
	bool transmitMessage(Message *msg);
	bool receivePacket(sf::Packet &pkt, RemoteClient *remote);
	bool transmitMessageSet(std::vector<Message*> &messages);
	bool treatMessage(Message &msg);
	void setMissilePosition(MissilePosition &mpos);
	void setPlayerPosition(PlayerPosition &ppos);
	void setPlayerPosition(ApproxPlayerPosition &ppos);
	void Acknowledge(const Message &msg);
	void reportPlayerAndMissilePositions(void);
	void setPlayerScore(const PlayerScore &score);
	void cleanupClients(void);
	void disconnectClient(const RemoteClient &client);
	void dropOldReceivedMessages(void);
	bool isDuplicate(const Message &msg);
	void declareNewReceivedMessage(const Message &msg);
	void serverDisconnected(void);
	void removeRemote(const RemoteClient &client);
	void removePair(const RemoteClient &client);
	void clear_all(void);
};
class MasterController: public Controller {
	public:
	MasterController(NetworkClient *client, Controller *phy);
	~MasterController();
	Controller *stealOrigController(void);
	Controller *getOrigController(void);
	virtual void reportMissileMovement(Missile *missile, MissileControllingData &mcd);
	virtual void reportPlayerMovement(Player *player, PlayerControllingData &pcd);
	virtual bool missileCreation(Missile *ml);
	virtual bool missileCollision(Missile *ml, Player *other);
	private:
	Controller *phy;
	NetworkClient *client;
};
class RemoteController: public Controller {
	public:
	RemoteController(NetworkClient *client, const RemoteClient &masterClient);
	~RemoteController();
	virtual void reportMissileMovement(Missile *missile, MissileControllingData &mcd);
	virtual void reportPlayerMovement(Player *player, PlayerControllingData &pcd);
	virtual bool missileCollision(Missile *ml, Player *other);
	virtual void teleported(void);
	RemoteClient getMasterClient(void) const;
	private:
	NetworkClient *client;
	RemoteClient masterClient;
};
#endif

