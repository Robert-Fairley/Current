/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2018 Dmitry "Dima" Korolev <dmitry.korolev@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

#ifndef EXAMPLES_FLOW_TOOL_SCHEMA_H
#define EXAMPLES_FLOW_TOOL_SCHEMA_H

#include "../../storage/storage.h"
#include "../../typesystem/struct.h"

namespace flow_tool {

// Storage
// =======

namespace db {

// `Blob` stores the contents single "file". It's versioned.
CURRENT_ENUM(blob_key_t, uint64_t){InvalidBlob = 0ull};
CURRENT_STRUCT(Blob) {
  CURRENT_FIELD(key, blob_key_t, blob_key_t::InvalidBlob);  // The key of this blob.
  CURRENT_FIELD(body, std::string);                         // The value of this blob.
};

// `Node` is an element of the hierarchical file system. Each node entry is a directory or a file.
CURRENT_ENUM(node_key_t, uint64_t){InvalidNode = 0ull, RootNode = static_cast<uint64_t>(-1)};
CURRENT_STRUCT(Dir) {
  CURRENT_FIELD(dir, std::vector<node_key_t>);  // The list of files in this directory.
};
CURRENT_STRUCT(File) {
  CURRENT_FIELD(blob, blob_key_t, blob_key_t::InvalidBlob);  // The reference to the respective blob.
};
CURRENT_STRUCT(Node) {
  CURRENT_FIELD(key, node_key_t, node_key_t::InvalidNode);  // The unique key of this node.
  CURRENT_FIELD(name, std::string);                         // The human-readable name of this file or dir, "" for root.
  CURRENT_FIELD(data, (Variant<Dir, File>));                // The contents of this node.
};

// The storage for the above is maintained by class `FlowTool`.
// It maintains some invariants, such as that the root is a directory and it can not be deleted.
CURRENT_STORAGE_FIELD_ENTRY(UnorderedDictionary, Blob, PersistedBlob);
CURRENT_STORAGE_FIELD_ENTRY(UnorderedDictionary, Node, PersistedNode);

CURRENT_STORAGE(FlowToolDB) {
  CURRENT_STORAGE_FIELD(blob, PersistedBlob);
  CURRENT_STORAGE_FIELD(node, PersistedNode);
};

}  // namespace flow_tool::db

// HTTP API
// ========

namespace api {

CURRENT_STRUCT(Healthz) {
  CURRENT_FIELD(up, bool, true);
  CURRENT_FIELD(uptime, std::string);
  CURRENT_FIELD(start_time_epoch_us, std::chrono::microseconds);
  CURRENT_FIELD(init_duration_us, std::chrono::microseconds);
  CURRENT_FIELD(uptime_us, std::chrono::microseconds);
};

CURRENT_STRUCT(Error) {
  CURRENT_FIELD(code, std::string);               // Some "CamelCase" error code.
  CURRENT_FIELD(message, Optional<std::string>);  // Optional human-readable description.
  CURRENT_CONSTRUCTOR(Error)
  (std::string code = "UnknownError") : code(std::move(code)) {}
  CURRENT_CONSTRUCTOR(Error)
  (std::string code, std::string message) : code(std::move(code)), message(std::move(message)) {}
};

CURRENT_STRUCT(ErrorPathNotFound, Error) {
  CURRENT_FIELD(error_component, std::string);
  CURRENT_FIELD(full_path, std::vector<std::string>);
  CURRENT_FIELD(index_in_full_path, uint32_t);
};

CURRENT_STRUCT(FileOrDirResponse) {
  CURRENT_FIELD(url, std::string);
  CURRENT_FIELD(path, std::vector<std::string>);
};

CURRENT_STRUCT(FileResponse, FileOrDirResponse) { CURRENT_FIELD(data, std::string); };

CURRENT_STRUCT(DirResponse, FileOrDirResponse) { CURRENT_FIELD(dir, std::vector<std::string>); };

}  // namespace flow_tool::api

}  // namespace flow_tool

#endif  // EXAMPLES_FLOW_TOOL_SCHEMA_H
