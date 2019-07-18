#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include "NixRenderer.h"
#include <map>
#include <typeinfo>
#include <string>
#include <typeindex>

#define KS_JSON_ENUM_TABLE std::map< size_t, std::map< std::string, uint32_t > > NixJsonEnumTable = {
#define KS_JSON_ENUM_TYPE( Type ) { typeid(Type).hash_code(), {
#define KS_JSON_ENUM_TYPE_END } },
#define KS_JSON_ENUM_TABLE_END };
#define KS_JSON_ENUM_VALUE( Value ) { #Value, (uint32_t)Value },

#include "NixJpEnum.txt"