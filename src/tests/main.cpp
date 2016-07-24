#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <Queue.hpp>

using namespace LuaCppMsg;

SCENARIO("Push and pop from C++")
{
	GIVEN("A queue")
	{
		Queue queue;

		THEN("the queue length is initially 0")
		{
			CHECK(queue.size() == 0);
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
				Message msg = queue.pop();

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
				Message msg = queue.pop();

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
					7, Message::Map{
						{ 3, 5.8 }
					}
				}
			};
			queue.push(Message(msg_map));

			THEN("the queue size is 1")
			{
				CHECK(queue.size() == 1);
			}

			AND_WHEN("we pop the message from the queue")
			{
				Message msg = queue.pop();

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
						.get(3)
						.as<Message::Num>();

					CHECK(val1 == true);
					CHECK(val2 == 5.8);
				}
			}
		}
	}
}


SCENARIO("Push and pop from Lua")
{
	Queue::bind();
	LuaContext* lua = Queue::lua.get();

	GIVEN("A queue bound to lua")
	{
		Queue queue;
		queue.to_lua("lqueue");

		THEN("the C++ queue and Lua queue are the same object")
		{
			std::shared_ptr<Queue> lua_queue = lua->readVariable<std::shared_ptr<Queue>>("lqueue");
			CHECK(lua_queue.get() == &queue);
		}

		THEN("the queue length is initially 0")
		{
			lua->executeCode("queue_size = lqueue:size()");
			int queue_size = lua->readVariable<int>("queue_size");
		    CHECK(queue_size == 0);
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
