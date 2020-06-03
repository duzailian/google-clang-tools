// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

class SomeClass;

struct MyStruct {
  SomeClass* ptr;
  SomeClass* ptr2;
};

int ConvertSomeClassToInt(SomeClass* some_class) {
  return 123;
}

void MyPrintf(const char* fmt, ...) {}

void foo() {
  MyStruct s;

  // Expected rewrite: MyPrintf("%p", s.ptr.get());
  MyPrintf("%p", s.ptr);

  // Test - all arguments are rewritten.
  // Expected rewrite: MyPrintf("%p, %p", s.ptr.get(), s.ptr2.get());
  MyPrintf("%p, %p", s.ptr, s.ptr2);

  // Test - only |s.ptr|-style arguments are rewritten.
  // Expected rewrite: MyPrintf("%d, %p", 123, s.ptr.get());
  MyPrintf("%d, %p", 123, s.ptr);

  // Test - |s.ptr| is deeply nested.
  // No rewrite expected.
  MyPrintf("%d", ConvertSomeClassToInt(s.ptr));
}
