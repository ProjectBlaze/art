/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "os.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cstddef>
#include <memory>

#include <android-base/logging.h>

#include "unix_file/fd_file.h"

namespace art {

File* OS::OpenFileForReading(const char* name) {
  return OpenFileWithFlags(name, O_RDONLY);
}

File* OS::OpenFileReadWrite(const char* name) {
  return OpenFileWithFlags(name, O_RDWR);
}

static File* CreateEmptyFile(const char* name, int extra_flags) {
  // In case the file exists, unlink it so we get a new file. This is necessary as the previous
  // file may be in use and must not be changed.
  unlink(name);

  return OS::OpenFileWithFlags(name, O_CREAT | extra_flags);
}

File* OS::CreateEmptyFile(const char* name) {
  return art::CreateEmptyFile(name, O_RDWR | O_TRUNC);
}

File* OS::CreateEmptyFileWriteOnly(const char* name) {
#ifdef _WIN32
  int flags = O_WRONLY | O_TRUNC;
#else
  int flags = O_WRONLY | O_TRUNC | O_NOFOLLOW | O_CLOEXEC;
#endif
  return art::CreateEmptyFile(name, flags);
}

File* OS::OpenFileWithFlags(const char* name, int flags, bool auto_flush) {
  CHECK(name != nullptr);
  bool read_only = ((flags & O_ACCMODE) == O_RDONLY);
  bool check_usage = !read_only && auto_flush;
  std::unique_ptr<File> file(
      new File(name, flags,  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, check_usage));
  if (!file->IsOpened()) {
    return nullptr;
  }
  return file.release();
}

bool OS::FileExists(const char* name, bool check_file_type) {
  struct stat st;
  if (stat(name, &st) == 0) {
    if (check_file_type) {
      return S_ISREG(st.st_mode);  // TODO: Deal with symlinks?
    } else {
      return true;
    }
  } else {
    return false;
  }
}

bool OS::DirectoryExists(const char* name) {
  struct stat st;
  if (stat(name, &st) == 0) {
    return S_ISDIR(st.st_mode);  // TODO: Deal with symlinks?
  } else {
    return false;
  }
}

bool OS::CreateDirectory(const char* name, mode_t mode) {
  if (!DirectoryExists(name)) {
    if (mkdir(name, mode) == 0) {
      return true;
    }

    if (errno == ENOENT) {
      std::string path(name);
      std::string::size_type pos = path.find_last_of('/');
      if (pos != std::string::npos) {
        if (!CreateDirectory(path.substr(0, pos).c_str(), mode)) {
          return false;
        }

        return mkdir(name, mode) == 0;
      }
    }

    return false;
  }

  return true;
}

int64_t OS::GetFileSizeBytes(const char* name) {
  struct stat st;
  if (stat(name, &st) == 0) {
    return st.st_size;  // TODO: Deal with symlinks? According to the documentation,
                        // the st_size for a symlink is "the length of the pathname
                        // it contains, without a terminating null byte."
  } else {
    return -1;
  }
}

}  // namespace art
