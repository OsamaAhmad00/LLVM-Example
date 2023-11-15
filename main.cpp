#include <ModuleCompiler.h>

int main()
{
    ModuleCompiler module(
        "MainModule", ""
        "int printf(i8* message, ...);"
        ""
        "int print(int num) = { printf(\"%d\n\", num); 0 };"
        ""
        "int main() {         "
        "    int x = 123;     "
        "    print(x += 100); "
        "    print(x);        "
        "    print(x -= 100); "
        "    print(x);        "
        "    print(x *= 100); "
        "    print(x);        "
        "    print(x /= 100); "
        "    print(x);        "
        "    print(x %= 100); "
        "    print(x);        "
        "    print(x <<= 5);  "
        "    print(x);        "
        "    print(x >>= 1);  "
        "    print(x);        "
        "    print(x >>>= 2); "
        "    print(x);        "
        "    print(x &= 100); "
        "    print(x);        "
        "    print(x ^= 100); "
        "    print(x);        "
        "    print(x |= 100); "
        "    print(x);        "
        "    x = -x;          "
        "    print(x >>>= 2); "
        "    print(x);        "
        "    return 0;        "
        "}                    "
    );
}