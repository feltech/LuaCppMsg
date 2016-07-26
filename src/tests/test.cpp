#include "catch.hpp"

#include <thread>
#include <LuaCppMsg.hpp>

using namespace LuaCppMsg;

extern lua_State* L;

SCENARIO("Push and pop from C++")
{
	GIVEN("A queue")
	{
		using SimpleQueue = Queue<>;

		SimpleQueue queue;

		THEN("the queue length is initially 0")
		{
			CHECK(queue.size() == 0);
		}

		WHEN("we try to pop an empty queue")
		{
			SimpleQueue::Opt msg = queue.pop();

			THEN("the SimpleQueue evaluates as falsey")
			{
				CHECK(!msg);
				CHECK(!msg.is_initialized());
			}
		}

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

		WHEN("we push a string to the queue")
		{
			queue.push(SimpleQueue::Msg("MOCK SimpleQueue"));

			AND_WHEN("we pop the string from the queue")
			{
				SimpleQueue::Msg msg = *queue.pop();

				THEN("the string is correct")
				{
					CHECK(msg.as<SimpleQueue::Str>() == "MOCK SimpleQueue");
				}
			}
		}

		WHEN("we push a map to the queue")
		{
			SimpleQueue::Map msg_map{
				{ "type", SimpleQueue::Str("MOCK SimpleQueue") },
				{ "nested", SimpleQueue::Map{{ "a bool", true }} },
				{ 7, 3.1 }
			};
			queue.push(SimpleQueue::Msg(msg_map));

			THEN("the queue size is 1")
			{
				CHECK(queue.size() == 1);
			}

			AND_WHEN("we pop the SimpleQueue from the queue")
			{
				SimpleQueue::Msg msg = *queue.pop();

				THEN("the queue size is 0")
				{
					CHECK(queue.size() == 0);
				}

				THEN("the SimpleQueue has the correct type")
				{
					CHECK(msg.get(SimpleQueue::Str("type")).as<SimpleQueue::Str>() == "MOCK SimpleQueue");
				}

				THEN("we can get nested attributes")
				{
					const bool val1 = msg
						.get(SimpleQueue::Str("nested"))
						.get(SimpleQueue::Str("a bool"))
						.as<SimpleQueue::Bool>();

					const SimpleQueue::Num val2 = msg
						.get(7)
						.as<SimpleQueue::Num>();

					CHECK(val1 == true);
					CHECK(val2 == 3.1);
				}
			}
		}
	}
}


SCENARIO("Push and pop from Lua")
{
	GIVEN("A queue bound to lua")
	{
		using SimpleQueue = Queue<>;

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

		WHEN("we try to pop a SimpleQueue from empty queue")
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
				"  type=\"MOCK SimpleQueue\", nested={[\"a bool\"]=true}, [7]=3.1"
				"})"
			);

			AND_WHEN("we pop the table from the queue")
			{
				lua->executeCode("item = lqueue:pop()");

				THEN("the table is correct")
				{
					SimpleQueue::Map item = lua->readVariable<SimpleQueue::Map>("item");
					SimpleQueue::Msg msg(item);
					CHECK(msg.get("type").as<SimpleQueue::Str>() == "MOCK SimpleQueue");
					CHECK(msg.get("nested").get("a bool").as<SimpleQueue::Bool>() == true);
					CHECK(msg.get(7).as<SimpleQueue::Num>() == 3.1);
				}
			}
		}
	}
}


SCENARIO("push and pop between C++ and Lua")
{
	GIVEN("A queue bound to lua")
	{
		using SimpleQueue = Queue<>;
		SimpleQueue queue(L, "lqueue");
		SimpleQueue::Lua lua = queue.lua();

		WHEN("we push a boolean to the queue from Lua")
		{
			lua->executeCode("lqueue:push(true)");

			AND_WHEN("we pop the boolean from the queue from C++")
			{
				SimpleQueue::Msg msg = *queue.pop();

				THEN("the boolean is correct")
				{
					CHECK(msg.as<SimpleQueue::Bool>() == true);
				}
			}
		}

		WHEN("we push a number to the queue from Lua")
		{
			lua->executeCode("lqueue:push(5.4)");

			AND_WHEN("we pop the number from the queue from C++")
			{
				SimpleQueue::Msg msg = *queue.pop();

				THEN("the number is correct")
				{
					CHECK(msg.as<SimpleQueue::Num>() == 5.4);
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

		WHEN("we push a boolean to the queue from C++")
		{
			queue.push(SimpleQueue::Msg(true));

			AND_WHEN("we pop the boolean from the queue from Lua")
			{
				lua->executeCode("item = lqueue:pop()");

				THEN("the boolean is correct")
				{
					SimpleQueue::Bool item = lua->readVariable<SimpleQueue::Bool>("item");
					CHECK(item == true);
				}
			}
		}

		WHEN("we push a number to the queue from C++")
		{
			queue.push(SimpleQueue::Msg(5.4));

			AND_WHEN("we pop the number from the queue from Lua")
			{
				lua->executeCode("item = lqueue:pop()");

				THEN("the number is correct")
				{
					SimpleQueue::Num item = lua->readVariable<SimpleQueue::Num>("item");
					CHECK(item == 5.4);
				}
			}
		}

		WHEN("we push a string to the queue from C++")
		{
			queue.push(SimpleQueue::Msg("a string"));

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
				{ "type", SimpleQueue::Str("MOCK SimpleQueue") },
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

					CHECK(type == "MOCK SimpleQueue");
					CHECK(nested_bool == true);
					CHECK(indexed_num == 3.1);
				}
			}
		}

		WHEN("we push an array to the queue from Lua")
		{
			lua->executeCode(
				"lqueue:push({"
				"  5, true, \"a string\""
				"})"
			);

			AND_WHEN("we pop the array from the queue from C++")
			{
				SimpleQueue::Msg msg = *queue.pop();

				THEN("the map is correct")
				{
					CHECK(msg.get(1).as<SimpleQueue::Num>() == 5);
					CHECK(msg.get(2).as<SimpleQueue::Bool>() == true);
					CHECK(msg.get(3).as<SimpleQueue::Str>() == "a string");
				}
			}
		}

		WHEN("we push a table to the queue from Lua")
		{
			lua->executeCode(
				"lqueue:push({"
				"  type=\"MOCK SimpleQueue\", nested={[\"a bool\"]=true}, [7]=3.1"
				"})"
			);

			AND_WHEN("we pop the table from the queue from C++")
			{
				SimpleQueue::Msg msg = *queue.pop();

				THEN("the map is correct")
				{
					CHECK(msg.get("type").as<SimpleQueue::Str>() == "MOCK SimpleQueue");
					CHECK(msg.get("nested").get("a bool").as<SimpleQueue::Bool>() == true);
					CHECK(msg.get(7).as<SimpleQueue::Num>() == 3.1);
				}
			}
		}
	}
}


SCENARIO("Multithreaded push/pop")
{
	GIVEN("C++ producers and consumers, with consumers consuming less than produced")
	{
		using SimpleQueue = Queue<>;
		SimpleQueue queue(L, "lqueue");
		SimpleQueue::Lua lua = queue.lua();

		// Push a map with a "value" attribute that is just the queue size.
		auto producer = [&queue]() {
			for (unsigned i = 0; i < 100; i++)
			{
				queue.push(SimpleQueue::Msg(SimpleQueue::Map{
					{"type", SimpleQueue::Str("FROM C++")},
					{"value", SimpleQueue::Num(queue.size())}
				}));
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
					SimpleQueue::Num num = msg.get("value").as<SimpleQueue::Num>();

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
//
struct MockType
{
	MockType(int val_) : val(val_) {}
	int val;
};

SCENARIO("Custom types")
{
	GIVEN("a queue with messages allowing a custom type")
	{
		using ExQueue = Queue<MockType>;
		ExQueue queue(L, "lqueue");

		WHEN("we push and pop a custom type")
		{
			queue.push(ExQueue::Msg(MockType(3)));
			ExQueue::Msg msg = *queue.pop();

			THEN("the value is correct")
			{
				CHECK(msg.as<MockType>().val == 3);
			}
		}
	}
}
