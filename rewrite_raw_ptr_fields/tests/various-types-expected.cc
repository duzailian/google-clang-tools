// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/checked_ptr.h"

class SomeClass {
 public:
  void Method(char) {}
  int data_member;
};

struct MyStruct {
  // Expected rewrite: CheckedPtr<CheckedPtr<SomeClass>> double_ptr;
  // TODO(lukasza): Handle recursion/nesting.
  CheckedPtr<SomeClass*> double_ptr;

  // Expected rewrite: CheckedPtr<void> void_ptr;
  CheckedPtr<void> void_ptr;

  // No rewrite of function pointers expected, because they won't ever be either
  // A) allocated by PartitionAlloc or B) derived from CheckedPtrSupport.  In
  // theory |member_data_ptr| below can be A or B, but it can't be expressed as
  // non-pointer T used as a template argument of CheckedPtr.
  int (*func_ptr)();
  void (SomeClass::*member_func_ptr)(char);  // ~ pointer to SomeClass::Method
  int SomeClass::*member_data_ptr;  // ~ pointer to SomeClass::data_member
  typedef void (*func_ptr_typedef)(char);
  func_ptr_typedef func_ptr_typedef_field;

  // Typedef-ed or type-aliased pointees should participate in the rewriting. No
  // desugaring of the aliases is expected.
  typedef SomeClass SomeClassTypedef;
  using SomeClassAlias = SomeClass;
  typedef void (*func_ptr_typedef2)(char);
  // Expected rewrite: CheckedPtr<SomeClassTypedef> ...
  // TODO(lukasza): No |MyStruct::| qualification expected.
  CheckedPtr<MyStruct::SomeClassTypedef> typedef_ptr;
  // Expected rewrite: CheckedPtr<SomeClassAlias> ...
  // TODO(lukasza): No |MyStruct::| qualification expected.
  CheckedPtr<MyStruct::SomeClassAlias> alias_ptr;
  // Expected rewrite: CheckedPtr<func_ptr_typedef2> ...
  // TODO(lukasza): No |MyStruct::| qualification expected.
  CheckedPtr<MyStruct::func_ptr_typedef2> ptr_to_function_ptr;

  // Typedefs and type alias definitions should not be rewritten.
  //
  // No rewrite expected (for now - in V1 we only rewrite field decls).
  typedef SomeClass* SomeClassPtrTypedef;
  // No rewrite expected (for now - in V1 we only rewrite field decls).
  using SomeClassPtrAlias = SomeClass*;
};
