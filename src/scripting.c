#include "scripting.h"

PCT_LuaScriptingContext *PCT_InitLuaScripting(void){
    PCT_LuaScriptingContext *ctx = malloc(sizeof(PCT_LuaScriptingContext));
    ctx->L = luaL_newstate();
    luaL_openlibs(ctx->L);
    return ctx;
}

void PCT_DestroyLuaScripting(PCT_LuaScriptingContext *ctx){
    lua_close(ctx->L);
    free(ctx);
}

    // -------------LUA-----------------
    
    
    // if(luaL_dofile(LuaState, "robots/init.lua") != LUA_OK){
    //     SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", lua_tostring(LuaState, -1));
    // }
    
    // lua_pop(LuaState, lua_gettop(LuaState));
    // -------------END LUA-------------