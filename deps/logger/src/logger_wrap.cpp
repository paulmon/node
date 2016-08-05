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

#include "node.h"
#include "logger_wrap.h"

namespace node { 
namespace logger {

  using namespace v8;

  static const ILogger* s_logger = nullptr;

  void SetLogger(const ILogger* logger) {
    s_logger = logger;
  }

  const ILogger* GetLogger() {
    return s_logger;
  }

  void Log(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    if (s_logger == nullptr) {
      return;
    }

    if (args.Length() < 2 || ! args[0]->IsNumber()) {
      isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Invalid arguments, expected: [log level : number], [message : string]")));
      return;
    }

    // get log level
    ILogger::LogLevel level = static_cast<ILogger::LogLevel>(args[0]->Int32Value());

    String::Utf8Value msgPtr(args[1]);
    s_logger->Log(level, *msgPtr);
  }
  
  void Initialize(Handle<Object> target) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    Handle<Object> loggerObject = Object::New();
    loggerObject->Set(String::NewFromUtf8(isolate, "log"), FunctionTemplate::New(isolate, Log)->GetFunction());

    // init log level enum: Verbose, Info, Warn, Error
    Handle<Object> logLevelsObj = Object::New();
    logLevelsObj->Set(String::NewFromUtf8(isolate, "verbose"), Integer::New(isolate, static_cast<int>(ILogger::LogLevel::Verbose)));
    logLevelsObj->Set(String::NewFromUtf8(isolate, "info"), Integer::New(isolate, static_cast<int>(ILogger::LogLevel::Info)));
    logLevelsObj->Set(String::NewFromUtf8(isolate, "warn"), Integer::New(isolate, static_cast<int>(ILogger::LogLevel::Warn)));
    logLevelsObj->Set(String::NewFromUtf8(isolate, "error"), Integer::New(isolate, static_cast<int>(ILogger::LogLevel::Error)));

    loggerObject->Set(String::NewFromUtf8(isolate, "logLevels"), logLevelsObj);

    target->Set(String::NewFromUtf8(isolate, "logger"), loggerObject);
  }

}  // namespace logger
}  // namespace node

NODE_MODULE_CONTEXT_AWARE_BUILTIN(logger_wrap, node::logger::Initialize)