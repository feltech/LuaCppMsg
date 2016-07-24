# LuaCppMsg
A thread-safe message queue between Lua and C++, designed with the aim of having a single 
Lua state communicate with multiple C++ threads in fast and flexible way.

Uses [lua wrapper](https://github.com/ahupowerdns/luawrapper) and it's support for Lua table 
conversion from/to `std::unordered_map`s of `boost::variant` types.

Supports messages that are simple types `bool`, `double` and `std::string`, as well as 
recursive `std::unordered_map`s with keys of `int` or `std::string` and values of `bool`, 
`double` or `std::string` (or another `std::unordered_map`).

The library is header-only and has the same dependencies as `luawrapper` (`boost` headers and 
C++11).  To run the tests you will need `LuaJIT` and `pthreads` - there is a `CMakeLists.txt`
for that.

## Documentation
The best documentation can be found in `src/tests/test.cpp`.  It uses the excellent
[Catch](https://github.com/philsquared/Catch) C++ BDD-style testing library, so is quite readable.

A sneak peak:

**Push in C++, pop in Lua**
```
WHEN("we push a map to the queue from C++")
{
	Message::Map msg_map{
		{ "type", Message::Str("MOCK MESSAGE") },
		{ "nested", Message::Map{{ "a bool", true }} },
		{ 7, 3.1 }
	};

	queue.push(Message(msg_map));

	AND_WHEN("we pop the table from the queue from Lua")
	{
		lua->executeCode("item = lqueue:pop()");

		THEN("the table is correct")
		{
			lua->executeCode("type = item.type");
			lua->executeCode("nested_bool = item.nested[\"a bool\"]");
			lua->executeCode("indexed_num = item[7]");

			Message::Str type = lua->readVariable<Message::Str>("type");
			Message::Bool nested_bool = lua->readVariable<Message::Bool>("nested_bool");
			Message::Num indexed_num = lua->readVariable<Message::Num>("indexed_num");

			CHECK(type == "MOCK MESSAGE");
			CHECK(nested_bool == true);
			CHECK(indexed_num == 3.1);
		}
	}
}
```

**Push in Lua, pop in C++**
```
WHEN("we push a table to the queue from Lua")
{
	lua->executeCode(
		"lqueue:push({"
		"  type=\"MOCK MESSAGE\", nested={[\"a bool\"]=true}, [7]=3.1"
		"})"
	);

	AND_WHEN("we pop the table from the queue from C++")
	{
		Message msg = *queue.pop();

		THEN("the map is correct")
		{
			CHECK(msg.get("type").as<Message::Str>() == "MOCK MESSAGE");
			CHECK(msg.get("nested").get("a bool").as<Message::Bool>() == true);
			CHECK(msg.get(7).as<Message::Num>() == 3.1);
		}
	}
}
```


