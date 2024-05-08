#pragma once
#define PARENS ()

#define EXPAND256(...) EXPAND64(EXPAND64(EXPAND64(EXPAND64(__VA_ARGS__))))
#define EXPAND64(...) EXPAND16(EXPAND16(EXPAND16(EXPAND16(__VA_ARGS__))))
#define EXPAND16(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
#define EXPAND4(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) __VA_ARGS__

#define FOR_EACH(macro, ...)                                                   \
  __VA_OPT__(EXPAND16(FOR_EACH_HELPER(macro, __VA_ARGS__)))
#define FOR_EACH_HELPER(macro, a1, ...)                                        \
  macro(a1) __VA_OPT__(FOR_EACH_AGAIN PARENS(macro, __VA_ARGS__))
#define FOR_EACH_AGAIN() FOR_EACH_HELPER

#define THROWS_WRAPPER(Err) , static_cast<Err*>(nullptr)
#define THROWS(...)                                                            \
  clang::annotate("Throws" FOR_EACH(THROWS_WRAPPER, __VA_ARGS__))

#define ALWAYS_TRACK clang::annotate("Explicit Error")
