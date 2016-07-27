# LuaCppMsg
A thread-safe message queue between Lua and C++, designed with the aim of having a single 
Lua state communicate with multiple C++ threads in fast and flexible way.

Supports messages of simple types `bool`, `double` and `std::string`, as well as 
recursive [std::unordered_map](http://en.cppreference.com/w/cpp/container/unordered_map)s, with 
keys of `int` or `std::string`, and values of `bool`, 
`double` or `std::string`, or another `std::unordered_map`.  Custom types can be added via the 
(variadic) template parameter.

Uses [lua wrapper](https://github.com/ahupowerdns/luawrapper) and it's support for Lua table 
conversion from/to `std::unordered_map`s 
of [boost::variant](http://www.boost.org/doc/libs/1_61_0/doc/html/variant.html) types.

The library is header-only and has the same dependencies as 
[lua wrapper](https://github.com/ahupowerdns/luawrapper) (boost headers and  C++11).  To run the 
tests you will need `LuaJIT` and `pthreads` - there is a `CMakeLists.txt`
for that.

## Documentation
The best documentation can be found in `src/tests/test.cpp`.  It uses the excellent
[Catch](https://github.com/philsquared/Catch) C++ BDD-style testing library, so is quite readable.


###Basics
```
// Simple queue with no user-defined types.
using SimpleQueue = Queue<>;
// Create the queue and bind and expose to Lua state.
SimpleQueue queue(L, "lqueue");
// Get a pointer to the `LuaContext`, for convenience. 
SimpleQueue::Lua lua = queue.lua();
```
```
WHEN("we try to pop an empty queue")
{
	SimpleQueue::Opt msg = queue.pop();

	THEN("the message evaluates as falsey")
	{
		CHECK(!msg);
		CHECK(!msg.is_initialized());
	}
}
```
```
WHEN("we push a number to the queue")
{
	queue.push(SimpleQueue::Msg(5.4));

	THEN("the queue size is 1")
	{
		CHECK(queue.size() == 1);
	}

	AND_WHEN("we pop the number from the queue")
	{
		SimpleQueue::Msg msg = *queue.pop();

		THEN("the queue size is 0")
		{
			CHECK(queue.size() == 0);
		}

		THEN("the number is correct")
		{
			CHECK(msg.as<SimpleQueue::Num>() == 5.4);
		}
	}
}
```		

###Push in C++, pop in Lua

```
WHEN("we push a map to the queue from C++")
{
	SimpleQueue::Map msg_map{
		{ "type", SimpleQueue::Str("MOCK MESSAGE") },
		{ "nested", SimpleQueue::Map{{ "a bool", true }} },
		{ 7, 3.1 }
	};

	queue.push(SimpleQueue::Msg(msg_map));

	AND_WHEN("we pop the table from the queue from Lua")
	{
		lua->executeCode("item = lqueue:pop()");

		THEN("the table is correct")
		{
			lua->executeCode("type = item.type");
			lua->executeCode("nested_bool = item.nested[\"a bool\"]");
			lua->executeCode("indexed_num = item[7]");

			SimpleQueue::Str type = lua->readVariable<SimpleQueue::Str>("type");
			SimpleQueue::Bool nested_bool = lua->readVariable<SimpleQueue::Bool>("nested_bool");
			SimpleQueue::Num indexed_num = lua->readVariable<SimpleQueue::Num>("indexed_num");

			CHECK(type == "MOCK MESSAGE");
			CHECK(nested_bool == true);
			CHECK(indexed_num == 3.1);
		}
	}
}
```

###Push in Lua, pop in C++
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
		SimpleQueue::Msg msg = *queue.pop();

		THEN("the map is correct")
		{
			CHECK(msg.get("type").as<SimpleQueue::Str>() == "MOCK MESSAGE");
			CHECK(msg.get("nested").get("a bool").as<SimpleQueue::Bool>() == true);
			CHECK(msg.get(7).as<SimpleQueue::Num>() == 3.1);
		}
	}
}
```

###Use custom types
```
using ExQueue = Queue<CustomType>;
ExQueue queue(L, "lqueue");
```

```
WHEN("we push in C++ and pop in Lua")
{
	queue.push(ExQueue::Msg(CustomType(5)));

	lua->executeCode(
		"lcustom_popped = lqueue:pop()\n"
		"val = lcustom_popped.val\n"
	);

	THEN("the value is correct")
	{
		int val = lua->readVariable<int>("val");

		CHECK(val == 5);
	}
}

WHEN("we push in Lua and pop in C++")
{
	lua->writeFunction("LCustom", [](int v) {
		return CustomType(v);
	});

	lua->executeCode("lcustom = LCustom(5)");
	lua->executeCode("lqueue:push(lcustom)");

	THEN("the value is correct")
	{
		ExQueue::Msg msg = *queue.pop();
		
		CHECK(msg.as<CustomType>().val == 5);
	}
}
```



