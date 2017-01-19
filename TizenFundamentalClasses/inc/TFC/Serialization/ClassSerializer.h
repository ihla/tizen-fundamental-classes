/*
 * ClassSerializer.h
 *
 *  Created on: Jan 18, 2017
 *      Author: gilang
 */

#ifndef TFC_SERIALIZATION_CLASSSERIALIZER_H_
#define TFC_SERIALIZATION_CLASSSERIALIZER_H_

#include "TFC/Serialization.h"

namespace TFC {
namespace Serialization {

template<typename TSerializerClass, typename TDeclaring, typename... TField>
struct ClassSerializer<TSerializerClass, TDeclaring, TypeSerializationInfo<TDeclaring, TField...>>
{
	static typename TSerializerClass::SerializedType Serialize(TDeclaring const& ptr)
	{
		TSerializerClass packer;
		SerializerFunctor<TSerializerClass, typename TField::ValueType...>::Func(packer, TField::Get(ptr)...);
		return packer.EndPack();
	}

	static void Serialize(TSerializerClass& packer, TDeclaring const& ptr)
	{
		SerializerFunctor<TSerializerClass, typename TField::ValueType...>::Func(packer, TField::Get(ptr)...);
	}
};

template<typename TDeserializerClass, typename TDeclaring, typename TField, typename = void>
struct ClassDeserializerSelect
{
	static void DeserializeAndSet(TDeserializerClass& p, TDeclaring& obj, int curIdx = 0)
	{
		if(TField::Evaluate(obj))
			TField::Set(obj, p.template Deserialize<typename TField::ValueType>(curIdx));
	}
};

template<typename TDeserializerClass, typename TDeclaring, typename... TFieldArgs>
struct ClassDeserializerFunctor;

template<typename TDeserializerClass, typename TDeclaring, typename TCurrentField, typename... TRemainArgs>
struct ClassDeserializerFunctor<TDeserializerClass, TDeclaring, TCurrentField, TRemainArgs...>
{
	static void Func(TDeserializerClass& p, TDeclaring& obj, int curIdx = 0)
	{
		ClassDeserializerSelect<TDeserializerClass, TDeclaring, TCurrentField>::DeserializeAndSet(p, obj, curIdx);
		ClassDeserializerFunctor<TDeserializerClass, TDeclaring, TRemainArgs...>::Func(p, obj, curIdx + 1);
	}
};

template<typename TDeserializerClass, typename TDeclaring>
struct ClassDeserializerFunctor<TDeserializerClass, TDeclaring>
{
	static void Func(TDeserializerClass& p, TDeclaring& obj, int curIdx = 0)
	{

	}
};

template<typename TDeserializerClass, typename TDeclaring, typename... TFieldArgs>
struct ClassDeserializer<TDeserializerClass, TDeclaring, TypeSerializationInfo<TDeclaring, TFieldArgs...>>
{
	static TDeclaring Deserialize(typename TDeserializerClass::SerializedType p, bool finalizePackedObject = true)
	{
		TDeserializerClass unpacker(p);

		TDeclaring ret;
		ClassDeserializerFunctor<TDeserializerClass, TDeclaring, TFieldArgs...>::Func(unpacker, ret);

		if(finalizePackedObject)
			unpacker.Finalize();

		return ret;
	}

	static TDeclaring Deserialize(TDeserializerClass& unpacker)
	{
		TDeclaring ret;
		ClassDeserializerFunctor<TDeserializerClass, TDeclaring, TFieldArgs...>::Func(unpacker, ret);
		return ret;
	}
};

}}



#endif /* TFC_SERIALIZATION_CLASSSERIALIZER_H_ */