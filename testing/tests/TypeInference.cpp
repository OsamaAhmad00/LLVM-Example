#include "FileTestCasesRunner.hpp"

namespace dua
{

TEST(type_inference, type_inference) {
    FileTestCasesRunner("type-inference.dua").run();
}

}
