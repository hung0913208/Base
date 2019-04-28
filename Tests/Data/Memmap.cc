#include <Unittest.h>
#include <Memmap.h>

TEST(Memmap, Blank) {
    Base::Memmap memmap("blank.map");
}

TEST(Memmap, Memory) {
    Base::Memmap memmap(100);
}

int main(){
    return RUN_ALL_TESTS();
}
