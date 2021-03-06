#pragma once

namespace timax{ namespace rpc 
{
	//result要么是基本类型，要么是结构体；当请求成功时，code为0, 如果请求是无返回类型的，则result为空; 
	//如果是有返回值的，则result为返回值。response_msg会序列化为一个标准的json串，回发给客户端。 
	//网络消息的格式是length+body，由4个字节的长度信息（用来指示包体的长度）加包体组成。 
	template<typename T, typename Tag = void>
	struct response_msg
	{
		int		code;
		T		result;
		Tag		tag;
		META(code, result, tag);
	};

	template <typename T>
	struct response_msg<T, void>
	{
		int code;
		T result; //json格式字符串，基本类型或者是结构体.
		META(code, result);
	};

	struct head_t
	{
		int16_t code;
		int16_t	framework_type;
		int32_t len;
	};

	enum class result_code : int16_t
	{
		OK = 0,
		FAIL = 1,
		EXCEPTION = 2,
		ARGUMENT_EXCEPTION = 3
	};

	//
	enum class framework_type
	{
		DEFAULT = 0,
		ROUNDTRIP = 1,
		PUB = 2,
		SUB = 3,
	};

	enum class data_type
	{
		JSON = 0,
		BINARY = 1,
	};

	static std::atomic<std::uint64_t> g_succeed_count(0); //for test qps
} }

