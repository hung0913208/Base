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
#include <Logcat.h>
#include <Utils.h>

namespace Base {
namespace Internal {
static Vector<Pair<ULong, Shared<Log>>> Loggers;

namespace LogN {
static UInt Loglevel{LOGLEVEL};
}  // namespace LogN
}  // namespace Internal

namespace holders = std::placeholders;

UInt& Log::Level(){ return Internal::LogN::Loglevel; }

Log& Log::Redirect(UInt level, Int device, Color color) {
  Shared<Log> result = None;
  Bool actived = Internal::LogN::Loglevel <= level;

  /* @NOTE: search device group now, it bases on the level */
  for (auto& item : Internal::Loggers) {
    if (item.Left == level) {
      result = item.Right;
    }
  }

  /* @NOTE: if the device group isn't existed, try to create the new one */
  if (!result) {
    result = std::make_shared<Log>(-1, color);

    Internal::Loggers.push_back(
        Pair<ULong, Shared<Log>>(level, result));
  } else {
    result->Apperance() = color;
  }

  /* @NOTE: active a device right now since Loglevel can be change at the
   * runtime */
  result->WithDevice(device).ActiveDevice(device, actived);
  return result->WithDevice(device);
}

Void Log::Enable(UInt level, Int device) {
  auto& logger = Log::Redirect(level, device);

  logger.Lock() = False;
  logger.ActiveDevice(device, True);
}

Void Log::Disable(UInt level, Int device) {
  auto& logger = Log::Redirect(level, device);

  logger.Lock() = True;
  logger.ActiveDevice(device, False);
}

Log::Log(Int device, Color color, Log* previous)
    : Stream{std::bind(&Log::WriteToDevice, this, holders::_1, holders::_2),
             None},
      _Previous{previous},
      _Status{True},
      _Lock{False},
      _Color{color},
      _Device{device}
{}

Log::~Log() {
  if (_Previous && _Device >= 0) _Previous->CloseDevice(_Device);

  for (auto logger : _Loggers) {
    CloseDevice(std::get<0>(logger));
  }
}

Bool& Log::Lock() { return _Lock; }

Color& Log::Apperance(){ return _Color; }

Log& Log::operator<<(const String& value) {
  if (_Writer && _Status) {
    auto error = _Writer((Bytes)value.c_str(), value.size());

    if (error != ENoError) throw Exception(error);
  }
  return *this;
}

Log& Log::operator<<(String&& value) {
  if (_Writer && _Status) {
    auto error = _Writer((Bytes)(value.c_str()), value.size());

    if (error != ENoError) throw Exception(error);
  }
  return *this;
}

Log& Log::operator<<(Color&& value) {
  if (_Status){
    if (_Device >= 0) {
      (*this) << value.Message();
    } else {
      value.Print();
    }
  }

  value.Clear();
  return *this;
}

Log& Log::operator<<(Color& value) {
  if (_Status){

    if (_Device >= 0) {
      (*this) << value.Message();
    } else {
      value.Print();
    }
  }

  value.Clear();
  return *this;
}

Log& Log::operator<<(Int&& value) { return (*this) << Base::ToString(value); }

Log& Log::operator<<(UInt&& value) { return (*this) << Base::ToString(value); }

Log& Log::operator<<(Float&& value) { return (*this) << Base::ToString(value); }

Log& Log::operator<<(Double&& value) {
  return (*this) << Base::ToString(value);
}

Log& Log::operator>>(String& UNUSED(value)) { throw Exception(ENoSupport); }

Log& Log::operator>>(Byte& UNUSED(value)) { throw Exception(ENoSupport); }

Log& Log::operator>>(Int& UNUSED(value)) { throw Exception(ENoSupport); }

Log& Log::operator>>(UInt& UNUSED(value)) { throw Exception(ENoSupport); }

Log& Log::operator>>(Float& UNUSED(value)) { throw Exception(ENoSupport); }

Log& Log::operator>>(Double& UNUSED(value)) { throw Exception(ENoSupport); }

Log& Log::WithDevice(Int device) {
  if (device == _Device) {
    return *this;
  } else if (_Loggers.find(device) == _Loggers.end()) {
    _Loggers[device] = new Log(device, _Color, this);
  }

  return *_Loggers[device];
}

ErrorCodeE Log::ActiveDevice(Int device, Bool status) {
  if (!AllowChangingStatus(device, status)) {
    return (BadAccess << Format{"dev {} is locked"}.Apply(device)).code();
  } else if (device < 0) {
    _Status = !_Lock && status;
  } else if (_Loggers.find(device) == _Loggers.end()) {
    return (BadAccess << Format{"dev {} didn\'t exist"}.Apply(device)).code();
  } else if (!_Loggers[device]->AllowChangingStatus(status)) {
    return (BadAccess << Format{"dev {} is locked"}.Apply(device)).code();
  } else {
    _Loggers[device]->_Status = !_Lock && status;
  }

  return ENoError;
}

ErrorCodeE Log::CloseDevice(Int device) {
  if (device == _Device) {
    _Device = -1;

    if (device != -1) {
      return BSCloseFileDescription(device);
    }
  } else if (_Loggers.find(device) != _Loggers.end()) {
    _Loggers.erase(device);
  } else {
    return (NotFound << Base::ToString(device)).code();
  }
  return ENoError;
}

Bool Log::AllowChangingStatus(Int UNUSED(device), Bool UNUSED(expected)) {
  return True;
}

Bool Log::AllowChangingStatus(Bool UNUSED(expected)) { return True; }

ErrorCodeE Log::WriteToColorConsole(Bytes&& buffer, UInt size) {
  _Color << String((char*)buffer, size);
  _Color.Print();
  return ENoError;
}

ErrorCodeE Log::WriteToDevice(Bytes&& buffer, UInt size) {
  if (_Device < 0) {
    if (_Color.Code() == Color::White || _Color.Code() == Color::Reset) {
      return Stream::WriteToConsole(RValue(buffer), size);
    } else {
      return WriteToColorConsole(RValue(buffer), size);
    }
  } else {
    auto writen = BSWriteToFileDescription(_Device, buffer, size);

    /* @NOTE: result of BSWriteToFileDescription must be size or an error_code.
     * When error happens, writen == -ErrorCodeE */
    if (writen < 0) return ErrorCodeE(-writen);
  }
  return ENoError;
}
}  // namespace Base
