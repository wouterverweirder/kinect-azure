
#include <napi.h>
#include <cstdlib>

#include <cstring>
#include <type_traits>

// header
class console
{
   public :
      static void init(const Napi::Env env, const Napi::Function emit);
      static void console::log(const uint64_t number);
      static void console::log(const std::string& value);
      static void console::log(const std::u16string& value);
      static void console::log(const char* value);
      static void console::log(const char16_t* value);
      static void console::log(const char* value, size_t length);
      static void console::log(const char16_t* value, size_t length);
      static Napi::Env env;
      static Napi::FunctionReference g_emit;
} ;

Napi::Env console::env = NULL;
Napi::FunctionReference console::g_emit;

void console::init(const Napi::Env env, const Napi::Function emit)
{
   console::env = env;
   console::g_emit = Napi::Persistent(emit);
}

void console::log(const uint64_t value) {
  console::g_emit.Call({Napi::String::New(console::env, "log"), Napi::String::New(console::env, std::to_string(value))});
}

void console::log(const std::string& value) {
  console::g_emit.Call({Napi::String::New(console::env, "log"), Napi::String::New(console::env, value)});
}

void console::log(const std::u16string& value) {
  console::g_emit.Call({Napi::String::New(console::env, "log"), Napi::String::New(console::env, value)});
}

void console::log(const char* value) {
  console::g_emit.Call({Napi::String::New(console::env, "log"), Napi::String::New(console::env, value)});
}

void console::log(const char16_t* value) {
  console::g_emit.Call({Napi::String::New(console::env, "log"), Napi::String::New(console::env, value)});
}

void console::log(const char* value, size_t length) {
  console::g_emit.Call({Napi::String::New(console::env, "log"), Napi::String::New(console::env, value, length)});
}

void console::log(const char16_t* value, size_t length) {
  console::g_emit.Call({Napi::String::New(console::env, "log"), Napi::String::New(console::env, value, length)});
}