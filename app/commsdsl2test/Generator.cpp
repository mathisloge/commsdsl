#include "Generator.h"

#include "commsdsl/version.h"

#include "Test.h"
#include "Cmake.h"

namespace commsdsl2test
{

const std::string& Generator::fileGeneratedComment()
{
    static const std::string Str =
        "// Generated by commsdsl2test v" + std::to_string(commsdsl::versionMajor()) +
        '.' + std::to_string(commsdsl::versionMinor()) + '.' +
        std::to_string(commsdsl::versionPatch()) + '\n';
    return Str;
}

bool Generator::writeImpl()
{
    return 
        Test::write(*this) &&
        Cmake::write(*this);
}

} // namespace commsdsl2test
