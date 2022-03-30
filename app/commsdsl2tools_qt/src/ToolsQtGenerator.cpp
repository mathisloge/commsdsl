//
// Copyright 2019 - 2022 (C). Alex Robenko. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ToolsQtGenerator.h"

#include "commsdsl/version.h"

#include "ToolsQtCmake.h"
#include "ToolsQtEnumField.h"
#include "ToolsQtIntField.h"
#include "ToolsQtSetField.h"

namespace commsdsl2tools_qt
{

const std::string& ToolsQtGenerator::fileGeneratedComment()
{
    static const std::string Str =
        "// Generated by commsdsl2tools_qt v" + std::to_string(commsdsl::versionMajor()) +
        '.' + std::to_string(commsdsl::versionMinor()) + '.' +
        std::to_string(commsdsl::versionPatch()) + '\n';
    return Str;
}

ToolsQtGenerator::FieldPtr ToolsQtGenerator::createIntFieldImpl(commsdsl::parse::Field dslObj, Elem* parent)
{
    return std::make_unique<commsdsl2tools_qt::ToolsQtIntField>(*this, dslObj, parent);
}

ToolsQtGenerator::FieldPtr ToolsQtGenerator::createEnumFieldImpl(commsdsl::parse::Field dslObj, Elem* parent)
{
    return std::make_unique<commsdsl2tools_qt::ToolsQtEnumField>(*this, dslObj, parent);
}

ToolsQtGenerator::FieldPtr ToolsQtGenerator::createSetFieldImpl(commsdsl::parse::Field dslObj, Elem* parent)
{
    return std::make_unique<commsdsl2tools_qt::ToolsQtSetField>(*this, dslObj, parent);
}

bool ToolsQtGenerator::writeImpl()
{
    return 
        ToolsQtCmake::write(*this);
}

} // namespace commsdsl2tools_qt
