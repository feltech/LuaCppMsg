#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <lua.hpp>

lua_State* L;

int main( int argc, char* const argv[] )
{
	L = luaL_newstate();

	int result = Catch::Session().run( argc, argv );

	lua_close(L);

	return result;
}
