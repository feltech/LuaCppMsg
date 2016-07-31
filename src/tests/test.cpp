#include "catch.hpp"

#include <thread>
#include <LuaCppMsg.hpp>

using namespace LuaCppMsg;

extern lua_State* L;


SCENARIO("Push and pop from C++")
{
	GIVEN("A queue")
	{
		using SimpleQueue = Queue<double>;

		SimpleQueue queue;

		THEN("the queue length is initially 0")
		{
			CHECK(queue.size() == 0);
		}

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

		WHEN("we push a string to the queue")
		{
			queue.push("MOCK MESSAGE");

			AND_WHEN("we pop the string from the queue")
			{
				SimpleQueue::Msg msg = *queue.pop();

				THEN("the string is correct")
				{
					CHECK(msg.as<SimpleQueue::Str>() == "MOCK MESSAGE");
				}
			}
		}

		WHEN("we push a map to the queue")
		{
			SimpleQueue::Map msg_map{
				{ "type", SimpleQueue::Str("MOCK MESSAGE") },
				{ "nested", SimpleQueue::Map{{ 2, 4.9 }} },
				{ 7, 3.1 }
			};
			queue.push(msg_map);

			THEN("the queue size is 1")
			{
				CHECK(queue.size() == 1);
			}

			AND_WHEN("we pop the message from the queue")
			{
				SimpleQueue::Msg msg = *queue.pop();

				THEN("the queue size is 0")
				{
					CHECK(queue.size() == 0);
				}

				THEN("the message map can be keyed by strings")
				{
					CHECK(msg.get("type").as<SimpleQueue::Str>() == "MOCK MESSAGE");
				}

				THEN("the message map can be keyed by integers")
				{
					CHECK(msg.get(7).as<double>() == 3.1);
				}

				THEN("we can get nested attributes")
				{
					CHECK(msg.get("nested").get(2).as<double>() == 4.9);
				}
			}
		}
	}
}


SCENARIO("Push and pop from Lua")
{
	GIVEN("A queue bound to lua")
	{
		using SimpleQueue = Queue<double>;

		SimpleQueue queue(L, "lqueue");
		SimpleQueue::Lua lua = queue.lua();

		THEN("the C++ queue and Lua queue are the same object")
		{
			SimpleQueue* pqueue = *lua->readVariable<boost::optional<SimpleQueue*>>("lqueue");
			CHECK(pqueue == &queue);
		}

		THEN("the queue length is initially 0")
		{
			lua->executeCode("queue_size = lqueue:size()");
			int queue_size = lua->readVariable<int>("queue_size");
		    CHECK(queue_size == 0);
		}

		WHEN("we try to pop a message from empty queue")
		{
			lua->executeCode("item = lqueue:pop()");

			THEN("the SimpleQueue is nil")
			{
				lua->executeCode("isnil = item == nil");
				int isnil = lua->readVariable<bool>("isnil");
			    CHECK(isnil);
			    CHECK(lua->readVariable<std::nullptr_t>("item") == nullptr);
			}
		}


		WHEN("we push a number to the queue")
		{
			lua->executeCode("lqueue:push(7)");

			THEN("the queue size is 1")
			{
				lua->executeCode("queue_size = lqueue:size()");
				int queue_size = lua->readVariable<int>("queue_size");
			    CHECK(queue_size == 1);
			}

			AND_WHEN("we pop the number from the queue")
			{
				lua->executeCode("item = lqueue:pop()");

				THEN("the queue size is 0")
				{
					lua->executeCode("queue_size = lqueue:size()");
					int queue_size = lua->readVariable<int>("queue_size");
				    CHECK(queue_size == 0);
				}

				THEN("the number is correct")
				{
					int item = lua->readVariable<int>("item");
					CHECK(item == 7);
				}
			}
		}

		WHEN("we push a string to the queue")
		{
			lua->executeCode("lqueue:push(\"a string\")");

			AND_WHEN("we pop the string from the queue")
			{
				lua->executeCode("item = lqueue:pop()");

				THEN("the string is correct")
				{
					SimpleQueue::Str item = lua->readVariable<SimpleQueue::Str>("item");
					CHECK(item == "a string");
				}
			}
		}

		WHEN("we push a table to the queue")
		{
			lua->executeCode(
				"lqueue:push({"
				"  type=\"MOCK MESSAGE\", nested={[2]=4.9}, [7]=3.1"
				"})"
			);

			AND_WHEN("we pop the table from the queue")
			{
				lua->executeCode("item = lqueue:pop()");

				THEN("the table is correct")
				{
					SimpleQueue::Map item = lua->readVariable<SimpleQueue::Map>("item");
					SimpleQueue::Msg msg(item);
					CHECK(msg.get("type").as<SimpleQueue::Str>() == "MOCK MESSAGE");
					CHECK(msg.get("nested").get(2).as<double>() == 4.9);
					CHECK(msg.get(7).as<double>() == 3.1);
				}
			}
		}
	}
}


SCENARIO("Push and pop between C++ and Lua")
{
	GIVEN("A queue bound to lua")
	{
		// Simple queue with no user-defined types.
		using SimpleQueue = Queue<double>;
		// Create the queue and bind and expose to Lua state.
		SimpleQueue queue(L, "lqueue");
		// Get a pointer to the `LuaContext`, for convenience.
		SimpleQueue::Lua lua = queue.lua();

		WHEN("we push a number to the queue from Lua")
		{
			lua->executeCode("lqueue:push(5.4)");

			AND_WHEN("we pop the number from the queue from C++")
			{
				SimpleQueue::Msg msg = *queue.pop();

				THEN("the number is correct")
				{
					CHECK(msg.as<double>() == 5.4);
				}
			}
		}

		WHEN("we push a string to the queue from Lua")
		{
			lua->executeCode("lqueue:push(\"a string\")");

			AND_WHEN("we pop the number from the queue from C++")
			{
				SimpleQueue::Msg msg = *queue.pop();

				THEN("the string is correct")
				{
					CHECK(msg.as<SimpleQueue::Str>() == "a string");
				}
			}
		}

		WHEN("we push a number to the queue from C++")
		{
			queue.push(5.4);

			AND_WHEN("we pop the number from the queue from Lua")
			{
				lua->executeCode("item = lqueue:pop()");

				THEN("the number is correct")
				{
					double item = lua->readVariable<double>("item");
					CHECK(item == 5.4);
				}
			}
		}

		WHEN("we push a string to the queue from C++")
		{
			queue.push("a string");

			AND_WHEN("we pop the number from the queue from Lua")
			{
				lua->executeCode("item = lqueue:pop()");

				THEN("the string is correct")
				{
					SimpleQueue::Str item = lua->readVariable<SimpleQueue::Str>("item");
					CHECK(item == "a string");
				}
			}
		}

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

		WHEN("we push an array to the queue from Lua")
		{
			lua->executeCode(
				"lqueue:push({"
				"  5, 6.1, \"a string\""
				"})"
			);

			AND_WHEN("we pop the array from the queue from C++")
			{
				SimpleQueue::Msg msg = *queue.pop();

				THEN("the map is correct")
				{
					CHECK(msg.get(1).as<double>() == 5);
					CHECK(msg.get(2).as<double>() == 6.1);
					CHECK(msg.get(3).as<SimpleQueue::Str>() == "a string");
				}
			}
		}

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
	}
}


SCENARIO("Multithreaded push/pop")
{
	GIVEN("C++ producers and consumers, with consumers consuming less than produced")
	{
		using SimpleQueue = Queue<double>;
		SimpleQueue queue(L, "lqueue");
		SimpleQueue::Lua lua = queue.lua();

		// Push a map with a "value" attribute that is just the queue size.
		auto producer = [&queue]() {
			for (unsigned i = 0; i < 100; i++)
			{
				queue.push(SimpleQueue::Map{
					{"type", SimpleQueue::Str("FROM C++")},
					{"value", queue.size()}
				});
			}
		};

		auto consumer = [&queue]() {
			for (unsigned i = 0; i < 95; i++)
				queue.pop();
		};

		std::array<std::thread, 5> producers;
		std::array<std::thread, 5> consumers;

		WHEN("we start the producer/consumer threads as well as a Lua producer/consumer")
		{
			for (std::thread& t : producers)
				t = std::thread(producer);
			for (std::thread& t : consumers)
				t = std::thread(consumer);

			lua->executeCode(
				"  for i=1,100,1\n"
				"  do\n"
				"    lqueue:pop()\n"
				"    lqueue:push({type=\"FROM LUA\", value=lqueue:size()})\n"
				"  end"
			);

			AND_WHEN("we wait for the threads to finish")
			{
				for (std::thread& t : producers)
					t.join();
				for (std::thread& t : consumers)
					t.join();

				THEN("the size of the queue is correct")
				{
					CHECK(queue.size() >= 5);
				}

				THEN("the queue has roughly correct elements")
				{
					SimpleQueue::Msg msg = *queue.pop();
					double num = msg.get("value").as<double>();

					unsigned queue_size = queue.size();
					unsigned num_luas = 0;
					unsigned num_cs = 0;

					while (SimpleQueue::Opt msg_exists = queue.pop())
					{
						const SimpleQueue::Msg& msg = *msg_exists;
						const SimpleQueue::Str& type = msg.get("type").as<const SimpleQueue::Str>();
						if (type == "FROM LUA")
							num_luas++;
						else if (type == "FROM C++")
							num_cs++;
						else
							FAIL();
					}

					INFO(
						queue_size << " elements: " << num_cs << " from C++, and " <<
						num_luas << " from Lua"
					);

					CHECK(0 <= num);
					CHECK(num <= 600);
				}
			}
		}
	}
}


struct CustomType
{
	CustomType()
	{
		vals = nullptr;
	}
	CustomType(const CustomType& other_)
	{
		val = other_.val;
		vals = new int[1];
		vals[0] = other_.vals[0];
	}
	CustomType(CustomType&& other_)
	{
		val = other_.val;
		vals = other_.vals;
		other_.vals = nullptr;
	}

	CustomType& operator=(const CustomType& other_)
	{
		val = other_.val;
		vals = other_.vals;
	}

	CustomType(int val_) : val(val_)
	{
		vals = new int[1];
		vals[0] = val_;
	}
	~CustomType()
	{
		delete[] vals;
	}
	int val;
	int* vals;
};


SCENARIO("Custom types")
{
	GIVEN("a queue with messages allowing a custom type")
	{
		using ExQueue = Queue<CustomType>;
		ExQueue queue(L, "lqueue");
		ExQueue::Lua lua = queue.lua();
		lua->registerMember("val", &CustomType::val);

		WHEN("we push and pop in C++")
		{
			queue.push(CustomType(3));
			ExQueue::Msg msg = *queue.pop();

			THEN("the value is correct")
			{
				CHECK(msg.as<CustomType>().val == 3);
			}
		}

		WHEN("we push and pop in Lua")
		{
			CustomType custom(7);

			lua->writeVariable("lcustom", custom);
			lua->executeCode(
				"val_before = lcustom.val\n"
				"lqueue:push(lcustom)\n"
				"lcustom_popped = lqueue:pop()\n"
				"val_after = lcustom_popped.val\n"
			);

			THEN("the value is correct")
			{
				int val_before = lua->readVariable<int>("val_before");
				int val_after = lua->readVariable<int>("val_after");

				CHECK(val_before == 7);
				CHECK(val_after == 7);
			}
		}

		WHEN("we push in C++ and pop in Lua")
		{
			queue.push(CustomType(5));

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
	}
}


SCENARIO("Unsafe pointers")
{
	GIVEN("a temporary object instance and a queue that can safely copy the instance when pushed")
	{
		// Queue accepting both custom type, and a safely wrapped pointer to custom type.
		using ExQueue = Queue<
			CustomType, CopyPtr<CustomType>*
		>;
		// Initialise queue and get lua context.
		ExQueue queue(L, "lqueue");
		ExQueue::Lua lua = queue.lua();
		// Bind CustomType to Lua.
		lua->registerMember("val", &CustomType::val);

		CustomType* temporary = nullptr;

		WHEN("we place the instance in the Lua state and patch the metatable")
		{
			// Create temporary instance and place in Lua state.
			CustomType* temporary = new CustomType(7);
			lua->writeVariable("ltemporary", temporary);
			//
			lua->writeVariable(
				"ltemporary", LuaContext::Metatable, "_typeid", &typeid(CopyPtr<CustomType>*)
			);

			AND_WHEN("we push the Lua instance to the queue and destroy it")
			{
				lua->executeCode("lqueue:push(ltemporary)");
				delete temporary;
				temporary = nullptr;

				AND_WHEN("we pop from the queue in C++")
				{
					ExQueue::Msg msg = *queue.pop();
					const CustomType& custom = msg.as<CustomType>();

					THEN("the value is correct")
					{
						CHECK(custom.val == 7);
					}

					THEN("the pointer has changed")
					{
						CHECK((void*)temporary != (void*)&custom);
					}
				}
			}

			AND_WHEN(
				"we push a table containing the instance to the queue, then destroy the instance"
			) {
				lua->executeCode("lqueue:push({temp=ltemporary})");
				delete temporary;
				temporary = nullptr;

				AND_WHEN("we pop from the queue in C++")
				{
					ExQueue::Msg msg = *queue.pop();
					const CustomType& copied = msg.get("temp").as<CustomType>();

					THEN("the value is correct")
					{
						CHECK(copied.val == 7);
					}

					THEN("the pointer has changed")
					{
						CHECK((void*)temporary != (void*)&copied);
					}
				}
			}


			AND_WHEN("we push the wrapped C++ instance to the queue and destroy it")
			{
				queue.push((CopyPtr<CustomType>*)temporary);
				delete temporary;
				temporary = nullptr;

				AND_WHEN("we pop from the queue in C++")
				{
					ExQueue::Msg msg = *queue.pop();
					const CustomType& custom = msg.as<CustomType>();

					THEN("the value is correct")
					{
						CHECK(custom.val == 7);
					}

					THEN("the pointer has changed")
					{
						CHECK((void*)temporary != (void*)&custom);
					}
				}
			}

			AND_WHEN(
				"we push a map containing the wrapped C++ instance to the queue, then destroy the "
				"instance"
			) {
				queue.push(ExQueue::Map{
					{ "temp", (CopyPtr<CustomType>*)temporary }
				});
				delete temporary;
				temporary = nullptr;

				AND_WHEN("we pop from the queue in C++")
				{
					ExQueue::Msg msg = *queue.pop();
					const CustomType& copied = msg.get("temp").as<CustomType>();

					THEN("the value is correct")
					{
						CHECK(copied.val == 7);
					}

					THEN("the pointer has changed")
					{
						CHECK((void*)temporary != (void*)&copied);
					}
				}
			}
		}

		delete temporary;
	}
}
