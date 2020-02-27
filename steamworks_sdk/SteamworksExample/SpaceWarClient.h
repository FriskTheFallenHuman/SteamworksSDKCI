//========= Copyright � 1996-2008, Valve LLC, All rights reserved. ============
//
// Purpose: Main class for the space war game client
//
// $NoKeywords: $
//=============================================================================

#ifndef SPACEWARCLIENT_H
#define SPACEWARCLIENT_H


#include "GameEngine.h"
#include "SpaceWar.h"
#include "Messages.h"
#include "StarField.h"
#include "Sun.h"
#include "Ship.h"
#include "steam\steam_api.h"
#include "StatsAndAchievements.h"
#include "BaseMenu.h"

// Forward class declaration
class CMainMenu; 
class CQuitMenu;
class CSpaceWarServer;
class CServerBrowser;
class CLobbyBrowser;
class CLobby;

// Height of the HUD font
#define HUD_FONT_HEIGHT 18

// Height for the instructions font
#define INSTRUCTIONS_FONT_HEIGHT 24

// Enum for various client connection states
enum EClientConnectionState
{
	k_EClientNotConnected,							// Initial state, not connected to a server
	k_EClientConnectedPendingAuthentication,		// We've established communication with the server, but it hasn't authed us yet
	k_EClientConnectedAndAuthenticated,				// Final phase, server has authed us, we are actually able to play on it
};

// a game server as shown in the find servers menu
struct ServerBrowserMenuData_t
{
	EClientGameState m_eStateToTransitionTo;
	uint32 m_unIPAddress;
	int32 m_nConnectionPort;
};

// a lobby as shown in the find lobbies menu
struct LobbyBrowserMenuItem_t
{
	CSteamID m_steamIDLobby;
	EClientGameState m_eStateToTransitionTo;
};

// a user as shown in the lobby screen
struct LobbyMenuItem_t
{
	CSteamID m_steamIDUser;		// the user who this is in the lobby
	bool m_bStartGame;
	bool m_bToggleReadyState;
	bool m_bLeaveLobby;
};



class CSpaceWarClient 
{
public:
	//Constructor
	CSpaceWarClient( CGameEngine *pEngine, CSteamID steamIDUser );

	// Destructor
	~CSpaceWarClient();

	// Run a game frame
	void RunFrame();

	// Checks for any incoming network data, then dispatches it
	void ReceiveNetworkData();

	// Connect to a server at a given IP address or game server steamID
	void InitiateServerConnection( uint32 unServerAddress, const int32 nPort );
	void InitiateServerConnection( CSteamID steamIDGameServer );

	// Send data to a client at the given ship index
	bool BSendServerData( char *pData, uint32 nSizeOfData, bool bSendReliably );

	// Menu callback handler (handles a bunch of menus that just change state with no extra data)
	void OnMenuSelection( EClientGameState eState ) { SetGameState( eState ); }

	// Menu callback handler (handles server browser selections with extra data)
	void OnMenuSelection( ServerBrowserMenuData_t selection ) 
	{ 
		if ( selection.m_eStateToTransitionTo == k_EClientGameConnecting )
		{
			InitiateServerConnection( selection.m_unIPAddress, selection.m_nConnectionPort );
		}
		else
		{
			SetGameState( selection.m_eStateToTransitionTo ); 
		}
	}

	void OnMenuSelection( LobbyBrowserMenuItem_t selection )
	{
		// start joining the lobby
		if ( selection.m_eStateToTransitionTo == k_EClientJoiningLobby )
		{
			SteamMatchmaking()->JoinLobby( selection.m_steamIDLobby );

			// the callback LobbyEnter_t will be received when we've joined
		}

		SetGameState( selection.m_eStateToTransitionTo );
	}

	void OnMenuSelection( LobbyMenuItem_t selection );

	// Set game state
	void SetGameState( EClientGameState eState );
	EClientGameState GetGameState() { return m_eGameState; }

	// set failure text
	void SetConnectionFailureText( const char *pchErrorText );

	// Were we the winner?
	bool BLocalPlayerWonLastGame();

	// Get the steam id for the local user at this client
	CSteamID GetLocalSteamID() { return m_SteamIDLocalUser; }

	// Get the local players name
	const char* GetLocalPlayerName() { return SteamFriends()->GetFriendPersonaName( m_SteamIDLocalUser ); }

	// Scale screen size to "real" size
	float PixelsToFeet( float flPixels );

	// Get a Steam-supplied image
	HGAMETEXTURE GetSteamImageAsTexture( int iImage );

private:

	// Receive a response from the server for a connection attempt
	void OnReceiveServerInfo( CSteamID steamIDGameServer, bool bVACSecure, const char *pchServerName );

	// Receive a response from the server for a connection attempt
	void OnReceiveServerAuthenticationResponse( bool bSuccess, uint32 uPlayerPosition );

	// Receive a state update from the server
	void OnReceiveServerUpdate( ServerSpaceWarUpdateData_t UpdateData );

	// Handle the server exiting
	void OnReceiveServerExiting();

	// Disconnects from a server (telling it so) if we are connected
	void DisconnectFromServer();

	// game state changes
	void OnGameStateChanged( EClientGameState eGameStateNew );

	// Draw the HUD text (should do this after drawing all the objects)
	void DrawHUDText();

	// Draw instructions for how to play the game
	void DrawInstructions();

	// Draw text telling the players who won (or that their was a draw)
	void DrawWinnerDrawOrWaitingText();

	// Draw text telling the user that the connection attempt has failed
	void DrawConnectionFailureText();

	// Draw connect to server text
	void DrawConnectToServerText();

	// Draw text telling the user a connection attempt is in progress
	void DrawConnectionAttemptText();

	// Server we are connected to
	CSpaceWarServer *m_pServer;

	// SteamID for the local user on this client
	CSteamID m_SteamIDLocalUser;

	// Our ship position in the array below
	uint32 m_uPlayerShipIndex;

	// List of steamIDs for each player
	CSteamID m_rgSteamIDPlayers[MAX_PLAYERS_PER_SERVER];

	// Ships for players, doubles as a way to check for open slots (pointer is NULL meaning open)
	CShip *m_rgpShips[MAX_PLAYERS_PER_SERVER];

	// Player scores
	uint32 m_rguPlayerScores[MAX_PLAYERS_PER_SERVER];

	// Who just won the game? Should be set if we go into the k_EGameWinner state
	uint32 m_uPlayerWhoWonGame;

	// Current game state
	EClientGameState m_eGameState;

	// true if we only just transitioned state
	bool m_bTransitionedGameState;

	// Font handle for drawing the HUD text
	HGAMEFONT m_hHUDFont;

	// Font handle for drawing the instructions text
	HGAMEFONT m_hInstructionsFont;

	// Time the last state transition occurred (so we can count-down round restarts)
	uint64 m_ulStateTransitionTime;

	// Time we started our last connection attempt
	uint64 m_ulLastConnectionAttemptRetryTime;

	// Time we last got data from the server
	uint64 m_ulLastNetworkDataReceivedTime;

	// Text to display if we are in an error state
	char m_rgchErrorText[256];

	// Socket to use when communicating with servers
	SNetSocket_t m_hSocketClient;

	// Server address data
	uint32 m_unServerIP;
	uint16 m_usServerPort;

	// Track whether we are connected to a server (and what specific state that connection is in)
	EClientConnectionState m_eConnectedStatus;

	// Star field instance
	CStarField *m_pStarField;

	// Sun instance
	CSun *m_pSun;

	// Main menu instance
	CMainMenu *m_pMainMenu;

	// Pause menu instance
	CQuitMenu *m_pQuitMenu;

	// pointer to game engine instance we are running under
	CGameEngine *m_pGameEngine;

	// track which steam image indexes we have textures for, and what handle that texture has
	std::map<int, HGAMETEXTURE> m_MapSteamImagesToTextures;

	CStatsAndAchievements *m_pStatsAndAchievements;

	CServerBrowser *m_pServerBrowser;

	// lobby handling
	bool m_bCreatingLobby;
	// the name of the lobby we're connected to
	CSteamID m_steamIDLobby;
	// callback for when we're creating a new lobby
	STEAM_CALLBACK( CSpaceWarClient, OnLobbyCreated, LobbyCreated_t, m_LobbyCreatedCallback );
	// callback for when we've joined a lobby
	STEAM_CALLBACK( CSpaceWarClient, OnLobbyEntered, LobbyEnter_t, m_LobbyEnteredCallback );
	// callback for when the lobby game server has started
	STEAM_CALLBACK( CSpaceWarClient, OnLobbyGameCreated, LobbyGameCreated_t, m_LobbyGameCreated );
	
	// lobby browser menu
	CLobbyBrowser *m_pLobbyBrowser;

	// local lobby display
	CLobby *m_pLobby;

	// connection handler
	STEAM_CALLBACK( CSpaceWarClient, OnSocketStatusCallback, SocketStatusCallback_t, m_SocketStatusCallback );
};

extern CSpaceWarClient *g_pSpaceWarClient;
CSpaceWarClient *SpaceWarClient();

#endif // SPACEWARCLIENT_H
