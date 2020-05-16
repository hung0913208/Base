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

#if !defined(BASE_STREAM_H_) && __cplusplus
#define BASE_STREAM_H_
#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Type.h>
#else
#include <Type.h>
#endif

namespace Base {
const String EOL = "\n";
const String TAB = "\t";

class Stream {
 public:
  using Write = Function<ErrorCodeE(Bytes&&, UInt)>;
  using Read = Function<ErrorCodeE(Bytes&, UInt*)>;

  explicit Stream(Write writer = Stream::WriteToConsole,
                  Read reader = Stream::ReadFromConsole);
  virtual ~Stream();

  ErrorCodeE ReadBlock(Bytes buffer, UInt buffer_size);

  /* @NOTE: caching data from stream to a privated storage */
  virtual String Cache();

  /* @NOTE: pull/push value from/to stream */
  virtual Stream& operator<<(String&& message);
  virtual Stream& operator<<(const String& message);
  virtual Stream& operator>>(String& message);

  /* @NOTE: push value thought to stream */
  virtual Stream& operator<<(Byte&& value);
  virtual Stream& operator<<(Int&& value);
  virtual Stream& operator<<(UInt&& value);
  virtual Stream& operator<<(Float&& value);
  virtual Stream& operator<<(Double&& value);

  /* @NOTE: pull value directly from stream */
  virtual Stream& operator>>(Byte& value);
  virtual Stream& operator>>(Int& value);
  virtual Stream& operator>>(UInt& value);
  virtual Stream& operator>>(Float& value);
  virtual Stream& operator>>(Double& value);

  static ErrorCodeE WriteToConsole(Bytes&& buffer, UInt buffer_size);
  static ErrorCodeE ReadFromConsole(Bytes& buffer, UInt* size_of_received);

 protected:
  Write _Writer;
  Read _Reader;
};
}  // namespace Base
#endif  // BASE_STREAM_H_
