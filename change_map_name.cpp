#include <stdio.h>
#include <string>
#include <cstring>
#include "change_map_name.h"
#include "metamod_oslink.h"
#include "schemasystem/schemasystem.h"
#include "interfaces/interfaces.h"
#include "iserver.h"
#include "filesystem.h"
#include "iloopmode.h"
#include "steam/steam_gameserver.h"

change_map_name g_change_map_name;
PLUGIN_EXPOSE(change_map_name, g_change_map_name);
IVEngineServer2* engine = nullptr;
CGameEntitySystem* g_pGameEntitySystem = nullptr;
CEntitySystem* g_pEntitySystem = nullptr;
CGlobalVars *gpGlobals = nullptr;

IUtilsApi* g_pUtils;

static const char* kConfigPath = "addons/configs/change_map_name.ini";
static std::string g_ServerInfoSuffix = "";
static std::string g_OriginalMapName = "";

static void LoadConfig()
{
	if (!g_pFullFileSystem)
		return;

	KeyValues* hKv = new KeyValues("ChangeModeName");
	if (!hKv->LoadFromFile(g_pFullFileSystem, kConfigPath))
	{
		ConColorMsg(Color(255, 0, 0, 255), "[change_map_name] Failed to load %s\n", kConfigPath);
		hKv->deleteThis();
		return;
	}

	g_ServerInfoSuffix = hKv->GetString("suffix", "");
	hKv->deleteThis();
}

static void ModifyGameConfig()
{
	if (!g_pNetworkServerService)
		return;

	CNetworkGameServerBase* pServer = g_pNetworkServerService->GetIGameServer();
	if (!pServer)
		return;

	const char* suffix = g_ServerInfoSuffix.c_str();
	if (!suffix || !suffix[0])
		return;

	const char* currentMapName = pServer->GetMapName();
	if (!currentMapName || !currentMapName[0])
		return;

	g_OriginalMapName = currentMapName;
	
	if (g_OriginalMapName.find(suffix) != std::string::npos)
		return;

	std::string newMapName = g_OriginalMapName + " | " + suffix;

	pServer->SetMapName(newMapName.c_str());

	ISteamGameServer* pSteamGameServer = SteamGameServer();
	if (pSteamGameServer)
	{
		pSteamGameServer->SetMapName(g_OriginalMapName.c_str());
	}
}

void StartupServer()
{
	g_pGameEntitySystem = GameEntitySystem();
	g_pEntitySystem = g_pUtils->GetCEntitySystem();
	gpGlobals = g_pUtils->GetCGlobalVars();

	ModifyGameConfig();
	
	static bool s_bTimerStarted = false;
	if (s_bTimerStarted) return;
	s_bTimerStarted = true;
	g_pUtils->CreateTimer(1.0f, []() {
		if (!g_OriginalMapName.empty())
		{
			ISteamGameServer* pSteamGameServer = SteamGameServer();
			if (pSteamGameServer)
			{
				pSteamGameServer->SetMapName(g_OriginalMapName.c_str());
			}
		}
		return 0.0f;
	});
}

bool change_map_name::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, g_pGameResourceServiceServer, IGameResourceService, GAMERESOURCESERVICESERVER_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, g_pSchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pNetworkServerService, INetworkServerService, NETWORKSERVERSERVICE_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);

	g_SMAPI->AddListener( this, this );

	return true;
}

bool change_map_name::Unload(char *error, size_t maxlen)
{
	ConVar_Unregister();
	
	return true;
}

void change_map_name::AllPluginsLoaded()
{
	char error[64];
	int ret;
	g_pUtils = (IUtilsApi *)g_SMAPI->MetaFactory(UTILS_INTERFACE, &ret, NULL);
	if (ret == META_IFACE_FAILED)
	{
		g_SMAPI->Format(error, sizeof(error), "Missing Utils system plugin");
		ConColorMsg(Color(255, 0, 0, 255), "[%s] %s\n", GetLogTag(), error);
		std::string sBuffer = "meta unload "+std::to_string(g_PLID);
		engine->ServerCommand(sBuffer.c_str());
		return;
	}
	LoadConfig();
	g_pUtils->StartupServer(g_PLID, StartupServer);
}

const char* change_map_name::GetLicense()
{
	return "GPL";
}

const char* change_map_name::GetVersion()
{
	return "1.0";
}

const char* change_map_name::GetDate()
{
	return __DATE__;
}

const char *change_map_name::GetLogTag()
{
	return "[change_map_name]";
}

const char* change_map_name::GetAuthor()
{
	return "ABKAM";
}

const char* change_map_name::GetDescription()
{
	return "Change Map Name";
}

const char* change_map_name::GetName()
{
	return "Change Map Name";
}

const char* change_map_name::GetURL()
{
	return "https://discord.gg/ChYfTtrtmS";
}
