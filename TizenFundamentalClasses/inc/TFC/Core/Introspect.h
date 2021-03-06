/*
 * Tizen Fundamental Classes - TFC
 * Copyright (c) 2016-2017 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *    Core/Introspect.h
 *
 * Basic metaprogram classes to help navigating and differencing
 * between types
 *
 * Created on: 	Oct 31, 2016
 * Author: 		Gilang Mentari Hamidy (g.hamidy@samsung.com)
 */
#pragma once

#ifndef TFC_CORE_INTROSPECT_H_
#define TFC_CORE_INTROSPECT_H_

#include <type_traits>
#include <tuple>
#include <functional>

namespace TFC {
namespace Core {

// Forward declaration
template<typename, typename>
class EventObject;

namespace Introspect {

/*
template<typename TClass, typename TEventInfo>
EventObject<TClass*, TEventInfo> EventOfFunc(EventObject<TClass*, TEventInfo> TClass::* param);
*/
template<typename T>
class HasMemberAccessOperator
{
	typedef char Correct;
	typedef long Incorrect;
	template<typename TTest> static Correct Test(decltype(&TTest::operator->));
	template<typename TTest> static Incorrect Test(...);

public:
	static constexpr bool Value = sizeof(Test<T>(0)) == sizeof(Correct);
};

template<typename T>
class HasCallOperator
{
	typedef char Correct;
	typedef long Incorrect;
	template<typename TTest> static Correct Test(decltype(&TTest::operator()));
	template<typename TTest> static Incorrect Test(...);

public:
	static constexpr bool Value = sizeof(Test<T>(0)) == sizeof(Correct);
};


template<typename TMemPtrType>
struct MemberField
{

};

template<typename TClass, typename TValueType>
struct MemberField<TValueType (TClass::*)>
{
	typedef TClass DeclaringType;
	typedef TValueType ValueType;
};

template<typename TFuncType>
struct MemberFunction
{

};

template<typename TClass, typename TReturn, typename... TArgs>
struct MemberFunction<TReturn (TClass::*)(TArgs...)>
{
	static constexpr auto Arity = sizeof...(TArgs);
	typedef TReturn ReturnType;
	typedef TClass	DeclaringType;

	template<size_t idx>
	using Args = typename std::tuple_element<idx, std::tuple<TArgs...>>::type;

	typedef std::tuple<TArgs...> ArgsTuple;
	typedef std::tuple<typename std::decay<TArgs>::type...> ArgsTupleDecay;
};

template<typename TClass, typename TReturn, typename... TArgs>
struct MemberFunction<TReturn (TClass::*)(TArgs...) const> : MemberFunction<TReturn (TClass::*)(TArgs...)>
{

};

template<typename T>
struct StaticFunction
{

};

template<typename TRet, typename... TArgs>
struct StaticFunction<TRet(TArgs...)>
{
	static constexpr auto Arity = sizeof...(TArgs);
	typedef TRet ReturnType;

	template<size_t idx>
	using Args = typename std::tuple_element<idx, std::tuple<TArgs...>>::type;

	typedef std::tuple<TArgs...> ArgsTuple;
	typedef std::tuple<typename std::decay<TArgs>::type...> ArgsTupleDecay;
};

template<typename T>
struct IsMemberFunction
{
	static constexpr bool Value = false;
};

template<typename TRet, typename TDeclaring, typename... TArgs>
struct IsMemberFunction<TRet (TDeclaring::*)(TArgs...)>
{
	static constexpr bool Value = true;
	typedef MemberFunction<TRet (TDeclaring::*)(TArgs...)> Introspect;
};

template<typename T>
struct IsFunctionPointer
{
	static constexpr bool Value = false;
};

template<typename TRet, typename... TArgs>
struct IsFunctionPointer<TRet(*)(TArgs...)>
{
	static constexpr bool Value = true;
};

template<typename T, bool = HasCallOperator<T>::Value || std::is_function<typename std::remove_pointer<T>::type>::value>
struct CallableObject;


template<typename T>
struct CallableObject<T, false>
{
	static constexpr bool Callable = false; // Fallback
};

template<typename T>
struct CallableObject<T, true> : MemberFunction<decltype(&T::operator())>
{
	static constexpr bool Callable = true;
	typedef typename MemberFunction<decltype(&T::operator())>::ReturnType ReturnType;
};

template<typename TRet, typename... TArgs>
struct CallableObject<TRet(*)(TArgs...), true> : StaticFunction<TRet(TArgs...)>
{
	static constexpr bool Callable = true;
	typedef typename StaticFunction<TRet(TArgs...)>::ReturnType ReturnType;
};

template<int size>
struct StorageSelector
{
	typedef unsigned char Type[size];
};

template<>
struct StorageSelector<1>
{
	typedef unsigned char Type;
};

template<>
struct StorageSelector<2>
{
	typedef unsigned short Type;
};

template<>
struct StorageSelector<4>
{
	typedef unsigned int Type;
};

template<>
struct StorageSelector<8>
{
	typedef unsigned long long Type;
};

template<>
struct StorageSelector<16>
{
	typedef struct { unsigned long long value[2]; } Type;
};

inline bool operator==(StorageSelector<16>::Type const& a, StorageSelector<16>::Type const& b)
{
	return a.value[0] == b.value[0] && a.value[1] == b.value[1];
}

inline bool operator!=(StorageSelector<16>::Type const& a, StorageSelector<16>::Type const& b)
{
	return a.value[0] != b.value[0] || a.value[1] != b.value[1];
}

struct PointerToMemberFunction
{
private:
	class Dummy { };
	static constexpr auto pointerSize = sizeof(void(Dummy::*)(void));

	template<typename TPtrType>
	union PointerToMemberFunctionValue;

	template<typename TDeclaring, typename TRet, typename... TArgs>
	union PointerToMemberFunctionValue<TRet (TDeclaring::*) (TArgs...)>
	{
		TRet (TDeclaring::* pointerValue) (TArgs...);
		StorageSelector<pointerSize>::Type value;
	};
public:

	StorageSelector<pointerSize>::Type value;

	template<typename TPtrType>
	static constexpr PointerToMemberFunction Get(TPtrType ptr)
	{
		PointerToMemberFunctionValue<TPtrType> obj { ptr };
		return { obj.value };
	}

	bool operator==(PointerToMemberFunction const& other)
	{
		return value == other.value;
	}
};

struct PointerToMemberFunctionHash
{
	size_t operator()(PointerToMemberFunction const& obj) const
	{
		auto val = obj.value;
		return std::hash<decltype(val)>()(val);
	}
};

struct PointerToMember
{
private:
	class Dummy { };
	static constexpr auto pointerSize = sizeof(int (Dummy::*));

	template<typename TPtrType>
	union PointerToMemberValue;

	template<typename TDeclaring, typename TValueType>
	union PointerToMemberValue<TValueType (TDeclaring::*)>
	{
		TValueType (TDeclaring::* pointerValue);
		StorageSelector<pointerSize>::Type value;
	};

public:
	typedef typename StorageSelector<pointerSize>::Type UnderlyingType;

	StorageSelector<pointerSize>::Type value;

	template<typename TPtrType>
	static constexpr PointerToMember Get(TPtrType ptr)
	{
		PointerToMemberValue<TPtrType> obj { ptr };
		return { obj.value };
	}

	bool operator==(PointerToMember const& other)
	{
		return value == other.value;
	}
};

struct PointerToMemberHash
{
	size_t operator()(PointerToMember const& obj) const
	{
		auto val = obj.value;
		return std::hash<decltype(val)>()(val);
	}
};

template<typename TPtr>
struct MemberEvent
{

};

template<typename TDeclaring, typename TArgs>
struct MemberEvent<TFC::Core::EventObject<TDeclaring*, TArgs> TDeclaring::*>
{
	typedef TArgs EventArgType;
	typedef TDeclaring DeclaringType;
};


}}}

template<>
struct std::hash<TFC::Core::Introspect::StorageSelector<16>::Type>
{
	typedef TFC::Core::Introspect::StorageSelector<16>::Type argument_type;
	typedef std::size_t result_type;

	result_type operator()(argument_type const& s) const
	{
		std::hash<long long> hasher;

		return hasher(s.value[0]) ^ hasher(s.value[1]);
	}
};


//#define EventOf(EVENT_VAR) decltype(TFC::Core::Introspect::EventOfFunc(& EVENT_VAR))


#endif /* INTROSPECT_H_ */
