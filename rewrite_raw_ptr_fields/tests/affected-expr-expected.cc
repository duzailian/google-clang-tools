// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>  // for uintptr_t

#include "base/memory/checked_ptr.h"

class SomeClass {};
class DerivedClass : public SomeClass {};

struct MyStruct {
  CheckedPtr<SomeClass> ptr;
  CheckedPtr<SomeClass> ptr2;
  CheckedPtr<const SomeClass> const_ptr;
};

namespace printf_tests {

int ConvertSomeClassToInt(SomeClass* some_class) {
  return 123;
}

void MyPrintf(const char* fmt, ...) {}

void foo() {
  MyStruct s;

  // Expected rewrite: MyPrintf("%p", s.ptr.get());
  MyPrintf("%p", s.ptr.get());

  // Test - all arguments are rewritten.
  // Expected rewrite: MyPrintf("%p, %p", s.ptr.get(), s.ptr2.get());
  MyPrintf("%p, %p", s.ptr.get(), s.ptr2.get());

  // Test - only |s.ptr|-style arguments are rewritten.
  // Expected rewrite: MyPrintf("%d, %p", 123, s.ptr.get());
  MyPrintf("%d, %p", 123, s.ptr.get());

  // Test - |s.ptr| is deeply nested.
  // No rewrite expected.
  MyPrintf("%d", ConvertSomeClassToInt(s.ptr));
}

}  // namespace printf_tests

namespace cast_tests {

void foo() {
  MyStruct my_struct;

  // To get |const_cast<...>(...)| to compile after the rewrite we
  // need to rewrite the casted expression.
  // Expected rewrite: const_cast<SomeClass*>(my_struct.const_ptr.get());
  SomeClass* v = const_cast<SomeClass*>(my_struct.const_ptr.get());
  // Expected rewrite: const_cast<const SomeClass*>(my_struct.ptr.get());
  const SomeClass* v2 = const_cast<const SomeClass*>(my_struct.ptr.get());

  // To get |reinterpret_cast<uintptr_t>(...)| to compile after the rewrite we
  // need to rewrite the casted expression.
  // Expected rewrite: reinterpret_cast<uintptr_t>(my_struct.ptr.get());
  uintptr_t u = reinterpret_cast<uintptr_t>(my_struct.ptr.get());

  // There is no need to append |.get()| inside static_cast - unlike the
  // const_cast and reinterpret_cast examples above, static_cast will compile
  // just fine.
  DerivedClass* d = static_cast<DerivedClass*>(my_struct.ptr);
  void* void_var = static_cast<void*>(my_struct.ptr);
}

}  // namespace cast_tests
