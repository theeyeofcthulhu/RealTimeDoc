#pragma once

#ifndef NDEBUG
#define LOG(...) fmt::println("RTD: " __VA_ARGS__)
#define DBG(x) fmt::println("{}: {}", #x, (x))
#else
#define LOG(...) do {} while(0)
#define DBG(x) do {} while(0)
#endif
