// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/gyp_target_writer.h"

#include <iostream>

#include "base/file_util.h"
#include "base/files/file_path.h"
#include "tools/gn/build_settings.h"
#include "tools/gn/builder_record.h"
#include "tools/gn/filesystem_utils.h"
#include "tools/gn/gyp_binary_target_writer.h"
#include "tools/gn/gyp_script_target_writer.h"
#include "tools/gn/scheduler.h"
#include "tools/gn/settings.h"
#include "tools/gn/target.h"
#include "tools/gn/toolchain.h"

GypTargetWriter::TargetGroup::TargetGroup()
    : debug(NULL),
      release(NULL),
      host_debug(NULL),
      host_release(NULL),
      debug64(NULL),
      release64(NULL),
      xcode_debug(NULL),
      xcode_release(NULL),
      xcode_host_debug(NULL),
      xcode_host_release(NULL) {
}

GypTargetWriter::GypTargetWriter(const Target* target,
                                 const Toolchain* toolchain,
                                 const SourceDir& gyp_dir,
                                 std::ostream& out)
    : settings_(target->settings()),
      target_(target),
      toolchain_(toolchain),
      gyp_dir_(gyp_dir),
      out_(out),
      // All GYP paths are relative to GYP file.
      path_output_(gyp_dir, ESCAPE_JSON, false) {
}

GypTargetWriter::~GypTargetWriter() {
}

// static
void GypTargetWriter::WriteFile(const SourceFile& gyp_file,
                                const std::vector<TargetGroup>& targets,
                                const Toolchain* debug_toolchain,
                                Err* err) {
  if (targets.empty())
    return;
  const Settings* debug_settings =
      targets[0].debug->item()->AsTarget()->settings();
  const BuildSettings* debug_build_settings = debug_settings->build_settings();

  base::FilePath gyp_file_path = debug_build_settings->GetFullPath(gyp_file);
  base::CreateDirectory(gyp_file_path.DirName());

  std::stringstream file;
  file << "# Generated by GN. Do not edit.\n\n";
  file << "{\n";

  // Optional GYP header specified in the default debug toolchain.
  if (!debug_toolchain->gyp_header().empty())
    file << debug_toolchain->gyp_header() << std::endl;

  file << "  'skip_includes': 1,\n";
  file << "  'targets': [\n";

  for (size_t i = 0; i < targets.size(); i++) {
    const Target* cur = targets[i].debug->item()->AsTarget();
    switch (cur->output_type()) {
      case Target::COPY_FILES:
        break;  // TODO(brettw)
      case Target::CUSTOM: {
        GypScriptTargetWriter writer(targets[i], debug_toolchain,
                                     gyp_file.GetDir(), file);
        writer.Run();
        break;
      }
      case Target::GROUP:
        break;  // TODO(brettw)
      case Target::EXECUTABLE:
      case Target::STATIC_LIBRARY:
      case Target::SHARED_LIBRARY:
      case Target::SOURCE_SET: {
        GypBinaryTargetWriter writer(targets[i], debug_toolchain,
                                     gyp_file.GetDir(), file);
        writer.Run();
        break;
      }
      default:
        CHECK(0);
    }
  }

  file << "  ],\n}\n";

  std::string contents = file.str();
  file_util::WriteFile(gyp_file_path, contents.c_str(),
                       static_cast<int>(contents.size()));
}

std::ostream& GypTargetWriter::Indent(int spaces) {
  return Indent(out_, spaces);
}

// static
std::ostream& GypTargetWriter::Indent(std::ostream& out, int spaces) {
  const char kSpaces[81] =
      "                                        "
      "                                        ";
  CHECK(static_cast<size_t>(spaces) <= arraysize(kSpaces) - 1);
  out.write(kSpaces, spaces);
  return out;
}
