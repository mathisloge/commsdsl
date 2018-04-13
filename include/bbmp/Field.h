#pragma once

#include <string>

#include "BbmpApi.h"
#include "Schema.h"

namespace bbmp
{

class FieldImpl;
class BBMP_API Field
{
public:

    enum class Kind
    {
        Int,
        Enum,
        Set,
        Float,
        Bitfield,
        Bundle,
        Ref,
        NumOfValues
    };

    explicit Field(const FieldImpl* impl);
    Field(const Field& other);
    ~Field();

    const std::string& name() const;
    const std::string& displayName() const;
    const std::string& description() const;
    Kind kind() const;
    std::size_t minLength() const;
    std::size_t maxLength() const;
    std::size_t bitLength() const;
    unsigned sinceVersion() const;
    unsigned deprecatedSince() const;
    bool isDeprecatedRemoved() const;

protected:
    const FieldImpl* m_pImpl;
};

} // namespace bbmp
