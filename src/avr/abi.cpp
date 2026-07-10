
#define ABI_FUNC [[gnu::naked, gnu::used, gnu::retain]]

extern "C" [[gnu::naked]] int __mulhi3(int A, int B) {
    asm(R"(
        clr r16
        clr r0

        __mulhi_loop:
        
    )");
}
