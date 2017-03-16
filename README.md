# LuaCppMsg
A thread-safe message queue between Lua and C++, designed with the aim of having a single 
Lua state communicate with multiple C++ threads in fast and flexible way.

Uses [lua wrapper](https://github.com/ahupowerdns/luawrapper) and its support for Lua table 
conversion from/to 
[`std::unordered_map`](http://en.cppreference.com/w/cpp/container/unordered_map)s 
of [`boost::variant`](http://www.boost.org/doc/libs/1_61_0/doc/html/variant.html) types.

By default supports messages of `std::string` and 
recursive `std::unordered_map`s, with keys of `int` or `std::string`.  Any number of additional 
value types are then added via the (variadic) template parameter, and are then available as both 
message items in their own right, as well as values within a map (or Lua table/array).

The library is header-only and has the same dependencies as 
[lua wrapper](https://github.com/ahupowerdns/luawrapper) (boost headers and  C++11).  To run the 
tests you will need `LuaJIT` and `pthreads` - there is a `CMakeLists.txt`
for that.

### Limitations
Note that `boost::variant` has it's limitations.  In-particular, when using numeric types
alongside one-another (including `bool`), then `boost::variant` can often store the value as a 
type you don't expect (and thus causes `bad_get` exceptions when you attempt to retrive the value).
Altering the order of the template parameters can, in practice, also affect which type is 
preferred as the storage type.

## Documentation
The best documentation can be found in `src/tests/test.cpp`.  It uses the excellent
[Catch](https://github.com/philsquared/Catch) C++ BDD-style testing library, so is quite readable.

### Basics
```
// Simple queue with no user-defined types.
using SimpleQueue = Queue<double>;
// Create the queue and bind and expose to Lua state.
// (you can also default-construct the queue and `bind` and `to_lua` it some time later).
SimpleQueue queue(L, "lqueue");
// Get a pointer to the `LuaContext`, for convenience. 
SimpleQueue::Lua lua = queue.lua();
```
```
WHEN("we try to pop an empty queue")
{
	SimpleQueue::Opt msg_exists = queue.pop();

	THEN("the message evaluates as falsey")
	{
		CHECK(!msg_exists);
		CHECK(!msg_exists.is_initialized());
	}
}

WHEN("we push a number to the queue")
{
	queue.push(5.4);

	THEN("the queue size is 1")
	{
		CHECK(queue.size() == 1);
	}

	AND_WHEN("we pop the number from the queue")
	{
		SimpleQueue::Opt msg_exists = queue.pop();

		THEN("the queue size is 0")
		{
			CHECK(queue.size() == 0);
		}

		THEN("the message evaluates as truthy")
		{
			CHECK(msg_exists);
			CHECK(msg_exists.is_initialized());
		}

		AND_WHEN("we dereference the optional")
		{
			SimpleQueue::Msg msg = *msg_exists;

			THEN("the number stored in the message is correct")
			{
				CHECK(msg.as<double>() == 5.4);
			}
		}
	}
}
```		

### Push in C++, pop in Lua

```
WHEN("we push a map to the queue from C++")
{
	SimpleQueue::Map msg_map{
		{ "type", SimpleQueue::Str("MOCK MESSAGE") },
		{ "nested", SimpleQueue::Map{{ 2, 4.9 }} },
		{ 7, 3.1 }
	};

	queue.push(msg_map);

	AND_WHEN("we pop the table from the queue from Lua")
	{
		lua->executeCode("item = lqueue:pop()");

		THEN("the table is correct")
		{
			lua->executeCode("type = item.type");
			lua->executeCode("nested_num = item.nested[2]");
			lua->executeCode("indexed_num = item[7]");

			SimpleQueue::Str type = lua->readVariable<SimpleQueue::Str>("type");
			double nested_num = lua->readVariable<double>("nested_num");
			double indexed_num = lua->readVariable<double>("indexed_num");

			CHECK(type == "MOCK MESSAGE");
			CHECK(nested_num == 4.9);
			CHECK(indexed_num == 3.1);
		}
	}
}
```

### Push in Lua, pop in C++
```
WHEN("we push a table to the queue from Lua")
{
	lua->executeCode(
		"lqueue:push({"
		"  type=\"MOCK MESSAGE\", nested={[2]=4.9}, [7]=3.1"
		"})"
	);

	AND_WHEN("we pop the table from the queue from C++")
	{
		SimpleQueue::Msg msg = *queue.pop();

		THEN("the map is correct")
		{
			CHECK(msg.get("type").as<SimpleQueue::Str>() == "MOCK MESSAGE");
			CHECK(msg.get("nested").get(2).as<double>() == 4.9);
			CHECK(msg.get(7).as<double>() == 3.1);
		}
	}
}
```
