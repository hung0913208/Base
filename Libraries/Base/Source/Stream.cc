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

#include <Exception.h>
#include <Stream.h>
#include <cstring>
#include <iostream>

namespace Base {
Stream::Stream(Write writer, Read reader) : _Writer{writer}, _Reader{reader} {}

Stream::~Stream() {}

ErrorCodeE Stream::ReadBlock(Bytes buffer, UInt buffer_size) {
  try {
    return _Reader(buffer, &buffer_size);
  } catch (Exception& except) {
    return except.code();
  }
}

String Stream::Cache(){ throw Except(ENoSupport, ""); }

Stream& Stream::operator<<(String&& message) {
  if (_Writer) {
    auto error = _Writer((Bytes)message.data(), message.size());

    if (error != ENoError) throw Exception(error);
  }
  return *this;
}

Stream& Stream::operator<<(const String& message) {
  if (_Writer) {
    auto error = _Writer((Bytes)message.data(), message.size());

    if (error != ENoError) throw Exception(error);
  }
  return *this;
}

Stream& Stream::operator>>(String& message) {
  if (_Reader) {
    Bytes buffer = None;
    UInt size_of_received = 0;
    ErrorCodeE error = _Reader(buffer, &size_of_received);

    if (error == ENoError) {
      message.resize(size_of_received);
      memcpy((char*)(message.c_str()),
             buffer,
             size_of_received);

      free(buffer);
    } else {
      throw Exception(error);
    }
  }
  return *this;
}

Stream& Stream::operator<<(Byte&& value) {
  if (_Writer) {
    auto error = _Writer((Bytes)&value, sizeof(value));

    if (error != ENoError) throw Exception(error);
  }
  return *this;
}

Stream& Stream::operator<<(Int&& value) {
  if (_Writer) {
    auto error = _Writer((Bytes)&value, sizeof(value));

    if (error != ENoError) throw Exception(error);
  }
  return *this;
}

Stream& Stream::operator<<(UInt&& value) {
  if (_Writer) {
    auto error = _Writer((Bytes)&value, sizeof(value));

    if (error != ENoError) throw Exception(error);
  }
  return *this;
}

Stream& Stream::operator<<(Float&& value) {
  if (_Writer) {
    auto error = _Writer((Bytes)&value, sizeof(value));

    if (error != ENoError) throw Exception(error);
  }
  return *this;
}

Stream& Stream::operator<<(Double&& value) {
  if (_Writer) {
    auto error = _Writer((Bytes)&value, sizeof(value));

    if (error != ENoError) throw Exception(error);
  }
  return *this;
}

Stream& Stream::operator>>(Byte& value) {
  if (_Reader) {
    Bytes buffer = (Bytes)&value;
    UInt size_of_received = sizeof(value);
    ErrorCodeE error = _Reader(buffer, &size_of_received);

    if (error != ENoError) throw Exception(error);
  }
  return *this;
}

Stream& Stream::operator>>(Int& value) {
  if (_Reader) {
    Bytes buffer = (Bytes)&value;
    UInt size_of_received = sizeof(value);
    ErrorCodeE error = _Reader(buffer, &size_of_received);

    if (error != ENoError) throw Exception(error);
  }
  return *this;
}

Stream& Stream::operator>>(UInt& value) {
  if (_Reader) {
    Bytes buffer = (Bytes)&value;
    UInt size_of_received = sizeof(value);
    ErrorCodeE error = _Reader(buffer, &size_of_received);

    if (error != ENoError) throw Exception(error);
  }
  return *this;
}

Stream& Stream::operator>>(Float& value) {
  if (_Reader) {
    Bytes buffer = (Bytes)&value;
    UInt size_of_received = sizeof(value);
    ErrorCodeE error = _Reader(buffer, &size_of_received);

    if (error != ENoError) throw Exception(error);
  }
  return *this;
}

Stream& Stream::operator>>(Double& value) {
  if (_Reader) {
    Bytes buffer = (Bytes)&value;
    UInt size_of_received = sizeof(value);
    ErrorCodeE error = _Reader(buffer, &size_of_received);

    if (error != ENoError) throw Exception(error);
  }
  return *this;
}

ErrorCodeE Stream::WriteToConsole(Bytes&& buffer, UInt buffer_size) {
  std::cout << String((char*)buffer, buffer_size) << std::flush;
  return ENoError;
}

ErrorCodeE Stream::ReadFromConsole(Bytes& buffer, UInt* size_of_received) {
  if (buffer == None || *size_of_received == 0) {
    return ENoSupport;
  } else {
    char* tmp = (char*)buffer;

    for (UInt i = 0; i < *size_of_received; ++i) {
      std::cin >> tmp[i];
    }
    return ENoError;
  }
}
}  // namespace Base
