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
#include <iostream>

namespace LuaCppMsg
{

/**
 * Wrapper class around the messages stored within the queue.
 *
 * Provides utility helper methods for extracting values.
 *
 * @tparam CustomTypes list of additional types in the variant map/table. Must already be bound
 * to Lua.
 */
template <class... CustomTypes>
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
	/// Variant type, which can be a message on its own, or combined in (recursive) `Map`s.
	using Item = typename boost::make_recursive_variant
	<
		Bool,
		Num,
		Str,
		CustomTypes...,
		std::unordered_map<
			Key,
			boost::recursive_variant_,
			boost::hash<Key>
		>
	>::type;
	/// A map containing `Item`s (including other `Map`s).
	using Map = std::unordered_map<Key, Item, boost::hash<Key>>;

	/**
	 * Transient utility class providing accessors to nested data within recursive unordered_maps
	 * of variants.
	 */
	class Nested
	{
	public:
		/**
		 * Create `Nested` helper pointing to a given Item in an Map hierarchy.
		 *
		 * @param pitem_ pointer to Item in the tree
		 */
		Nested(const Item* pitem_) : m_pitem(pitem_) {};

		/**
		 * Extract the variant at this branch of the Map as a concrete value.
		 *
		 * @return extracted value.
		 */
		template <class T>
		T as() const
		{
			return boost::get<T>(*m_pitem);
		}

		/**
		 * Navigate to a value in the current branch of the Map.
		 *
		 * @param key_ variant key into Map.
		 * @return a new Nested representing the Item referenced at `key_`.
		 */
		Nested get(const Key key_) const
		{
			const Map& map = boost::get<Map>(*m_pitem);
			const Item& item = map.at(key_);
			return Nested(&item);
		}

		/**
		 * Navigate to a value in the current branch of the Map.
		 *
		 * @param key_ string key into Map.
		 * @return a new Nested representing the Item referenced at `key_`.
		 */
		Nested get(const char* key_) const
		{
			return get(Str(key_));
		}

		/**
		 * Get reference to the Item represented by this object.
		 *
		 * @return item on this branch of the Map.
		 */
		const Item& item() const
		{
			return *m_pitem;
		}

	private:
		/// Pointer to Item on branch of Map represented by this class.
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
	 * Construct a message from given Item, either for creation or when popped from the queue.
	 *
	 * @param item_ message item.
	 */
	Message(const Item& item_) : m_item(item_) {}

	/**
	 * Get reference to the Item at the root of this message.
	 *
	 * @return item at root of message.
	 */
	Item& item()
	{
		return m_item;
	}

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
	 * Navigate to a value in the root Map.
	 *
	 * @param key_ variant key into Map.
	 * @return a new Nested representing the Item referenced at `key_`.
	 */
	Nested get(const Key key_) const
	{
		const Map& map = boost::get<Map>(m_item);
		const Item& item = map.at(key_);
		return Nested(&item);
	}

	/**
	 * Navigate to a value in the root Map.
	 *
	 * @param key_ string key into Map.
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
 * Wrapper for pointer types that are unsafe.
 *
 * Bound C++ types may be passed from Lua as pointers by some binding libraries (e.g. tolua++).
 * If the object pointed to is temporary, then it may be garbage collected before the queue is
 * popped.
 *
 * To use this wrapper you must override the Lua metatable/prototype's `_typeid` attribute to be
 * a `CopyPtr<type>`.  Then the queue will ensure the underlying data is copied when it is pushed.
 *
 * The object can then be popped in C++ using `as<T>()`.
 *
 * E.g.
 * `lua->writeVariable("anObj", LuaContext::Metatable, "_typeid", &typeid(CopyPtr<CustomType>));`
 * or
 * `lua->writeVariable("AClass", "_typeid", &typeid(CopyPtr<CustomType>));`
 * where `anObj` is a Lua object you want to pass, and `AClass` is a Lua prototype that new objects
 * are constructed from.
 *
 * @tparam T the type to wrap
 */
template <class T>
class CopyPtr
{
public:
	/// Do not allow construction.
	CopyPtr() = delete;
	/// Do not allow copy construction.
	CopyPtr(const CopyPtr<T>& other_) = delete;
	/// Do not allow move construction.
	CopyPtr(CopyPtr<T>&& other_) = delete;
};


/**
 * Thread-safe C++/Lua queue of `Message`s.
 */
template <class... CustomTypes>
class Queue
{
public:
	using QueueType = Queue<CustomTypes...>;
	using Msg = typename  LuaCppMsg::Message<CustomTypes...>;
	using Opt = typename Msg::Opt;
	/// Boolean type (value).
	using Bool = typename Msg::Bool;
	/// Numeric type (value).
	using Num = typename Msg::Num;
	/// Integer type (key).
	using Int = typename Msg::Int;
	/// String type (key + value).
	using Str = typename Msg::Str;
	/// Key type for maps/tables.
	using Key = typename Msg::Key;
	/// Item variant representing any of the supported types.
	using Item = typename Msg::Item;
	/// An map containing `Item`s (including other `Map`s).
	using Map = typename Msg::Map;
	/// Internal queue type for storage of `Item`s.
	using InternalQueue = std::queue<Item>;

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
	 * Thread-safely push a string in C++.
	 *
	 * @param msg_ message string to append to queue.
	 */
	void push (const char* msg_)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queue.push(Item(Str(msg_)));
	}

	/**
	 * Thread-safely push a an Item in C++.
	 *
	 * @param msg_ message to append to queue.
	 */
	void push (const Item& msg_)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		Item item = boost::apply_visitor(CopyVisitor(), msg_);
		m_queue.push(std::move(item));
	}

	/**
	 * Thread-safely push a Item in C++.
	 *
	 * @param msg_ message to append to queue.
	 */
	void push (Item&& msg_)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		Item item = boost::apply_visitor(CopyVisitor(), msg_);
		m_queue.push(std::move(item));
	}

	/**
	 * Thread-safely pop a Message in C++.
	 *
	 * @return an `optional` that either contains a Message, or is falsey if the queue is empty.
	 */
	Opt pop ()
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		if (!size_unsafe())
			return boost::none;

		Msg msg(std::move(m_queue.front()));
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
	void push_lua (Item msg_)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		Item item = boost::apply_visitor(CopyVisitor(), msg_);
		m_queue.push(item);
	}

	/**
	 * Thread-safely pop a message in Lua.
	 *
	 * @return basic type or table, depending on the message.
	 */
	boost::optional<Item> pop_lua ()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (!size_unsafe())
			return boost::none;
		Item msg(std::move(m_queue.front()));
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
	std::queue<Item> m_queue;
	/// Mutex used for locking push/pop/size calls.
	std::mutex m_mutex;

	/**
	 * A visitor over the Item variant to duplicate `CopyPtr<T>`s as `T`s.
	 *
	 * The CustomTypes of the Item must have both `CopyPtr<T>*` and `T` as allowed value types.
	 */
	class CopyVisitor : public boost::static_visitor<Item>
	{
	public:
		template <class T>

		/**
		 * Overload taking a pointer to CopyPtr<T> and replacing it with a T.
		 *
		 * @param to_copy item pointed to for copying.
		 * @return variant Item containing a T
		 */
		Item operator()(CopyPtr<T>* to_copy) const
	    {
			return T(*((T*)to_copy));
	    }

		/**
		 * Recursively apply this CopyVisitor to values in Map.
		 * @param to_copy Map to loop over
		 * @return the updated Map.
		 */
		Item operator()(Map& to_copy) const
	    {
			for (auto& item : to_copy)
				item.second = std::move(boost::apply_visitor(CopyVisitor(), item.second));

			return std::move(to_copy);
	    }

		/**
		 * Pass-through for all other types (i.e. not Map or CopyPtr).
		 *
		 * @param to_copy general type available to variant Item.
		 * @return an Item containing the same object as passed.
		 */
		template <class T>
		Item operator()(T& to_copy) const
	    {
			return std::move(to_copy);
	    }
	};

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

