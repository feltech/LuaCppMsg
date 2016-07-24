#include <Queue.hpp>

namespace LuaCppMsg
{

Queue::Queue ()
{
}

Queue::~Queue ()
{
}

unsigned Queue::size ()
{
	return m_queue.size();
}

void Queue::push (const Message& msg_)
{
	m_queue.push(msg_.item());
}

void Queue::push (Message&& msg_)
{
	m_queue.push(std::move(msg_.item()));
}

Message Queue::pop ()
{
	Message msg(std::move(m_queue.front()));
	m_queue.pop();
	return msg;
}

std::unique_ptr<LuaContext> Queue::lua;

void Queue::to_lua (const std::string& name_)
{
	Queue::lua->writeVariable(name_, std::shared_ptr<Queue>(this));
}

void Queue::push_lua (Message::Item msg_)
{
	m_queue.push(msg_);
}

Message::Item Queue::pop_lua ()
{
	Message::Item msg(std::move(m_queue.front()));
	m_queue.pop();
	return msg;
}

void Queue::bind ()
{
	if (Queue::lua.get() != nullptr)
		return;
	Queue::bind(new LuaContext());
}

void Queue::bind (lua_State* L)
{
	if (Queue::lua.get() != nullptr)
		return;
	Queue::bind(new LuaContext(L));
}

void Queue::bind (LuaContext* L)
{
	if (Queue::lua.get() != nullptr)
		return;
	Queue::lua = std::unique_ptr<LuaContext>(L);
	Queue::lua->registerFunction("size", &Queue::size);
	Queue::lua->registerFunction("push", &Queue::push_lua);
	Queue::lua->registerFunction("pop", &Queue::pop_lua);
}

} /* namespace LuaCppMsg */
