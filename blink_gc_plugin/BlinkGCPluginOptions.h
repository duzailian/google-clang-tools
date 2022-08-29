// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_BLINK_GC_PLUGIN_BLINK_GC_PLUGIN_OPTIONS_H_
#define TOOLS_BLINK_GC_PLUGIN_BLINK_GC_PLUGIN_OPTIONS_H_

#include <set>
#include <string>
#include <vector>

struct BlinkGCPluginOptions {
  bool dump_graph = false;

  // Member<T> fields are only permitted in managed classes,
  // something CheckFieldsVisitor verifies, issuing errors if
  // found in unmanaged classes. WeakMember<T> should be treated
  // the exact same, but CheckFieldsVisitor was missing the case
  // for handling the weak member variant until crbug.com/724418.
  //
  // We've default-enabled the checking for those also now, but do
  // offer an opt-out option should enabling the check lead to
  // unexpected (but wanted, really) compilation errors while
  // rolling out an updated GC plugin version.
  //
  // TODO(sof): remove this option once safely rolled out.
  bool enable_weak_members_in_unmanaged_classes = false;

  // Persistent<T> fields are not allowed in garbage collected classes to avoid
  // memory leaks. Enabling this flag allows the plugin to check also for
  // Persistent<T> in types held by unique_ptr in garbage collected classes. The
  // guideline for this check is that a Persistent<T> should never be kept alive
  // by a garbage collected class, which unique_ptr clearly conveys.
  //
  // This check is disabled by default since there are currently non-ignored
  // violations of this rule in the code base, leading to compilation failures.
  // TODO(chromium:1283867): Enable this checks once all violations are handled.
  bool enable_persistent_in_unique_ptr_check = false;

  // On stack references to garbage collected objects should use raw pointers.
  // Although using Members/WeakMembers on stack is not strictly incorrect, it
  // is redundant and incurs additional costs that can mount up and become
  // significant. Enabling this flag lets the plugin to check for instances of
  // using Member/WeakMember on stack. These would include variable
  // declarations, method arguments and return types.
  //
  // This check is disabled by default since there currently are violations
  // of this rule in the code base, leading to compilation failures.
  // TODO(chromium:1283720): Enable this checks once all violations are handled.
  bool enable_members_on_stack_check = false;

  std::set<std::string> ignored_classes;
  std::set<std::string> checked_namespaces;
  std::vector<std::string> ignored_paths;
  // |allowed_paths| overrides |ignored_paths|.
  std::vector<std::string> allowed_paths;

  // For the default malloc, the following conditions are checked in addition
  // to the conditions above.
  std::set<std::string> checked_namespaces_for_default_malloc;
  std::vector<std::string> ignored_paths_for_default_malloc;
  std::vector<std::string> always_ignored_paths_for_default_malloc;
  // |allowed_paths_for_default_malloc| overrides
  // |ignore_paths_for_default_malloc|, but doesn't override
  // |always_ignored_paths_for_default_malloc|.
  std::vector<std::string> allowed_paths_for_default_malloc;
};

#endif  // TOOLS_BLINK_GC_PLUGIN_BLINK_GC_PLUGIN_OPTIONS_H_
