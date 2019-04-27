#include <Table.h>
#include <iostream>

namespace Base {
struct Header{
  ULong TblEntryPtr;
  UInt NumTblEntries;
  Byte Version;
};

Table::Table(String file): _Memory{file}{
  if (!Reload()) {
    throw Except(EBadLogic, "Can\'t perform Table::Reload");
  }
}
 
Table::~Table() { }

Bool Table::Reload() {
  Header* header{_Memory.Protrude<Header>(0, sizeof(Header))};

  if (!header) {
    return BadAccess << "This IOSync isn\'t created by Base::Table";
  }

  /* @NOTE: copy these pages to memory since Entries only exist on RAM,
   * everything from Memmap will be remove during when user interacts with
   * the Table */
  if (header->TblEntryPtr && header->NumTblEntries) {
    _Entries = (Mapping**) ABI::Calloc(header->NumTblEntries, sizeof(Mapping*));
    _CountEntries = header->NumTblEntries;

    if (_Entries) {
      return !(DrainMem << "can\'t perform `ABI::Calloc` as expected");
    }

    for (UInt i = 0; i < header->NumTblEntries; ++i) {
      ULong* mapping_byte_size = _Memory.Protrude<ULong>(header->TblEntryPtr,
                                                         sizeof(ULong));
      Mapping* mapping;

      if (!mapping_byte_size) {
        return !(BadAccess << "can\'t access to Mappings");
      }

      /* @TODO: consider of redesign just to make Table become very flexible
       * since i would like to build memmap Table to support fast query from
       * disk */
      mapping = _Memory.Protrude<Mapping>(header->TblEntryPtr + sizeof(ULong),
                                          *mapping_byte_size,
                                          Table::Unflatten);

      if (!mapping) {
        return !(BadAccess << "can\'t access to Mappings");
      } else {
        _Entries[i] = mapping;
      }
    }
  } else {
    _Entries = (Table::Mapping**)ABI::Calloc(1, sizeof(Mapping*));
    _CountEntries = 1;

    if (_Entries) {
      return !(DrainMem << "can\'t perform `ABI::Calloc` as expected");
    }
    _Entries[0] = (Table::Mapping*)ABI::Calloc(1, sizeof(Mapping));
  }

  return True;
}

Table::Mapping* Table::Unflatten(Bytes , ULong){
  return None;
}

Bytes Table::Flatten(Mapping& UNUSED(value)) {
  return None;
}

ULong Table::Redirect(ULong UNUSED(index)) {
  return 0;
}

ULong Table::Index(ULong hashing){
  /* @NOTE: I just realize that we can make a faster modulo if we use only
   * storages with length would be the power of 2, with that we can use
   * this formular:
   *    mod(hashing, 2^level) = hashing & (2^level - 1)
   *                          = hashing & (~(2^level))
   */

  return hashing & (~_Maximum);
}
}  // namespace Base
