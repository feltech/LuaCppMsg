#include "catch.hpp"

#include <thread>
#include <LuaCppMsg.hpp>

using namespace LuaCppMsg;

extern lua_State* L;

SCENARIO("Push and pop from C++")
{
	GIVEN("A queue")
	{
		Queue queue;

		THEN("the queue length is initially 0")
		{
			CHECK(queue.size() == 0);
		}

		WHEN("we try to pop an empty queue")
		{
			Message::Opt msg = queue.pop();

			THEN("the message evaluates as falsey")
			{
				CHECK(!msg);
				CHECK(!msg.is_initialized());
			}
		}

		WHEN("we push a number to the queue")
		{
			queue.push(Message(5.4));

			THEN("the queue size is 1")
			{
				CHECK(queue.size() == 1);
			}

			AND_WHEN("we pop the number from the queue")
			{
				Message msg = *queue.pop();

				THEN("the queue size is 0")
				{
					CHECK(queue.size() == 0);
				}

				THEN("the number is correct")
				{
					CHECK(msg.as<Message::Num>() == 5.4);
				}
			}
		}

		WHEN("we push a string to the queue")
		{
			queue.push(Message("MOCK MESSAGE"));

			AND_WHEN("we pop the string from the queue")
			{
				Message msg = *queue.pop();

				THEN("the string is correct")
				{
					CHECK(msg.as<Message::Str>() == "MOCK MESSAGE");
				}
			}
		}

		WHEN("we push a map to the queue")
		{
			Message::Map msg_map{
				{
					Message::Str("type"), Message::Str("MOCK MESSAGE")
				},
				{
					Message::Str("nested"), Message::Map{
						{ Message::Str("a bool"), true }
					},
				},
				{
					7, 3.1
				}
			};
			queue.push(Message(msg_map));

			THEN("the queue size is 1")
			{
				CHECK(queue.size() == 1);
			}

			AND_WHEN("we pop the message from the queue")
			{
				Message msg = *queue.pop();

				THEN("the queue size is 0")
				{
					CHECK(queue.size() == 0);
				}

				THEN("the message has the correct type")
				{
					CHECK(msg.get(Message::Str("type")).as<Message::Str>() == "MOCK MESSAGE");
				}

				THEN("we can get nested attributes")
				{
					const bool val1 = msg
						.get(Message::Str("nested"))
						.get(Message::Str("a bool"))
						.as<Message::Bool>();

					const Message::Num val2 = msg
						.get(7)
						.as<Message::Num>();

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
		Queue queue(L, "lqueue");
		Queue::Lua lua = queue.lua();

		THEN("the C++ queue and Lua queue are the same object")
		{
			Queue* pqueue = *lua->readVariable<boost::optional<Queue*>>("lqueue");
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

			THEN("the message is nil")
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
					Message::Str item = lua->readVariable<Message::Str>("item");
					CHECK(item == "a string");
				}
			}
		}

		WHEN("we push a table to the queue")
		{
			lua->executeCode(
				"lqueue:push({"
				"  type=\"MOCK MESSAGE\", nested={[\"a bool\"]=true}, [7]=3.1"
				"})"
			);

			AND_WHEN("we pop the table from the queue")
			{
				lua->executeCode("item = lqueue:pop()");

				THEN("the table is correct")
				{
					Message::Map item = lua->readVariable<Message::Map>("item");
					Message msg(item);
					CHECK(msg.get("type").as<Message::Str>() == "MOCK MESSAGE");
					CHECK(msg.get("nested").get("a bool").as<Message::Bool>() == true);
					CHECK(msg.get(7).as<Message::Num>() == 3.1);
				}
			}
		}
	}
}


SCENARIO("push and pop between C++ and Lua")
{
	GIVEN("A queue bound to lua")
	{
		Queue queue(L, "lqueue");
		Queue::Lua lua = queue.lua();

		WHEN("we push a boolean to the queue from Lua")
		{
			lua->executeCode("lqueue:push(true)");

			AND_WHEN("we pop the boolean from the queue from C++")
			{
				Message msg = *queue.pop();

				THEN("the boolean is correct")
				{
					CHECK(msg.as<Message::Bool>() == true);
				}
			}
		}

		WHEN("we push a number to the queue from Lua")
		{
			lua->executeCode("lqueue:push(5.4)");

			AND_WHEN("we pop the number from the queue from C++")
			{
				Message msg = *queue.pop();

				THEN("the number is correct")
				{
					CHECK(msg.as<Message::Num>() == 5.4);
				}
			}
		}

		WHEN("we push a string to the queue from Lua")
		{
			lua->executeCode("lqueue:push(\"a string\")");

			AND_WHEN("we pop the number from the queue from C++")
			{
				Message msg = *queue.pop();

				THEN("the string is correct")
				{
					CHECK(msg.as<Message::Str>() == "a string");
				}
			}
		}

		WHEN("we push a boolean to the queue from C++")
		{
			queue.push(Message(true));

			AND_WHEN("we pop the boolean from the queue from Lua")
			{
				lua->executeCode("item = lqueue:pop()");

				THEN("the boolean is correct")
				{
					Message::Bool item = lua->readVariable<Message::Bool>("item");
					CHECK(item == true);
				}
			}
		}

		WHEN("we push a number to the queue from C++")
		{
			queue.push(Message(5.4));

			AND_WHEN("we pop the number from the queue from Lua")
			{
				lua->executeCode("item = lqueue:pop()");

				THEN("the number is correct")
				{
					Message::Num item = lua->readVariable<Message::Num>("item");
					CHECK(item == 5.4);
				}
			}
		}

		WHEN("we push a string to the queue from C++")
		{
			queue.push(Message("a string"));

			AND_WHEN("we pop the number from the queue from Lua")
			{
				lua->executeCode("item = lqueue:pop()");

				THEN("the string is correct")
				{
					Message::Str item = lua->readVariable<Message::Str>("item");
					CHECK(item == "a string");
				}
			}
		}

		WHEN("we push a map to the queue from C++")
		{
			Message::Map msg_map{
				{
					Message::Str("type"), Message::Str("MOCK MESSAGE")
				},
				{
					Message::Str("nested"), Message::Map{
						{ Message::Str("a bool"), true }
					},
				},
				{
					7, 3.1
				}
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
	}
}


SCENARIO("Multithreaded push/pop")
{
	GIVEN("C++ producers and consumers, with consumers consuming less than produced")
	{
		Queue queue(L, "lqueue");
		Queue::Lua lua = queue.lua();

		// Push a map with a "value" attribute that is just the queue size.
		auto producer = [&queue]() {
			for (unsigned i = 0; i < 100; i++)
			{
				queue.push(Message(Message::Map{
					{Message::Str("type"), Message::Str("FROM C++")},
					{Message::Str("value"), Message::Num(queue.size())}
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
					Message msg = *queue.pop();
					Message::Num num = msg.get("value").as<Message::Num>();

					unsigned queue_size = queue.size();
					unsigned num_luas = 0;
					unsigned num_cs = 0;

					while (Message::Opt msg_exists = queue.pop())
					{
						const Message& msg = *msg_exists;
						const Message::Str& type = msg.get("type").as<const Message::Str>();
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
