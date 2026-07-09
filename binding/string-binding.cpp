/*
** system.cpp
**
** This file is part of HiddenChest
**
** Copyright (C) 2018-2026 Kyonides-Arkanthes
*/

#include "picosha2.h"
#include "hcextras.h"

static VALUE string_sha256_hash(VALUE self)
{
  const char *fn = RSTRING_PTR(self);
  std::string filename = fn;
  std::string checksum = picosha2::hash256_hex_string(filename);
  const char *sha = checksum.c_str();
  return rstr(sha);
}

void init_string_sha256()
{
  rb_define_method(rb_cString, "hash256", RMF(string_sha256_hash), 0);
}