#include <assert.h>
#include <memory>
#include <iostream>
#include <sstream>

#define UNUSED __attribute__((unused))

#define STRINGSTRM(code)                                 \
    ({                                                   \
        std::stringstream __string_strm;                 \
        __string_strm << code;                           \
        std::move(__string_strm);                        \
    })

#define STRING(code)                            \
    STRINGSTRM(code).str()

class Foo {
    int id_;
    int bar_;
    std::string track_;
    bool moved; // this object has been moved from (i.e. is null-like)
public:
    Foo(int bar);
    Foo();
    void destruct();
    ~Foo();
    int bar() {
        return id_;
    }
    Foo(Foo& other) noexcept;
    Foo(Foo&& other) noexcept;
#ifndef NO_COPY_VIA
    Foo& operator=(Foo& other) noexcept;
#endif
    Foo& operator=(Foo&& other) noexcept;
    bool operator==(Foo& other) noexcept;
};

int id_counter = 0;

Foo::Foo(int bar) {
    std::cout << "construct Foo " << id_ << " with bar = " << bar << "\n";
    bar_ = bar;
    id_ = id_counter++;
    moved = false;
}
Foo::Foo() {
    std::cout << "construct Foo " << id_ << " with no bar\n";
    bar_ = -1;
    id_ = id_counter++;
}

void Foo::destruct() {
    if (moved) {
        std::cout << "(destruct Foo " << id_ << " with bar = " << bar_ << " which saw: " << track_ << " -- moved)\n";
    } else {
        std::cout << "destruct Foo " << id_ << " with bar = " << bar_ << " which saw: " << track_ << "\n";
        // moved = true;  but destructor is called once only anyway (in theory)
    }
}

Foo::~Foo() {
    destruct();
}

// Requires `new_id` in scope; either copy it from the old obj, or from `id_counter++`.
#define EVENT_FROM(msg)                                                 \
    ({                                                                  \
        auto m = STRING(msg << " [" << other.id_ << " -> "              \
                        << new_id << "]");                              \
        std::cout << " { " << m << " }\n";                              \
                                                                        \
        id_ = new_id;                                                   \
        bar_ = other.bar_;                                              \
        track_ = other.track_;                                          \
        moved = false;                                                  \
                                                                        \
        if (track_.length()) {                                          \
            track_ += ", ";                                             \
        }                                                               \
        track_ += m;                                                    \
    })

// Also show object that is being overwritten, for `operator=`
#define EVENT_FROMTO(msg)                                               \
    ({                                                                  \
        auto m = STRING(msg << " [" << other.id_ << " -> "              \
                        << "(" << id_ << ")"                            \
                        << new_id << "]");                              \
        std::cout << " { " << m << " }\n";                              \
                                                                        \
        id_ = new_id;                                                   \
        bar_ = other.bar_;                                              \
        track_ = other.track_;                                          \
        moved = false;                                                  \
                                                                        \
        if (track_.length()) {                                          \
            track_ += ", ";                                             \
        }                                                               \
        track_ += m;                                                    \
    })

#define EVENT_PURE(msg)                                                 \
    ({                                                                  \
        auto m = STRING(msg << " (" << id_ << ", " << other.id_ << ")"); \
        std::cout << " { " << m << " }\n";                              \
        if (track_.length()) {                                          \
            track_ += ", ";                                             \
        }                                                               \
        track_ += m;                                                    \
    })


Foo::Foo(Foo& other) noexcept {
    auto new_id = id_counter++;
    EVENT_FROM("copy");
}

Foo::Foo(Foo&& other) noexcept {
    assert(! other.moved);
    auto new_id = other.id_;
    EVENT_FROM("move");
    other.moved = true;
}

#ifndef NO_COPY_VIA
Foo& Foo::operator=(Foo& other) noexcept {
    destruct();
    auto new_id = id_counter++;
    EVENT_FROMTO("copy=");
    return *this;
}
#endif

Foo& Foo::operator=(Foo&& other) noexcept {
    assert(! other.moved);
    // this->~Foo(); -- no good, since (as ASAN seems to indicate) the string is then freed twice
    destruct();
    auto new_id = other.id_;
    EVENT_FROMTO("move=");
    other.moved = true;
    return *this;
}

bool Foo::operator==(Foo& other) noexcept {
    EVENT_PURE("==");
    return id_ == other.id_;
}


int main() {
    // auto x = std::move(std::make_shared<Foo>(122));
    auto x = Foo { 122 };
    std::cout << "--- auto y = x\n";
    auto y = x; // { copy [0 -> 1] }
    std::cout << "--- y = x\n";
    // destruct Foo 1 with bar = 122 which saw: copy [0 -> 1]
    y = x;  // { copy= [0 -> (1)2] }

    auto x2 = x; // { copy [0 -> 3] }
    auto x3 = x; // { copy [0 -> 4] }
    std::cout << "--- y = std::move(x)\n";
    // destruct Foo 2 with bar = 122 which saw: copy= [0 -> (1)2]
    y = std::move(x); // { move= [0 -> (2)0] }

    auto z2 { std::move(x2) }; // { move [3 -> 3] }
    std::cout << z2.bar() << std::endl;

    if (getenv("TEST_MOVE_TWICE")) {
        std::cout << "--- auto move_twice = std::move(x)\n";
        auto move_twice = std::move(x);
    }
    std::cout << "--- auto z = std::move(x3)\n";
    auto z = std::move(x3); // { move [4 -> 4] }
    // std::cout << x->bar() << std::endl;
    auto z0 = z; // { copy [4 -> 5] }
    std::cout << z0.bar() << std::endl;
    std::cout << y.bar() << std::endl;
    std::cout << z.bar() << std::endl;

    std::cout << "z == z0: " << (z == z0) << std::endl; // { == (4, 5) }
    std::cout << "z == z: " << (z == z) << std::endl; // { == (4, 4) }

    // z0; // destruct Foo 5 with bar = 122 which saw: copy [0 -> 4], move [4 -> 4], copy [4 -> 5]
    // z; // <- x3 // destruct Foo 4 with bar = 122 which saw: copy [0 -> 4], move [4 -> 4], == (4, 5), == (4, 4)
    // z2; // <- x2 // destruct Foo 3 with bar = 122 which saw: copy [0 -> 3], move [3 -> 3]
    // x3; // (destruct Foo 4 with bar = 122 which saw: copy [0 -> 4] -- moved)
    // x2; // (destruct Foo 3 with bar = 122 which saw: copy [0 -> 3] -- moved)
    // y; // <- x // destruct Foo 0 with bar = 122 which saw: move= [0 -> (2)0]
    // x; // (destruct Foo 0 with bar = 122 which saw:  -- moved)
}
