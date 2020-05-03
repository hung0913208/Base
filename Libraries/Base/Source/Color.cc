// Copyright (c) 2018,  All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.

// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.

// 3. Neither the name of ORGANIZATION nor the names of
//    its contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <Logcat.h>
#include <Utils.h>
#include <stdio.h>

namespace Base {
#if USE_COLOR
Color::Color(Color::ColorCodeE code)
    : Message{[&]() -> String& { return _Message; },
              [](String) { throw Exception(ENoSupport); }},
      _Code{code}, _Printed{False} {}
#else
Color::Color(Color::ColorCodeE)
    : Message{[&]() -> String& { return _Message; },
              [](String) { throw Exception(ENoSupport); }},
      _Code{Color::Reset}, _Printed{False} {}
#endif

Color::~Color() {
  if (_Message.size() > 0) {
    Print();
  }
}

Color& Color::operator()(String& message) {
  Print(std::move(message));
  return *this;
}

Color& Color::operator()(String&& message) {
  Print(RValue(message));
  return *this;
}

Color& Color::operator<<(String& message) {
  _Message.append(message);
  _Printed = False;
  return *this;
}

Color& Color::operator<<(String&& message) {
  _Message.append(message);
  _Printed = False;

  return *this;
}

Color& Color::operator>>(String& message) {
  message = _Message;
  return *this;
}

Color::ColorCodeE& Color::Code(){ return _Code; }

void Color::Clear(){
  _Message = "";
}

void Color::Print(){
  Print(RValue(_Message));
  Clear();
}

void Color::Print(String&& message) {
  if (message.size() > 0 && !_Printed) {
#if USE_COLOR
    fprintf(stdout, "\x1B[%lum%s\x1B[0m", (long unsigned int)(_Code), message.c_str());
#else
    fprintf(stdout, "%s", message.c_str());
#endif
  }
    
  _Printed = True;

}
}  // namespace Base
