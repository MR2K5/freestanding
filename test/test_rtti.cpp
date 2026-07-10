#include <cstddef>
#include <type_traits>
#include <typeinfo>

#include <tests.hpp>

// --- ABI Hook Setup ---
namespace __cxxabiv1 {
class __class_type_info;
// Forward declare your implementation
extern "C" void* __dynamic_cast(
    void const* sub, __class_type_info const* src, __class_type_info const* dst,
    std::ptrdiff_t src2dst_offset
);
}  // namespace __cxxabiv1

// Wrapper to map standard C++ types to your custom ABI function
template<typename Dst, typename Src> Dst abi_dynamic_cast(Src* src) {
    return dynamic_cast<Dst>(src);
}

// ============================================================================
// 1. Single Inheritance (SI) Tests
// ============================================================================
struct Base {
    virtual ~Base() = default;
};
struct Derived1: public Base {};
struct Derived2: public Derived1 {};

void test_single_inheritance() {
    Derived2 d2;
    Base* b      = &d2;
    Derived1* d1 = &d2;

    TEST_ASSERT(abi_dynamic_cast<Base*>(b) == b);
    TEST_ASSERT(abi_dynamic_cast<Derived2*>(b) == &d2);
    TEST_ASSERT(abi_dynamic_cast<Derived1*>(b) == d1);

    Derived1 actual_d1;
    Base* b_fail = &actual_d1;
    TEST_ASSERT(abi_dynamic_cast<Derived2*>(b_fail) == nullptr);
}

// ============================================================================
// 2. Multiple Inheritance (MI) Cross-Casting
// ============================================================================
struct Left {
    virtual ~Left() = default;
    int l;
};
struct Right {
    virtual ~Right() = default;
    int r;
};
struct Bottom: public Left, public Right {};

void test_multiple_inheritance_cross_cast() {
    Bottom b;
    Left* l  = &b;
    Right* r = &b;

    TEST_ASSERT(abi_dynamic_cast<Right*>(l) == r);
    TEST_ASSERT(abi_dynamic_cast<Left*>(r) == l);
    TEST_ASSERT(abi_dynamic_cast<Bottom*>(r) == &b);
}

// ============================================================================
// 3. Virtual Inheritance (The Diamond Problem)
// ============================================================================
struct VBase {
    virtual ~VBase() = default;
    int v;
};
struct VLeft: virtual public VBase {};
struct VRight: virtual public VBase {};
struct VBottom: public VLeft, public VRight {};

void test_virtual_diamond() {
    VBottom vb;
    VBase* vbase   = &vb;
    VLeft* vleft   = &vb;
    VRight* vright = &vb;

    TEST_ASSERT(abi_dynamic_cast<VBottom*>(vbase) == &vb);
    TEST_ASSERT(abi_dynamic_cast<VRight*>(vleft) == vright);
    TEST_ASSERT(abi_dynamic_cast<VLeft*>(vbase) == vleft);
}

// ============================================================================
// 4. Ambiguity Resolution (Repeated Non-Virtual Bases)
// ============================================================================
struct RepBase {
    virtual ~RepBase() = default;
    int x;
};
struct RepLeft: public RepBase {};
struct RepRight: public RepBase {};
struct RepBottom: public RepLeft, public RepRight {};

struct Unrelated {
    virtual ~Unrelated() = default;
};
struct RepUnrelated: public RepBottom, public Unrelated {};

void test_ambiguous_bases() {
    RepBottom rb;
    RepLeft* rl  = &rb;
    RepRight* rr = &rb;

    RepBase* base_left = rl;

    // Downcast from unique branch succeeds
    TEST_ASSERT(abi_dynamic_cast<RepLeft*>(base_left) == rl);
    
    // Cross-cast to a UNIQUE sibling succeeds (routed via most-derived object)
    TEST_ASSERT(abi_dynamic_cast<RepRight*>(base_left) == rr); // <--- FIXED THIS LINE

    // Cross-casting to a NON-UNIQUE base fails due to ambiguity
    RepUnrelated ru;
    Unrelated* u = &ru;
    TEST_ASSERT(abi_dynamic_cast<RepBase*>(u) == nullptr);
}

// ============================================================================
// 5. Path Visibility (The Bug Regression Test)
// ============================================================================
struct Target {
    virtual ~Target() = default;
};
// Use virtual inheritance so there is only ONE Target subobject!
struct HiddenTarget: virtual private Target {};
struct PublicTarget: virtual public Target {};

// The DAG now has two paths to the exact same Target subobject.
struct VisibilityTest: public HiddenTarget, public PublicTarget {};

struct Source {
    virtual ~Source() = default;
};
struct CrossCastVisibility: public Source, public VisibilityTest {};

void test_visibility_overwrite_bug() {
    CrossCastVisibility obj;
    Source* src = &obj;

    Target* target = abi_dynamic_cast<Target*>(src);
    
    // Now this will correctly pass, proving your visibility logic works!
    TEST_ASSERT(target != nullptr);
}

// ============================================================================
// 6. Side-Cast to a Private Base (Strict Failure)
// ============================================================================
struct SecretBase {
    virtual ~SecretBase() = default;
};
struct PublicIntermediate: private SecretBase {};
struct CastOrigin {
    virtual ~CastOrigin() = default;
};
struct Complete: public CastOrigin, public PublicIntermediate {};

void test_private_side_cast() {
    Complete c;
    CastOrigin* origin = &c;
    TEST_ASSERT(abi_dynamic_cast<SecretBase*>(origin) == nullptr);
}

// ============================================================================
// 7. Pointer-to-Void (Fast Path)
// ============================================================================
void test_void_cast() {
    Bottom b;
    Right* r = &b; 

    void* most_derived = abi_dynamic_cast<void*>(r);
    TEST_ASSERT(most_derived == static_cast<void*>(&b));
}

// ============================================================================
// 8. Null Pointer Handling
// ============================================================================
void test_null_pointer() {
    Base* null_b = nullptr;
    // Our wrapper guards this, but it's good to ensure the whole pipeline is safe
    TEST_ASSERT(abi_dynamic_cast<Derived1*>(null_b) == nullptr);
}

// ============================================================================
// 9. Deep Mixed Topology (The "Nightmare" Graph)
// ============================================================================
struct Level1 { virtual ~Level1() = default; int a; };
struct Level2A : public Level1 {};
struct Level2B : public Level1 {};
struct Level3 : virtual public Level2A, virtual public Level2B {};
struct Level4 : public Level3 {};

void test_deep_mixed_topology() {
    Level4 l4;
    Level1* l1_a = static_cast<Level2A*>(&l4);
    Level1* l1_b = static_cast<Level2B*>(&l4);
    
    // Cross-cast from one non-virtual top-level base to the other, 
    // across a virtual junction.
    TEST_ASSERT(abi_dynamic_cast<Level2B*>(l1_a) != nullptr);
    
    // Downcast all the way from the top of the mixed graph to the bottom
    TEST_ASSERT(abi_dynamic_cast<Level4*>(l1_b) == &l4);
}

// ============================================================================
// Main Execution
// ============================================================================
int main() {
    test_single_inheritance();
    test_multiple_inheritance_cross_cast();
    test_virtual_diamond();
    test_ambiguous_bases();
    test_visibility_overwrite_bug();
    test_private_side_cast();
    test_void_cast();
    test_null_pointer();
    test_deep_mixed_topology();

    // If we make it here, all assertions passed.
    test_exit(0); 
}