#ifndef COM_MODULES_OLDLOADING
#include "$CurrentDir:missions\\DayZCommunityOfflineMode.Sakhal\\core\\BaseModuleInclude.c"
#endif
/*
    Define used for optional compilations
*/
#define MODULE_LOOT_DEBUG

/*
    Include of all .c files that belong to this module
*/
#ifdef COM_MODULES_OLDLOADING
#include "$CurrentDir:missions\\DayZCommunityOfflineMode.Sakhal\\core\\modules\\LootDebug\\gui\\LootDebugMenu.c"
#include "$CurrentDir:missions\\DayZCommunityOfflineMode.Sakhal\\core\\modules\\LootDebug\\LootDebug.c"
#endif

#ifndef COM_MODULES_OLDLOADING
void RegisterModule()
{
    COM_GetModuleManager().RegisterModule( new LootDebug );
}
#endif
