

int volatile write = 0;

struct A {
    ~A() { write = 1; }
};

extern "C" [[gnu::used, gnu::retain]] int main() {
    try {
        {
            A a;
            throw 7;
        }
    } catch (...) {}
    return 0;
}
