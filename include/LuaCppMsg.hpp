#ifndef INCLUDE_LUACPPMSG_HPP_
#define INCLUDE_LUACPPMSG_HPP_

#include <queue>
#include <string>
#include <unordered_map>
#include <mutex>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <boost/functional/hash.hpp>
#include <LuaContext.hpp>


namespace LuaCppMsg
{

/**
 * Wrapper class around the messages stored within the queue.
 *
 * Namespaces the message items and provides utility helper methods for extracting values.
 */
class Message
{
public:
	/// The type returned when the queue is popped in C++ - allows for `nil` to be represented.
	using Opt = boost::optional<Message>;
	/// Boolean type (value).
	using Bool = bool;
	/// Numeric type (value).
	using Num = double;
	/// Integer type (key).
	using Int = int;
	/// String type (key + value).
	using Str = std::string;
	/// Key type for maps/tables.
	using Key = boost::variant<Int, Str>;

	/// Variant type, which can be a message on its own, or combined in (recursive) unordered_maps.
	using Item = boost::make_recursive_variant
	<
		Bool,
		Num,
		Str,
		std::unordered_map<
			Key,
			boost::recursive_variant_,
			boost::hash<Key>
		>
	>::type;

	/// An unordered_map containing `Item`s (including other `Map`s).
	using Map = std::unordered_map<Key, Item, boost::hash<Key>>;

	/**
	 * Transient utility class providing accessors to nested data within recursive unordered_maps
	 * of variants.
	 */
	class Nested
	{
	public:
		/**
		 * Create `Nested` helper pointing to a given Item in an `unordered_map` hierarchy.
		 *
		 * @param pitem_ pointer to Item in the tree
		 */
		Nested(const Item* pitem_) : m_pitem(pitem_) {};

		/**
		 * Extract the variant at this branch of the `unordered_map` as a concrete value.
		 *
		 * @return extracted value.
		 */
		template <class T>
		T as() const
		{
			return boost::get<T>(*m_pitem);
		}

		/**
		 * Navigate to a value in the current branch of the `unordered_map`.
		 *
		 * @param key_ variant key into `unordered_map`.
		 * @return a new Nested representing the Item referenced at `key_`.
		 */
		Nested get(const Key key_) const
		{
			const Map& map = boost::get<Map>(*m_pitem);
			const Item& item = map.at(key_);
			return Nested(&item);
		}

		/**
		 * Navigate to a value in the current branch of the `unordered_map`.
		 *
		 * @param key_ string key into `unordered_map`.
		 * @return a new Nested representing the Item referenced at `key_`.
		 */
		Nested get(const char* key_) const
		{
			return get(Str(key_));
		}

		/**
		 * Get reference to the Item represented by this object.
		 *
		 * @return item on this branch of the `unordered_map`.
		 */
		const Item& item() const
		{
			return *m_pitem;
		}

	private:
		/// Pointer to `Item` on branch of `unordered_map` represented by this class.
		const Item* m_pitem;
	};

	/**
	 * Convenience overload for passing character array as message.
	 *
	 * @param item_ string of message.
	 */
	Message(const char* item_) : m_item(Item(Message::Str(item_))) {}

	/**
	 * Construct a message from given Item, either for creation or when popped from the queue.
	 *
	 * @param item_ message item.
	 */
	Message(Item&& item_) : m_item(item_) {}

	/**
	 * Get reference to the Item at the root of this message.
	 *
	 * @return item at root of message.
	 */
	const Item& item() const
	{
		return m_item;
	}

	/**
	 * Extract the variant at the root of message as a concrete value.
	 *
	 * @return extracted value.
	 */
	template <class T>
	T as() const
	{
		return boost::get<T>(m_item);
	}

	/**
	 * Navigate to a value in the root `unordered_map`.
	 *
	 * @param key_ variant key into `unordered_map`.
	 * @return a new Nested representing the Item referenced at `key_`.
	 */
	Nested get(const Key key_) const
	{
		const Map& map = boost::get<Map>(m_item);
		const Item& item = map.at(key_);
		return Nested(&item);
	}

	/**
	 * Navigate to a value in the root `unordered_map`.
	 *
	 * @param key_ string key into `unordered_map`.
	 * @return a new Nested representing the Item referenced at `key_`.
	 */
	Nested get(const char* key_) const
	{
		return get(Str(key_));
	}

private:
	/// Item at root of this message.
	Item m_item;
};


/**
 * Thread-safe C++/Lua queue of `Message`s.
 */
class Queue
{
public:
	/// Smart pointer to "luawrapper" `LuaContext`.
	using Lua = std::shared_ptr<LuaContext>;

	/**
	 * Basic construction.
	 *
	 * To use with lua, Queue::bind and Queue::to_lua will have to be called separately.
	 */
	Queue() = default;

	/**
	 * Construct and bind to given Lua state.
	 *
	 * Queue will not be exposed to Lua yet though
	 *
	 * @param plua Lua state to bind to.
	 */
	Queue (lua_State* plua)
	{
		bind(plua);
	}

	/**
	 * Construct and bind and expose to Lua.
	 *
	 * @param plua Lua state to bind to.
	 * @param lua_name name of variable in global Lua namespace.
	 */
	Queue (lua_State* plua, const std::string& lua_name)
	{
		bind(plua);
		to_lua(lua_name);
	}

	/**
	 * Trivial destructor.
	 */
	~Queue () {}

	/**
	 * Thread-safely get size of queue.
	 */
	unsigned size ()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return size_unsafe();
	}

	/**
	 * Thread-safely push a Message in C++.
	 *
	 * @param msg_ message to append to queue.
	 */
	void push (const Message& msg_)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queue.push(msg_.item());
	}

	/**
	 * Thread-safely push a Message in C++.
	 *
	 * @param msg_ message to append to queue.
	 */
	void push (Message&& msg_)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queue.push(std::move(msg_.item()));
	}

	/**
	 * Thread-safely pop a Message in C++.
	 *
	 * @return an `optional` that either contains a Message, or is falsey if the queue is empty.
	 */
	Message::Opt pop ()
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		if (!size_unsafe())
			return boost::none;

		Message msg(std::move(m_queue.front()));
		m_queue.pop();
		return msg;
	}

	/**
	 * Expose this queue to Lua.
	 *
	 * @param name_ variable name in global Lua namespace.
	 */
	void to_lua (const std::string& name_)
	{
		m_lua->writeVariable(name_, this);
	}

	/**
	 * Thread-safely push a message in Lua.
	 *
	 * @param msg_ message to push - will be intelligently converted from basic type or table.
	 */
	void push_lua (Message::Item msg_)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queue.push(msg_);
	}

	/**
	 * Thread-safely pop a message in Lua.
	 *
	 * @return basic type or table, depending on the message.
	 */
	boost::optional<Message::Item> pop_lua ()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (!size_unsafe())
			return boost::none;
		Message::Item msg(std::move(m_queue.front()));
		m_queue.pop();
		return msg;
	}

	/**
	 * Bind this queue to Lua.
	 *
	 * Does not expose the queue to Lua, just binds the methods.
	 *
	 * Will not bind methods if they've already been bound to the given Lua state.
	 *
	 * @param L Lua state to bind to.
	 */
	void bind (lua_State* L)
	{
		m_lua = Lua(new LuaContext(L));
		if (!bound_states().count(L))
		{
			m_lua->registerFunction("size", &Queue::size);
			m_lua->registerFunction("push", &Queue::push_lua);
			m_lua->registerFunction("pop", &Queue::pop_lua);
			bound_states().insert(L);
		}
	}

	/**
	 * Getter for internal `LuaContext` object.
	 *
	 * @return `shared_ptr` to `LuaContext`.
	 */
	Lua lua()
	{
		return m_lua;
	}

	/**
	 * Storage of already-bound Lua states, so we don't keep re-binding.
	 *
	 * @return set of state pointers that have already been bound.
	 */
	static std::set<lua_State*>& bound_states()
	{
		static std::set<lua_State*> bound_states;
		return bound_states;
	}

private:
	/// Smart pointer to `LuaContext` object used for binding and exposing.
	Lua m_lua;
	/// Actual internal queue of messages.
	std::queue<Message::Item> m_queue;
	/// Mutex used for locking push/pop/size calls.
	std::mutex m_mutex;

	/**
	 * Get size of queue without thread-safety.
	 *
	 * @return size of queue
	 */
	unsigned size_unsafe ()
	{
		return m_queue.size();
	}
};

} /* namespace LuaCppMsg */

#endif /* INCLUDE_LUACPPMSG_HPP_ */

