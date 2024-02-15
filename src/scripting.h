/**
 * @file scripting.h
 * Declarations related to scripting subsystem.
 */

#ifndef PCT_SCRIPTING
#define PCT_SCRIPTING

#include <lua5.4/lua.h>
#include <lua5.4/lualib.h>
#include <lua5.4/lauxlib.h>
#include <stdlib.h>

typedef struct ScriptingContext{
    lua_State *L;
} PCT_LuaScriptingContext;

/**
 * @brief Initializes new scripting context for running Lua scripts in
 * User should call PCT_DestroyLuaScripting to free any resources created by init.
 * @return PCT_LuaScriptingContext pointer that should be passed to all functions in this module
 */ 
PCT_LuaScriptingContext *PCT_InitLuaScripting(void);

/**
 * @brief Cleans up any resources allocated by Lua scripting module.
 * @param ctx pointer to PCT_LuaScriptingContext for which resources should be cleared.
 */
void PCT_DestroyLuaScripting(PCT_LuaScriptingContext *ctx);

#endif // PCT_SCRIPTING
