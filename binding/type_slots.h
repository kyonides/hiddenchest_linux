
#define RUBY_VERSION 34

#if RUBY_VERSION == 27
#define type_slots 0, 0, { 0 }
#elif RUBY_VERSION >= 30
#define type_slots 0, 0, 0
#endif