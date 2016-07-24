#ifndef INCLUDE_QUEUE_HPP_
#define INCLUDE_QUEUE_HPP_

#include <queue>
#include <string>
#include <unordered_map>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <boost/functional/hash.hpp>
#include <LuaContext.hpp>

namespace LuaCppMsg
{

class Message
{
public:
	using Bool = bool;
	using Int = int;
	using Num = double;
	using Str = std::string;
	using Key = boost::variant<Int, Str>;

	using Item = boost::make_recursive_variant
	<
		Bool,
		Num,
		Int,
		Str,
		std::unordered_map<
			Key,
			boost::recursive_variant_,
			boost::hash<Key>
		>
	>::type;

	using Map = std::unordered_map<Key, Item, boost::hash<Key>>;

	class Nested
	{
	public:
		Nested(const Item* pitem_) : m_pitem(pitem_) {};

		template <class T>
		T as() const
		{
			return boost::get<T>(*m_pitem);
		}

		Nested get(const Key key_) const
		{
			const Map& map = boost::get<Map>(*m_pitem);
			const Item& item = map.at(key_);
			return Nested(&item);
		}

		Nested get(const char* key_) const
		{
			return get(Str(key_));
		}

		const Item& item() const { return *m_pitem; }

	private:
		const Item* m_pitem;
	};


	Message(const char* item_) : m_item(Item(Message::Str(item_))) {}
	Message(Item&& item_) : m_item(item_) {}

	const Item& item() const { return m_item; }

	template <class T>
	const T& as()
	{
		return boost::get<T>(m_item);
	}

	const Nested get(const Key key_)
	{
		const Map& map = boost::get<Map>(m_item);
		const Item& item = map.at(key_);
		return Nested(&item);
	}

	const Nested get(const char* key_)
	{
		return get(Str(key_));
	}

private:
	Item m_item;
};


class Queue
{
public:
	Queue ();
	~Queue ();

	void push (const Message& msg_);
	void push (Message&& msg_);
	Message pop ();

	void to_lua(const std::string& name_);
	void push_lua (Message::Item msg_);
	Message::Item pop_lua ();

	unsigned size ();

	static void bind();
	static void bind(lua_State* L);
	static void bind(LuaContext* L);
	static std::unique_ptr<LuaContext> lua;
private:
	std::queue<Message::Item> m_queue;
};

} /* namespace LuaCppMsg */
#endif /* INCLUDE_QUEUE_HPP_ */

