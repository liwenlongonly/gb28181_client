#ifndef _RAPIDJSON_CPP_WRAPER_HPP
#define _RAPIDJSON_CPP_WRAPER_HPP
#ifdef _MSC_VER
#pragma once
#pragma warning(disable:4996)
#endif
#define RAPIDJSON_HAS_STDSTRING 1
#define RAPIDJSON_ASSERT(e) ((e) ? static_cast<void>(0)\
	: Rjson::_throw_assert(#e))\


#define RAPIDJSON_CHECK(check, code)				\
	do{												\
		if (!(check))								\
		{											\
			throw GeneralException2(code);			\
		}											\
	} while(0)										\

#define RAPIDJSON_CHECK_MSG(check, code, msg)		\
	do{												\
		if (!(check))								\
		{											\
			throw GeneralException2(code, msg);		\
		}											\
	} while(0)										\


#include "General_exception2.h"
#include <unordered_map>
#include <vector>
#include <type_traits>

#define ERROR_JSON_DOC_NOT_OBJECT 21801 // Document对象无效
#define ERROR_JSON_NODE_EMPTY 21802 // Node为空
#define ERROR_JSON_OBJECT_INVALID 21803 // 无效的object
#define ERROR_JSON_VALUE_INVALID 21804 // 无效的value
#define ERROR_JSON_ARRAY_INVALID 21805 // 无效的array
#define ERROR_JSON_PARSE_FAILED 21806 // json解析失败
#define ERROR_JSON_OPEN_FILE_FAILED 21807 //打开文件失败
#define ERROR_JSON_VALUE_EMPTY 21808 //空value
#define ERROR_JSON_NODE_NOT_EXISTS 21809 //Node不存在
#define ERROR_JSON_TYPE_MISMATCH 21810 //类型不匹配
#define ERROR_JSON_INDEX_OUT_RANGE 21811 //下标超出范围

static std::unordered_map<int, std::string> s_error_msg =
{
	{ ERROR_JSON_DOC_NOT_OBJECT, "Document Is Invalid" },
	{ ERROR_JSON_NODE_EMPTY, "Node Is Empty" },
	{ ERROR_JSON_OBJECT_INVALID, "Object Is Invalid" },
	{ ERROR_JSON_VALUE_INVALID, "Value Is Invalid" },
	{ ERROR_JSON_ARRAY_INVALID, "Array Is Invalid" },
	{ ERROR_JSON_OPEN_FILE_FAILED, "Open File Failed"},
};

namespace Rjson
{
	void _throw_assert(const char* _exp);
}

#include <string>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdarg.h>

#include "error/en.h"
#include "document.h"
#include "rapidjson.h"
#include "prettywriter.h"
#include "filereadstream.h"

using namespace rapidjson;
using std::string;

namespace Rjson{

#if defined(__linux) || defined(__APPLE__)
	inline static int format_string(std::string& str, const char* format, ...) {
		va_list args;
		va_start(args, format);
		char* ps = 0;
		int rt = vasprintf(&ps, format, args);
		if (ps != 0) { str = ps; free(ps); }
		va_end(args);
		return rt;
	}

	inline static std::string format_string(const char* format, ...) 
	{
		std::string str;
		va_list args;
		va_start(args, format);
		char* ps = 0;
		int rt = vasprintf(&ps, format, args);
		if (ps != 0) { str = ps; free(ps); }
		va_end(args);
		return str;
	}

#else//windows
	inline static int format_string(std::string& str, const char* format, ...) {
		va_list args;
		va_start(args, format);
		str.resize(_vscprintf(format, args)); // _vscprintf doesn't count terminating '\0'
		return vsprintf_s((char*)str.data(), str.size() + 1, format, args);
	}
	inline static std::string format_string(const char* format, ...)
	{
		std::string str;
		va_list args;
		va_start(args, format);
		str.resize(_vscprintf(format, args)); // _vscprintf doesn't count terminating '\0'
		vsprintf_s((char*)str.data(), str.size() + 1, format, args);

		return str;
	}
#endif

	inline void _throw_assert(const char* _exp) {
		throw GeneralException2(-1, _exp);
	}

	inline void _throw_assert(int code, const char* msg) 
	{
		throw GeneralException2(code, msg);
	}

	inline void _add_check(const Document& dc, const char* name)
	{
		RAPIDJSON_CHECK(dc.IsObject(), ERROR_JSON_DOC_NOT_OBJECT);
		RAPIDJSON_CHECK(name != nullptr, ERROR_JSON_NODE_EMPTY);
	}

	inline void _add_check(const Document& dc, const char* name, const Value& value, bool isObjCheck = false)
	{
		RAPIDJSON_CHECK(dc.IsObject(), ERROR_JSON_DOC_NOT_OBJECT);
		RAPIDJSON_CHECK(name != nullptr, ERROR_JSON_NODE_EMPTY);
		if (isObjCheck)
		{
			RAPIDJSON_CHECK(value.IsObject(), ERROR_JSON_OBJECT_INVALID);
		}
		else
		{
			RAPIDJSON_CHECK(value.IsArray() || value.IsObject() || value.IsString(), ERROR_JSON_VALUE_INVALID);
		}
	}

	inline void _add_check(const Document& dc, const char* name, const Value& obj, const Value& value)
	{
		RAPIDJSON_CHECK(dc.IsObject(), ERROR_JSON_DOC_NOT_OBJECT);
		RAPIDJSON_CHECK(name != nullptr, ERROR_JSON_NODE_EMPTY);
		RAPIDJSON_CHECK(obj.IsObject(), ERROR_JSON_OBJECT_INVALID);
		RAPIDJSON_CHECK(value.IsArray() || value.IsObject() || value.IsString(), ERROR_JSON_VALUE_INVALID);
	}

	inline void _push_check(const Document& dc, const Value& array)
	{
		RAPIDJSON_CHECK(dc.IsObject(), ERROR_JSON_DOC_NOT_OBJECT);
		RAPIDJSON_CHECK(array.IsArray(), ERROR_JSON_ARRAY_INVALID);
	}

	//parser
	static string GetParseErrStr(Document& dc)
	{
		if (dc.HasParseError())
		{
			string msg;
			format_string(msg, "Parse error,errcode:%d,offset:%d,errmsg:%s", dc.GetParseError(), dc.GetErrorOffset(), GetParseError_En(dc.GetParseError()));
			return msg;
		}
		return "";
	}

	static void Parse(Document &dc, const string& json)
	{
		dc.Parse<0>(json.c_str());
		RAPIDJSON_CHECK_MSG(!dc.HasParseError(), ERROR_JSON_PARSE_FAILED, GetParseErrStr(dc).data());
	}

	static void ParseFile(Document& dc, const string& filename)
	{
		FILE* f = fopen(filename.data(), "rb");
		RAPIDJSON_CHECK_MSG(f != nullptr, ERROR_JSON_OPEN_FILE_FAILED, "File Open Failed");
		char read_buffer[65536] = { '\0' };
		FileReadStream reader_stream(f, read_buffer, sizeof(read_buffer));
		dc.ParseStream(reader_stream);
		fclose(f);
		RAPIDJSON_CHECK_MSG(!dc.HasParseError(), ERROR_JSON_PARSE_FAILED, GetParseErrStr(dc).data());
	}


	//node to string
	static void ToString(string& str, const Value* node)
	{
		RAPIDJSON_CHECK(node != nullptr, ERROR_JSON_VALUE_EMPTY);
		StringBuffer sb;
		Writer<StringBuffer> writer(sb); // PrettyWriter
		node->Accept(writer);
		str = sb.GetString();
	}

	static string ToString(const Value* node)
	{
		RAPIDJSON_CHECK(node != nullptr, ERROR_JSON_VALUE_EMPTY);
		StringBuffer sb;
		Writer<StringBuffer> writer(sb); // PrettyWriter
		node->Accept(writer);
		return sb.GetString();
	}
	//dc to string
	static string ToString(Document& dc)
	{
		RAPIDJSON_CHECK(dc.IsObject(), ERROR_JSON_DOC_NOT_OBJECT);
		StringBuffer sb;
		Writer<StringBuffer> writer(sb); // PrettyWriter
		dc.Accept(writer);
		return sb.GetString();
	}

	static void SaveJson(Document& dc, const string& filename)
	{
		RAPIDJSON_CHECK(dc.IsObject(), ERROR_JSON_DOC_NOT_OBJECT);
		RAPIDJSON_CHECK_MSG(!filename.empty(), ERROR_JSON_OPEN_FILE_FAILED, "filename empty");

		string jsonstr = ToString(dc);
		FILE *fp = fopen(filename.c_str(), "wb");
		RAPIDJSON_CHECK_MSG(fp != nullptr, ERROR_JSON_OPEN_FILE_FAILED, "File Open Failed");
		fwrite(jsonstr.c_str(), 1, jsonstr.size(), fp);
		fclose(fp);
	}

	static void SaveJson(const Value* node, const string& filename)
	{
		RAPIDJSON_CHECK(node != nullptr, ERROR_JSON_VALUE_EMPTY);
		RAPIDJSON_CHECK(node->IsObject(), ERROR_JSON_OBJECT_INVALID);
		RAPIDJSON_CHECK_MSG(!filename.empty(), ERROR_JSON_OPEN_FILE_FAILED, "filename empty");

		string jsonstr = ToString(node);
		FILE *fp = fopen(filename.c_str(), "wb");
		RAPIDJSON_CHECK_MSG(fp != nullptr, ERROR_JSON_OPEN_FILE_FAILED, "File Open Failed");
		fwrite(jsonstr.c_str(), 1, jsonstr.size(), fp);
		fclose(fp);
	}

	//==================== GetValue/GetValueV ==================== 
	static void GetValue(const Value** value, const char* name, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		RAPIDJSON_CHECK(name != nullptr, ERROR_JSON_NODE_EMPTY);
		Value::ConstMemberIterator itr = parent->FindMember(name);
		RAPIDJSON_CHECK_MSG(itr != parent->MemberEnd(), ERROR_JSON_NODE_NOT_EXISTS,
			format_string("key='%s' not exist", name));
		*value = &(itr->value);
	}

	static void GetValue(const Value** value, size_t idx, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		RAPIDJSON_CHECK(parent->IsArray(), ERROR_JSON_ARRAY_INVALID);
		RAPIDJSON_CHECK_MSG(idx >= 0 && idx < (int)parent->Size(), ERROR_JSON_INDEX_OUT_RANGE,
			format_string("Index [%d] out of arry range", idx));
		*value = &((*parent)[(unsigned)idx]);
	}

	template<typename T1, typename T2>
	static void GetValue(const Value** value, T1 t1, T2 t2, const Value* parent)
	{
		const Value* tmpv = NULL;
		GetValue(&tmpv, t1, parent);
		GetValue(value, t2, tmpv);
	}

	static bool GetValueV(const Value** value, const char* name, const Value* parent)
	{
		if (parent && name && parent->IsObject())
		{
			Value::ConstMemberIterator itr = parent->FindMember(name);
			if (itr != parent->MemberEnd())
			{
				*value = &(itr->value);
				return true;
			}
		}
		return false;
	}

	static bool GetValueV(const Value** value, size_t idx, const Value* parent)
	{
		if (parent && idx >= 0 && parent->IsArray() && idx < (int)parent->Size())
		{
			*value = &((*parent)[(unsigned)idx]);
			return true;
		}
		return false;
	}

	template<typename T1, typename T2>
	static bool GetValueV(const Value** value, T1 t1, T2 t2, const Value* parent)
	{
		const Value* tmpv = NULL;
		if (GetValueV(&tmpv, t1, parent))
		{
			return GetValueV(value, t2, tmpv);
		}
		return false;
	}

	//==================== GetString/GetStringV ==================== 
	template<typename T>
	static string GetString(T t, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		const Value* value = nullptr;
		GetValue(&value, t, parent);
		bool isIntegral = std::is_integral<T>::value;
		if (isIntegral)
		{
			RAPIDJSON_CHECK_MSG(value->IsString(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not string--- element[%d]", t));
		}
		else
		{
			RAPIDJSON_CHECK_MSG(value->IsString(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not string---key='%s'", t));
		}
		return value->GetString();
	}

	template<typename T1, typename T2>
	static string GetString(T1 t1, T2 t2, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		const Value* value = NULL;
		GetValue(&value, t1, t2, parent);
		bool isIntegral = std::is_integral<T2>::value;
		if (isIntegral)
		{
			RAPIDJSON_CHECK_MSG(value->IsString(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not string--- element[%d]", t2));
		}
		else
		{
			RAPIDJSON_CHECK_MSG(value->IsString(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not string---key='%s'", t2));
		}
		return value->GetString();
	}

	template<typename T>
	static bool GetStringV(string& s, T t, const Value* parent)
	{
		const Value* value = NULL;
		if (GetValueV(&value, t, parent) && value->IsString())
		{
			s = value->GetString();
			return true;
		}
		return false;
	}

	template<typename T1, typename T2>
	static bool GetStringV(string& str, T1 t1, T2 t2, const Value* parent)
	{
		const Value* value = NULL;
		if (GetValueV(&value, t1, t2, parent) && value->IsString())
		{
			str = value->GetString();
			return true;
		}
		return false;
	}

	//==================== GetObject/GetObjectV ==================== 
	template<typename T>
	static const Value* GetObject(T t, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		const Value* value = NULL;
		GetValue(&value, t, parent);
		bool isIntegral = std::is_integral<T>::value;
		if (isIntegral)
		{
			RAPIDJSON_CHECK_MSG(value->IsObject(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not object--- element[%d]", t));
		}
		else
		{
			RAPIDJSON_CHECK_MSG(value->IsObject(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not object---key='%s'", t));
		}
		return value;
	}

	template<typename T1, typename T2>
	static const Value* GetObject(T1 t1, T2 t2, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		const Value* value = NULL;
		GetValue(&value, t1, t2, parent);
		bool isIntegral = std::is_integral<T2>::value;
		if (isIntegral)
		{
			RAPIDJSON_CHECK_MSG(value->IsObject(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not object--- element[%d]", t2));
		}
		else
		{
			RAPIDJSON_CHECK_MSG(value->IsObject(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not object---key='%s'", t2));
		}
		return value;
	}

	template<typename T>
	static bool GetObjectV(const Value** value, T t, const Value* parent)
	{
		if (GetValueV(value, t, parent) && (*value)->IsObject())
			return true;
		*value = NULL;
		return false;
	}

	template<typename T1, typename T2>
	static bool GetObjectV(const Value** value, T1 t1, T2 t2, const Value* parent)
	{
		if (GetValueV(value, t1, t2, parent) && (*value)->IsObject())
			return true;
		*value = NULL;
		return false;
	}

	//==================== GetArray/GetArrayV ==================== 
	template<typename T>
	static const Value* GetArray(T t, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		const Value* value = NULL;
		GetValue(&value, t, parent);
		bool isIntegral = std::is_integral<T>::value;
		if (isIntegral)
		{
			RAPIDJSON_CHECK_MSG(value->IsArray(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not array--- element[%d]", t));
		}
		else
		{
			RAPIDJSON_CHECK_MSG(value->IsArray(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not array---key='%s'", t));
		}
		return value;
	}

	template<typename T1, typename T2>
	static const Value* GetArray(T1 t1, T2 t2, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		const Value* value = NULL;
		GetValue(&value, t1, t2, parent);
		bool isIntegral = std::is_integral<T2>::value;
		if (isIntegral)
		{
			RAPIDJSON_CHECK_MSG(value->IsArray(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not array--- element[%d]", t2));
		}
		else
		{
			RAPIDJSON_CHECK_MSG(value->IsArray(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not array---key='%s'", t2));
		}
		return value;
	}

	template<typename T1, typename T2>
	static bool GetArrayV(const Value** value, T1 t1, T2 t2, const Value* parent)
	{
		if (GetValueV(value, t1, t2, parent) && (*value)->IsArray())
			return true;
		*value = NULL;
		return false;
	}

	template<typename T>
	static bool GetArrayV(const Value** value, T t, const Value* parent)
	{
		if (GetValueV(value, t, parent) && (*value)->IsArray())
			return true;
		*value = NULL;
		return false;
	}

	//==================== GetBool/GetBoolV ====================
	template<typename T>
	static bool GetBool(T t, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		const Value* value = NULL;
		GetValue(&value, t, parent);
		bool isIntegral = std::is_integral<T>::value;
		if (isIntegral)
		{
			RAPIDJSON_CHECK_MSG(value->IsBool(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not bool--- element[%d]", t));
		}
		else
		{
			RAPIDJSON_CHECK_MSG(value->IsBool(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not bool---key='%s'", t));
		}
		return value->GetBool();	
	}

	template<typename T1, typename T2>
	static bool GetBool(T1 t1, T2 t2, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		const Value* value = NULL;
		GetValue(&value, t1, t2, parent);
		bool isIntegral = std::is_integral<T2>::value;
		if (isIntegral)
		{
			RAPIDJSON_CHECK_MSG(value->IsBool(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not bool--- element[%d]", t2));
		}
		else
		{
			RAPIDJSON_CHECK_MSG(value->IsBool(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not bool---key='%s'", t2));
		}
		return value->GetBool();
	}

	template<typename T>
	static bool GetBoolV(bool& b, T t, const Value* parent)
	{
		const Value* value = NULL;
		if (GetValueV(&value, t, parent) && value->IsBool())
		{
			b = value->GetBool();
			return true;
		}
		return false;
	}

	template<typename T1, typename T2>
	static bool GetBoolV(bool& b, T1 t1, T2 t2, const Value* parent)
	{
		const Value* value = NULL;
		if (GetValueV(&value, t1, t2, parent) && value->IsBool())
		{
			b = value->GetBool();
			return true;
		}
		return false;
	}

	//==================== GetInt/GetIntV ====================
	template<typename T>
	static int GetInt(T t, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		const Value* value = NULL;
		GetValue(&value, t, parent);
		bool isIntegral = std::is_integral<T>::value;
		if (isIntegral)
		{
			RAPIDJSON_CHECK_MSG(value->IsInt(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not int--- element[%d]", t));
		}
		else
		{
			RAPIDJSON_CHECK_MSG(value->IsInt(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not int---key='%s'", t));
		}
		return value->GetInt();
	}

	template<typename T1, typename T2>
	static int GetInt(T1 t1, T2 t2, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		const Value* value = NULL;
		GetValue(&value, t1, t2, parent);
		bool isIntegral = std::is_integral<T2>::value;
		if (isIntegral)
		{
			RAPIDJSON_CHECK_MSG(value->IsInt(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not int--- element[%d]", t2));
		}
		else
		{
			RAPIDJSON_CHECK_MSG(value->IsInt(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not int---key='%s'", t2));
		}
		return value->GetInt();
	}

	template<typename T1, typename T2>
	static bool GetIntV(int& n, T1 t1, T2 t2, const Value* parent)
	{
		const Value* value = NULL;
		if (GetValueV(&value, t1, t2, parent) && value->IsInt())
		{
			n = value->GetInt();
			return true;
		}
		return false;
	}

	template<typename T>
	static bool GetIntV(int& n, T t, const Value* parent)
	{
		const Value* value = NULL;
		if (GetValueV(&value, t, parent) && value->IsInt())
		{
			n = value->GetInt();
			return true;
		}
		return false;
	}

	//==================== GetUint/GetUintV ====================
	template<typename T>
	static uint32_t GetUint(T t, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		const Value* value = NULL;
		GetValue(&value, t, parent);
		bool isIntegral = std::is_integral<T>::value;
		if (isIntegral)
		{
			RAPIDJSON_CHECK_MSG(value->IsUint(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not uint--- element[%d]", t));
		}
		else
		{
			RAPIDJSON_CHECK_MSG(value->IsUint(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not uint---key='%s'", t));
		}
		return value->GetUint();
	}

	template<typename T1, typename T2>
	static uint32_t GetUint(T1 t1, T2 t2, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		const Value* value = NULL;
		GetValue(&value, t1, t2, parent);
		bool isIntegral = std::is_integral<T2>::value;
		if (isIntegral)
		{
			RAPIDJSON_CHECK_MSG(value->IsUint(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not uint--- element[%d]", t2));
		}
		else
		{
			RAPIDJSON_CHECK_MSG(value->IsUint(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not uint---key='%s'", t2));
		}
		return value->GetUint();
	}

	template<typename T1, typename T2>
	static bool GetUintV(uint32_t& n, T1 t1, T2 t2, const Value* parent)
	{
		const Value* value = NULL;
		if (GetValueV(&value, t1, t2, parent) && value->IsUint())
		{
			n = value->GetUint();
			return true;
		}
		return false;
	}

	template<typename T>
	static bool GetUintV(uint32_t& n, T t, const Value* parent)
	{
		const Value* value = NULL;
		if (GetValueV(&value, t, parent) && value->IsUint())
		{
			n = value->GetUint();
			return true;
		}
		return false;
	}

	//==================== GetInt64/GetInt64V ====================
	template<typename T>
	static int64_t GetInt64(T t, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		const Value* value = NULL;
		GetValue(&value, t, parent);
		bool isIntegral = std::is_integral<T>::value;
		if (isIntegral)
		{
			RAPIDJSON_CHECK_MSG(value->IsInt64(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not int64--- element[%d]", t));
		}
		else
		{
			RAPIDJSON_CHECK_MSG(value->IsInt64(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not int64---key='%s'", t));
		}
		return value->GetInt64();
	}

	template<typename T1, typename T2>
	static int64_t GetInt64(T1 t1, T2 t2, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		const Value* value = NULL;
		GetValue(&value, t1, t2, parent);
		bool isIntegral = std::is_integral<T2>::value;
		if (isIntegral)
		{
			RAPIDJSON_CHECK_MSG(value->IsInt64(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not int64--- element[%d]", t2));
		}
		else
		{
			RAPIDJSON_CHECK_MSG(value->IsInt64(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not int64---key='%s'", t2));
		}
		return value->GetInt64();
	}

	template<typename T1, typename T2>
	static bool GetInt64V(int64_t& n, T1 t1, T2 t2, const Value* parent)
	{
		const Value* value = NULL;
		if (GetValueV(&value, t1, t2, parent) && value->IsInt64())
		{
			n = value->GetInt64();
			return true;
		}
		return false;
	}

	template<typename T>
	static bool GetInt64V(int64_t& n, T t, const Value* parent)
	{
		const Value* value = NULL;
		if (GetValueV(&value, t, parent) && value->IsInt64())
		{
			n = value->GetInt64();
			return true;
		}
		return false;
	}

	//==================== GetUint64/GetUint64V ====================
	template<typename T>
	static uint64_t GetUint64(T t, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		const Value* value = NULL;
		GetValue(&value, t, parent);
		bool isIntegral = std::is_integral<T>::value;
		if (isIntegral)
		{
			RAPIDJSON_CHECK_MSG(value->IsUint64(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not uint64--- element[%d]", t));
		}
		else
		{
			RAPIDJSON_CHECK_MSG(value->IsUint64(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not uint64---key='%s'", t));
		}
		return value->GetUint64();
	}

	template<typename T1, typename T2>
	static uint64_t GetUint64(T1 t1, T2 t2, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		const Value* value = NULL;
		GetValue(&value, t1, t2, parent);
		bool isIntegral = std::is_integral<T2>::value;
		if (isIntegral)
		{
			RAPIDJSON_CHECK_MSG(value->IsUint64(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not uint64--- element[%d]", t2));
		}
		else
		{
			RAPIDJSON_CHECK_MSG(value->IsUint64(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not uint64---key='%s'", t2));
		}
		return value->GetUint64();
	}

	template<typename T1, typename T2>
	static bool GetUint64V(uint64_t& n, T1 t1, T2 t2, const Value* parent)
	{
		const Value* value = NULL;
		if (GetValueV(&value, t1, t2, parent) && value->IsUint64())
		{
			n = value->GetUint64();
			return true;
		}
		return false;
	}

	template<typename T>
	static bool GetUint64V(uint64_t& n, T t, const Value* parent)
	{
		const Value* value = NULL;
		if (GetValueV(&value, t, parent) && value->IsUint64())
		{
			n = value->GetUint64();
			return true;
		}
		return false;
	}

	//==================== GetFloat/GetFloatV ====================
	template<typename T>
	static float GetFloat(T t, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		const Value* value = NULL;
		GetValue(&value, t, parent);
		bool isIntegral = std::is_integral<T>::value;
		if (isIntegral)
		{
			RAPIDJSON_CHECK_MSG(value->IsFloat(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not float--- element[%d]", t));
		}
		else
		{
			RAPIDJSON_CHECK_MSG(value->IsFloat(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not float---key='%s'", t));
		}
		return value->GetFloat();
	}

	template<typename T1, typename T2>
	static float GetFloat(T1 t1, T2 t2, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		const Value* value = NULL;
		GetValue(&value, t1, t2, parent);
		bool isIntegral = std::is_integral<T2>::value;
		if (isIntegral)
		{
			RAPIDJSON_CHECK_MSG(value->IsFloat(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not float--- element[%d]", t2));
		}
		else
		{
			RAPIDJSON_CHECK_MSG(value->IsFloat(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not float---key='%s'", t2));
		}
		return value->GetFloat();
	}

	template<typename T1, typename T2>
	static bool GetFloatV(float& f, T1 t1, T2 t2, const Value* parent)
	{
		const Value* value = NULL;
		if (GetValueV(&value, t1, t2, parent) && value->IsFloat())
		{
			f = value->GetFloat();
			return true;
		}
		return false;
	}

	template<typename T>
	static bool GetFloatV(float& f, T t, const Value* parent)
	{
		const Value* value = NULL;
		if (GetValueV(&value, t, parent) && value->IsFloat())
		{
			f = value->GetFloat();
			return true;
		}
		return false;
	}

	//==================== GetDouble/GetDoubleV ====================
	template<typename T>
	static double GetDouble(T t, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		const Value* value = NULL;
		GetValue(&value, t, parent);
		bool isIntegral = std::is_integral<T>::value;
		if (isIntegral)
		{
			RAPIDJSON_CHECK_MSG(value->IsDouble(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not double--- element[%d]", t));
		}
		else
		{
			RAPIDJSON_CHECK_MSG(value->IsDouble(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not double---key='%s'", t));
		}
		return value->GetDouble();
	}

	template<typename T1, typename T2>
	static double GetDouble(T1 t1, T2 t2, const Value* parent)
	{
		RAPIDJSON_CHECK(parent != nullptr, ERROR_JSON_VALUE_EMPTY);
		const Value* value = NULL;
		GetValue(&value, t1, t2, parent);
		bool isIntegral = std::is_integral<T2>::value;
		if (isIntegral)
		{
			RAPIDJSON_CHECK_MSG(value->IsDouble(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not double--- element[%d]", t2));
		}
		else
		{
			RAPIDJSON_CHECK_MSG(value->IsDouble(), ERROR_JSON_TYPE_MISMATCH,
				format_string("Data type mismatch: value not double---key='%s'", t2));
		}
		return value->GetDouble();
	}

	template<typename T1, typename T2>
	static bool GetDoubleV(double& d, T1 t1, T2 t2, const Value* parent)
	{
		const Value* value = NULL;
		if (GetValueV(&value, t1, t2, parent) && value->IsDouble())
		{
			d = value->GetDouble();
			return true;
		}
		return false;
	}

	template<typename T>
	static bool GetDoubleV(double& d, T t, const Value* parent)
	{
		const Value* value = NULL;
		if (GetValueV(&value, t, parent) && value->IsDouble())
		{
			d = value->GetDouble();
			return true;
		}
		return false;
	}

	
	//writer
	static Document rWriteDC()
	{
		Document dc;
		dc.SetObject();
		return dc;
	}

	static Value rObject()
	{
		Value obj(kObjectType);
		return obj;
	}

	static Value rArray()
	{
		Value array(kArrayType);
		return array;
	}

	//dc add value
	template <typename T>
	static void rAdd(Document& dc, const char* name, T value)
	{
		_add_check(dc, name);
		dc.AddMember(StringRef(name), value, dc.GetAllocator());
	}

	//dc add obj or arry
	static void rAdd(Document& dc, const char* name, Value& value)
	{
		_add_check(dc, name, value);
		dc.AddMember(StringRef(name), value, dc.GetAllocator());
	}

	//obj add value
	template <typename T>
	static void rAdd(Document& dc, Value& obj, const char* name, T value)
	{
		_add_check(dc, name, obj, true);
		obj.AddMember(StringRef(name), value, dc.GetAllocator());
	}

	//obj add obj or arry
	static void rAdd(Document& dc, Value& obj, const char* name, Value& value)
	{
		_add_check(dc, name, obj, value);
		obj.AddMember(StringRef(name), value, dc.GetAllocator());
	}

	//arry push value
	template <typename T>
	static void rPush(Document& dc, Value &arry, T value)
	{
		_push_check(dc, arry);
		arry.PushBack(value, dc.GetAllocator());
	}

	//arry push obj or arry
	static void rPush(Document& dc, Value &arry, Value& value)
	{
		_push_check(dc, arry);
		arry.PushBack(value, dc.GetAllocator());
	}
	
//-------------------------------------------------------
/*
	基于C++11的高易用json序列化/反序列化系统
*/

	///前置声明


	class FVal;
	template<class T> class LVal;
	class RVal;

	//用于构造json-object的“名字-值”对
	class NameValuePair{
	public:
		template<class T>
		NameValuePair(const char* s, T&& f);
		const char* name;
		inline const FVal* pv() const;
	private:
		void* _dt[2];//FVal val;
		uint32_t _pad;
	};

	//用于构造json-object的C++11初始化列表类型
	//注意使用列表初始化时，传入数据必须保证函数执行完毕生存期有效
	using object_initlist = std::initializer_list<NameValuePair>;

	//用于构造json-array的C++11初始化列表类型
	//注意使用列表初始化时，传入数据必须保证函数执行完毕生存期有效
	using list_initlist = std::initializer_list<FVal>;



/*
	用于进行json构造的传参辅助数据结构。
	常用于列表初始化以及多类型初始化适配（包含各种隐式类型转换等）。
	正常情况下开发者一般不会直接使用。
*/
class FVal{
	union
	{
		void* _res[2];
		uint64_t u64_;
		bool b_;
		int64_t i64_;
		double d_;
		const char* p_;
	};

	uint32_t flags_;
	enum { fEmptyType, fBoolType, fIntType, fUIntType, fRealType, fStrType, fArrType, fObjType };
	friend FVal ARR(const list_initlist& ol);
public:
	FVal(const FVal& f) :flags_(f.flags_) {
		_res[0] = f._res[0];
		_res[1] = f._res[1];
		//f.flags_ = fEmptyType;
	}

	FVal() :flags_(fEmptyType) {	}

	FVal(bool v) :flags_(fBoolType) {
		b_ = v;
	}

	FVal(int v) :flags_(fIntType) {
		i64_ = v;
	}
	FVal(int64_t v) :flags_(fIntType) {
		i64_ = v;
	}

	FVal(unsigned int v) :flags_(fUIntType) {
		u64_ = v;
	}

	FVal(uint64_t v) :flags_(fUIntType) {
		u64_ = v;
	}

	FVal(float v) :flags_(fRealType) {
		d_ = v;
	}

	FVal(double v) :flags_(fRealType) {
		d_ = v;
	}

	FVal(const char* v) :flags_(fStrType) {
		p_ = v;
	}

	//注意，不使用const引用，是因为要求传入的string实例全生存期有效！
	//禁止使用函数临时返回的string对象进行FVal初始化
	FVal(std::string& v) :flags_(fStrType) {
		p_ = v.c_str();
	}

	FVal(const object_initlist& ol):flags_(fObjType) {
		new (_res)object_initlist(ol);
	}

	/*
		为了防止json-object的列表初始化与json-array的列表初始化混淆
		（如：类似{"name", 1}这样的列表既可以匹配object_initlist也可以匹配list_initlist）
		将json-list构造单独拿出（使用ARR函数），不再提供默认构造函数。
	*/
	//FVal(const list_initlist& ll) :flags_(fArrType) {
	//	new (_res)list_initlist(ll);
	//}
		
	template<class Allocator>
	Value val(Allocator& al) const {
		//return Value(v, al);
		switch (flags_)
		{
		case fBoolType:
			return Value(b_);
		case fIntType:
			return Value(i64_);
		case fUIntType:
			return Value(u64_);
		case fRealType:
			return Value(d_);
		case fStrType:
			return Value(p_, al);
		case fArrType: {
			list_initlist* pl = (list_initlist*)_res;
			Value v(kArrayType);
			for (auto& i : *pl) {
				v.PushBack(i.val(al), al);
			}
			return v;
			}
		case fObjType: {
			object_initlist* pl = (object_initlist*)_res;
			Value v(kObjectType);
			for (auto& i : *pl) {
				v.AddMember(Value(i.name, al), i.pv()->val(al), al);
			}
			return v;
			}
		case fEmptyType:
			break;
		default:
			RAPIDJSON_ASSERT(false);
		}
		return Value();
	}
};

template<class T>
NameValuePair::NameValuePair(const char* s, T&& f) :name(s) {
	new (_dt)FVal(f);
}

inline const FVal* NameValuePair::pv() const {
	return (FVal*)_dt;
}
/*
	用于辅助列表初始化方法构造json-array的helper-function
	@param: ll 针对json-array的初始化列表。
	@return: 使用初始化列表构造的FVal实例
	注意：初始化列表中传入的值，除了基本数值类型（int,float等）外，要求在初始化列表使用期间全生存期有效！特别是string类型！
		禁止使用函数临时返回的string对象进行FVal初始化
*/
inline FVal ARR(const list_initlist& ll) {
	FVal v;
	v.flags_ = FVal::fArrType;
	new (v._res)list_initlist(ll);
	return v;
}

/*
	用于直接返回json-null元素的helper-function。一般配合列表初始化使用。
	@return: null值的FVal实例
*/
inline static FVal Null() {
	return FVal();
}




/*
	Rjson“右值”类型。数据只读。支持：
	> 以下标[index]形式进行array元素访问
	> 以下标[name]形式进行object成员访问
	> 以特定类型转换获取指定元素的内部存储数据类型
	访问、转换失败会抛出RjException
	注意：所有的右值类型实例均来自于对源Doc里数据节点的引用。因此在Doc实例之外保存RVal实例是无意义的
*/
class RVal {
	template<class T>
	friend class LVal;
	Value const& r;
public:
	RVal(Value const& v) :r(v) {}
	RVal(RVal&& rv) : r(rv.r) {}

	//---------array操作方法组------------------

	/*
		json-array下标访问获取元素操作
		@brief: 注意：需要保证此实例节点的类型是json-array。否则会抛出异常。
		@param: idx 访问array元素的整数下标。如果下标超出限制会抛异常。
		@return: 成功访问到的元素实例
		@throw: RjException
	*/
	RVal operator[](SizeType idx) const {
		RAPIDJSON_ASSERT(r.IsArray());
		return r[idx];
	}

	/*
		json-array获取长度操作
		@brief: 注意：需要保证此实例节点的类型是json-array。否则会抛出异常。
		@return: array长度
		@throw: RjException
	*/
	SizeType size() const {
		return r.Size();
	}

	/*
		json-array下标访问获取元素操作。与[]版本区别是，此方法在访问元素失败(下标越界)时不会抛异常，而是返回false
		@brief: 注意：需要保证此实例节点的类型是json-array。否则会抛出异常。
		@param: idx 访问array元素的整数下标。如果下标超出限制会返回false。待写入实例v不做任何修改。
		@param: v  待写入实例。注意需要保证实例类型与json中元素类型匹配。不匹配会抛出异常。
		@return: 是否成功获取到元素值
		@throw: RjException
	*/
	bool get_elem(SizeType idx, std::string& v) const {
		if (idx >= size()) return false;
		auto& ref = r[idx];
		v.assign(ref.GetString(), ref.GetStringLength());
		return true;
	}

	/*
		json-array下标访问获取元素操作。与[]版本区别是，此方法在访问元素失败(下标越界)时不会抛异常，而是返回false
		@brief: 注意：需要保证此实例节点的类型是json-array。否则会抛出异常。
		@param: idx 访问array元素的整数下标。如果下标超出限制会返回false。待写入实例v不做任何修改。
		@param: v  待写入实例。注意需要保证实例类型与json中元素类型匹配。不匹配会抛出异常。
		@return: 是否成功获取到元素值
		@throw: RjException
	*/
	template<class T>
	bool get_elem(SizeType idx, T& v) const {
		if (idx >= size()) return false;
		v = r[idx].Get<T>();
		return true;
	}

	/*
		json-array整体获取元素操作。支持传入一个声明的数组来获取array里各元素值。
		@brief: 注意：需要保证此实例节点的类型是json-array。否则会抛出异常。
		@param: arr 声明的数组。如 int[8]; double[5]; std::string[10];等
		@return: 成功取到的元素值个数。具体为arr长度与实际json-array长度的最小值
		@throw: RjException
	*/
	template<class T, size_t N, class ...Types>
	int get_arr(T(&arr)[N]) const {
		RAPIDJSON_ASSERT(r.IsArray());
		SizeType rlen = r.Size();
		if (rlen > N) rlen = (SizeType)N;
		for (SizeType i = 0; i < rlen; i++) {
			arr[i] = RVal(r[i]);
		}
		return (int)rlen;
	}
	
	template<class T>
	int get_arr(std::vector<T>& arr) const {
		RAPIDJSON_ASSERT(r.IsArray());
		SizeType rlen = r.Size();
		arr.resize(rlen);
		for (SizeType i = 0; i < rlen; i++) {
			arr[i] = RVal(r[i]);
		}
		return (int)rlen;
	}
		
	/*
		json-object成员获取操作
		@brief: 注意：需要保证此实例节点的类型是json-object。否则会抛出异常。
		@param: name 待访问获取的成员名字。如果此json-object不包含此名字则抛出异常。
		@return: 成功取到的成员实例。
		@throw: RjException
	*/
	RVal operator[](const std::string& name) const {
		RAPIDJSON_ASSERT(r.IsObject() && r.HasMember(name));
		return r[name];
	}

	/*
		json-object检查成员是否存在
		@brief: 注意：需要保证此实例节点的类型是json-object。否则会抛出异常。
		@param: name 待检查的成员名字。
		@return: 是否含有此成员
		@throw: RjException
	*/
	bool has_val(const std::string& name) const {
		return r.HasMember(name);
	}
	
	/*
		json-object成员获取操作。与[]方法不同的是，此方法在object里没有成员时不会抛异常，而是返回false。
		@brief: 注意：需要保证此实例节点的类型是json-object。否则会抛出异常。
		@param: name 待访问获取的成员名字。如果此json-object不包含此名字则返回false，v值不做修改
		@param: v 待写入实例。注意需要保证实例类型与json中元素类型匹配。不匹配会抛出异常。
		@return: 是否成功获取此成员。
		@throw: RjException
	*/
	bool get_val(const std::string& name, std::string& v) const {
		if (!has_val(name)) return false;
		auto& ref = r[name];
		v.assign(ref.GetString(), ref.GetStringLength());
		return true;
	}

	/*
		json-object成员获取操作。与[]方法不同的是，此方法在object里没有成员时不会抛异常，而是返回false。
		@brief: 注意：需要保证此实例节点的类型是json-object。否则会抛出异常。
		@param: name 待访问获取的成员名字。如果此json-object不包含此名字则返回false，v值不做修改
		@param: v 待写入实例。注意需要保证实例类型与json中元素类型匹配。不匹配会抛出异常。
		@return: 是否成功获取此成员。
		@throw: RjException
	*/
	template<class T>
	bool get_val(const std::string& name, T& v) const {
		if (!has_val(name)) return false;
		v = r[name].Get<T>();
		return true;
	}

	//以下为通用类型获取方法。

	//以std::string类型访问数据（类型转换）
	std::string get_str() const {
		return{ r.GetString(), r.GetStringLength() };
	}

	bool get_str(std::string& v) const {
		if (!r.IsString())
			return false;
		v.assign(r.GetString(), r.GetStringLength());
		return true;
	}
	
	inline operator std::string() const {
		return get_str();
	}

	//以其他类型访问数据（int, float等）
	template<class T>
	inline operator T() const {
		return r.Get<T>();
	}

	template<class T>
	bool get(T& v) const {
		try{
			v = r.Get<T>();
			return true;
		}
		catch (...) {
			return false;
		}
	}

	int get_int() const {
		return r.GetInt();
	}

	bool get_int(int& v) const {
		if (!r.IsInt())
			return false;
		v = r.GetInt();
		return true;
	}

	int64_t get_i64() const {
		return r.GetInt64();
	}

	bool get_i64(int64_t& v) const {
		if (!r.IsInt64())
			return false;
		v = r.GetInt64();
		return true;
	}

	unsigned get_uint() const {
		return r.GetUint();
	}

	bool get_uint(unsigned& v) const {
		if (!r.IsUint())
			return false;
		v = r.GetUint();
		return true;
	}

	uint64_t get_u64() const {
		return r.GetUint64();
	}

	bool get_u64(uint64_t& v) const {
		if (!r.IsUint64())
			return false;
		v = r.GetUint64();
		return true;
	}

	float get_float() const {
		return r.GetFloat();
	}

	bool get_float(float& v) const {
		if (!r.IsFloat())
			return false;
		v = r.GetFloat();
		return true;
	}

	double get_double() const {
		return r.GetDouble();
	}

	bool get_double(double& v) const {
		if (!r.IsDouble())
			return false;
		v = r.GetDouble();
		return true;
	}

	bool get_bool() const {
		return r.GetBool();
	}

	bool get_bool(bool& v) const {
		if (!r.IsBool())
			return false;
		v = r.GetBool();
		return true;
	}

	//此节点数据转json字符串
	std::string dumps() const {
		StringBuffer sb;
		Writer<StringBuffer> writer(sb); // PrettyWriter
		RAPIDJSON_ASSERT(r.Accept(writer));
		return{ sb.GetString(), sb.GetSize() };
	}
};

/*
	Rjson“左值”类型。数据可写。支持：
	> 从其他Doc节点里复制内容（移花接木）
	> 将当期节点设置为json-object，并添加内容
	> 将当期节点设置为json-array，并添加内容
	> 将当期节点设置为string/int/float/null...，并设置内容
	> 以下标方式访问object/array下属子节点。
	> 使用rv()方法获取R-value。
	访问、转换失败会抛出RjException
	注意：所有的右值类型实例均来自于对源Doc里数据节点的引用。因此在Doc实例之外保存RVal实例是无意义的
*/
template<class AlType>
class LVal {
	Value& r;
	AlType& al;
public:
	LVal(Value& v, AlType& a) :r(v), al(a) {}
	LVal(const LVal& l) : r(l.r), al(l.al) {}

	void copy_from(const RVal& v) {
		r.CopyFrom(v.r, al, true);
	}

	//for object 

	LVal& set_obj() {
		r.SetObject();
		return *this;
	}

	LVal& set_obj(const object_initlist& ol) {
		r.SetObject();
		for (auto& i : ol) {
			r.AddMember(StringRef(i.name), i.pv()->val(al), al);
		}
		return *this;
	}

	LVal& add(const std::string& name, FVal&& v) {
		RAPIDJSON_ASSERT(r.IsObject());
		r.AddMember(Value(StringRef(name), al), v.val(al), al);
		return *this;
	}

	LVal& add(const std::string& name, const string& v) {
		RAPIDJSON_ASSERT(r.IsObject());
		r.AddMember(Value(StringRef(name), al), Value(v,al), al);
		return *this;
	}

	//添加一个新的array节点，并返回此节点
	LVal<AlType> add_new_arr(const std::string& name) {
		RAPIDJSON_ASSERT(r.IsObject());
		r.AddMember(Value(StringRef(name), al), Value(kArrayType), al);
		return LVal<Document::AllocatorType>(r[name], al);
	}

	//添加一个新的object节点，并返回此节点
	LVal<AlType> add_new_obj(const std::string& name) {
		RAPIDJSON_ASSERT(r.IsObject());
		r.AddMember(Value(StringRef(name), al), Value(kObjectType), al);
		return LVal<Document::AllocatorType>(r[name], al);
	}

	LVal<AlType> operator[](const std::string& name) {
		RAPIDJSON_ASSERT(r.IsObject());
		return LVal<Document::AllocatorType>(r[name], al);
	}

	//for array

	LVal& set_arr() {
		r.SetArray();
		return *this;
	}

	LVal& set_arr(const list_initlist& ll) {
		r.SetArray();
		for (auto& i : ll) {
			r.PushBack(i.val(al), al);
		}
		return *this;
	}

	LVal& push(FVal&& v) {
		RAPIDJSON_ASSERT(r.IsArray());
		r.PushBack(v.val(al), al);
		return *this;
	}

	LVal& push(const string& v) {
		RAPIDJSON_ASSERT(r.IsArray());
		r.PushBack(Value(v,al), al);
		return *this;
	}

	LVal<AlType> push_new_obj() {
		RAPIDJSON_ASSERT(r.IsArray());
		r.PushBack(Value(kObjectType), al);
		return LVal<Document::AllocatorType>(r[r.Size()-1], al);
	}

	LVal<AlType> push_new_arr() {
		RAPIDJSON_ASSERT(r.IsArray());
		r.PushBack(Value(kArrayType), al);
		return LVal<Document::AllocatorType>(r[r.Size() - 1], al);
	}

	LVal<AlType> operator[](SizeType idx) {
		RAPIDJSON_ASSERT(r.IsArray());
		return LVal<Document::AllocatorType>(r[idx], al);
	}

	void set_null() {
		r.SetNull();
	}

	void set_val(const std::string& str) {
		r.SetString(str.data(), str.size(), al);
	}

	void set_val(const char* str) {
		r.SetString(str, al);
	}

	template<class T>
	void set_val(const T& v) {
		r.Set(v);
	}

	RVal rv() const {
		return RVal(r);
	}
};

/*
	Rjson document实例。
	所有其他操作都应源于document。
*/
class Doc{
	Document m_doc;
public:
	Doc() {}
	Doc(Doc&& d):m_doc(std::move(d.m_doc)) {  }
	Doc(FVal&& v) {
		auto& al = m_doc.GetAllocator();
		m_doc.CopyFrom(v.val(al), al, true);
			
	}
	template<class T>
	Doc(const T& v) {
		m_doc.Set(v);
	}

	static Doc loads(const char* str,size_t len=0) {
		Doc d;
		if(len==0) {
			d.m_doc.Parse<0>(str);
		}else {
			d.m_doc.Parse<0>(str, len);
		}
		RAPIDJSON_ASSERT(!d.m_doc.HasParseError());
		return d;
	}

	static Doc loads(const std::string& str) {
		Doc d;
		d.m_doc.Parse<0>(str);
		RAPIDJSON_ASSERT(!d.m_doc.HasParseError());
		return d;
	}

	std::string dumps() const {
		StringBuffer sb;
		Writer<StringBuffer> writer(sb); // PrettyWriter
		RAPIDJSON_ASSERT(m_doc.Accept(writer));
		return{ sb.GetString(), sb.GetSize() };
	}

	/*
		获取右值表达（用于读数据）
	*/
	RVal rv() const {
		return RVal(m_doc);
	}

	//获取左值表达（用于写数据）
	LVal<Document::AllocatorType> lv() {
		return LVal<Document::AllocatorType>(m_doc, m_doc.GetAllocator());
	}

};
}///END of namespace

#endif