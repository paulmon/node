/*
    Copyright(c) Microsoft Open Technologies, Inc. All rights reserved.

    The MIT License(MIT)

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files(the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions :

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

#include "logger_wrap.h"
#include "node_logger.h"

namespace node {
namespace logger {

  void NODE_API LogVerbose(const char* msg) {
    NODE_LOGGER_LOG(ILogger::LogLevel::Verbose, msg);
  }

  void NODE_API LogInfo(const char* msg) {
    NODE_LOGGER_LOG(ILogger::LogLevel::Info, msg);
  }

  void NODE_API LogWarn(const char* msg) {
    NODE_LOGGER_LOG(ILogger::LogLevel::Warn, msg);
  }

  void NODE_API LogError(const char* msg) {
    NODE_LOGGER_LOG(ILogger::LogLevel::Error, msg);
  }

}  // namespace logger
}  // namespace node