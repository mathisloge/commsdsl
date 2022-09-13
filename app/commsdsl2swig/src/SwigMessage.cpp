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

#include "SwigMessage.h"

#include "SwigGenerator.h"
#include "SwigInterface.h"
#include "SwigMsgId.h"

#include "commsdsl/gen/comms.h"
#include "commsdsl/gen/strings.h"
#include "commsdsl/gen/util.h"

#include <cassert>
#include <fstream>

namespace comms = commsdsl::gen::comms;
namespace strings = commsdsl::gen::strings;
namespace util = commsdsl::gen::util;

namespace commsdsl2swig
{


SwigMessage::SwigMessage(SwigGenerator& generator, commsdsl::parse::Message dslObj, Elem* parent) :
    Base(generator, dslObj, parent)
{
}   

SwigMessage::~SwigMessage() = default;

void SwigMessage::swigAddCodeIncludes(StringsList& list) const
{
    list.push_back(comms::relHeaderPathFor(*this, generator()));
}

void SwigMessage::swigAddCode(StringsList& list) const
{
    for (auto* f : m_swigFields) {
        f->swigAddCode(list);
    }

    auto& gen = SwigGenerator::cast(generator());
    auto* mainInterface = gen.swigMainInterface();
    assert(mainInterface != nullptr);

    util::ReplacementMap repl = {
        {"COMMS_CLASS", comms::scopeFor(*this, gen)},
        {"CLASS_NAME", gen.swigClassName(*this)},
        {"INTERFACE", gen.swigClassName(*mainInterface)}
    };


    std::string publicCode = util::readFileContents(gen.swigInputCodePathFor(*this) + strings::publicFileSuffixStr());
    std::string protectedCode = util::readFileContents(gen.swigInputCodePathFor(*this) + strings::protectedFileSuffixStr());
    std::string privateCode = util::readFileContents(gen.swigInputCodePathFor(*this) + strings::privateFileSuffixStr());

    if (publicCode.empty() && protectedCode.empty() && privateCode.empty() && (m_swigFields.empty())) {
        static const std::string Templ = 
            "class #^#CLASS_NAME#$# : public #^#COMMS_CLASS#$#<#^#INTERFACE#$#> {};\n";

        list.push_back(util::processTemplate(Templ, repl));
        return;
    }

    if (!protectedCode.empty()) {
        static const std::string TemplTmp = 
            "protected:\n"
            "    #^#CODE#$#\n";

        util::ReplacementMap replTmp = {
            {"CODE", std::move(protectedCode)}
        };

        protectedCode = util::processTemplate(TemplTmp, replTmp);
    }

    if (!privateCode.empty()) {
        static const std::string TemplTmp = 
            "private:\n"
            "    #^#CODE#$#\n";

        util::ReplacementMap replTmp = {
            {"CODE", std::move(privateCode)}
        };

        privateCode = util::processTemplate(TemplTmp, replTmp);
    }    

    static const std::string Templ = 
        "class #^#CLASS_NAME#$# : public #^#COMMS_CLASS#$#<#^#INTERFACE#$#>\n"
        "{\n"
        "    using Base = #^#COMMS_CLASS#$#<#^#INTERFACE#$#>;\n"
        "public:\n"
        "    #^#FIELDS#$#\n"
        "    #^#PUBLIC#$#\n"
        "#^#PROTECTED#$#\n"
        "#^#PRIVATE#$#\n"
        "};\n";

    repl.insert({
        {"FIELDS", swigFieldsAccCodeInternal()},
        {"PUBLIC", std::move(publicCode)},
        {"PROTECTED", std::move(protectedCode)},
        {"PRIVATE", std::move(privateCode)}
    });

    list.push_back(util::processTemplate(Templ, repl));    
}

void SwigMessage::swigAddDef(StringsList& list) const
{
    for (auto* f : m_swigFields) {
        f->swigAddDef(list);
    }

    list.push_back(SwigGenerator::swigDefInclude(comms::relHeaderPathFor(*this, generator())));
}

bool SwigMessage::prepareImpl()
{
    if (!Base::prepareImpl()) {
        return false;
    }

    m_swigFields = SwigField::swigTransformFieldsList(fields());
    return true;
}

bool SwigMessage::writeImpl() const
{
    auto filePath = comms::headerPathFor(*this, generator());
    auto dirPath = util::pathUp(filePath);
    assert(!dirPath.empty());
    if (!generator().createDirectory(dirPath)) {
        return false;
    }       

    auto& logger = generator().logger();
    logger.info("Generating " + filePath);

    std::ofstream stream(filePath);
    if (!stream) {
        logger.error("Failed to open \"" + filePath + "\" for writing.");
        return false;
    }

    static const std::string Templ = 
        "#^#GENERATED#$#\n"
        "#pragma once\n\n"
        "#^#FIELDS#$#\n"
        "#^#DEF#$#\n"
    ;

    util::ReplacementMap repl = {
        {"GENERATED", SwigGenerator::fileGeneratedComment()},
        {"FIELDS", swigFieldDefsInternal()},
        {"DEF", swigClassDeclInternal()},
    };
    
    stream << util::processTemplate(Templ, repl, true);
    stream.flush();
    return stream.good();   
}

std::string SwigMessage::swigFieldDefsInternal() const
{
    StringsList fields;
    fields.reserve(m_swigFields.size());

    for (auto* f : m_swigFields) {
        fields.push_back(f->swigClassDecl());
    }

    return util::strListToString(fields, "\n", "");
}

std::string SwigMessage::swigClassDeclInternal() const
{
    static const std::string Templ = 
        "class #^#CLASS_NAME#$# : public #^#INTERFACE#$#\n"
        "{\n"
        "public:\n"
        "    #^#FIELDS#$#\n"
        "    static const char* doName();\n"
        "    #^#MSG_ID#$# doGetId() const;\n"
        "    comms_ErrorStatus doRead(const #^#UINT8_T#$#*& iter, #^#SIZE_T#$# len);\n"
        "    comms_ErrorStatus doWrite(#^#UINT8_T#$#*& iter, #^#SIZE_T#$# len) const;\n"
        "    bool doRefresh();\n"
        "    #^#SIZE_T#$# doLength() const;\n"
        "    bool doValid() const;\n"
        "    #^#CUSTOM#$#\n"
        "};\n";

    auto& gen = SwigGenerator::cast(generator());
    auto* iFace = gen.swigMainInterface();
    assert(iFace != nullptr);
    util::ReplacementMap repl = {
        {"CLASS_NAME", gen.swigClassName(*this)},
        {"INTERFACE", gen.swigClassName(*iFace)},
        {"FIELDS", swigFieldsAccDeclInternal()},
        {"CUSTOM", util::readFileContents(gen.swigInputCodePathFor(*this) + strings::appendFileSuffixStr())},
        {"UINT8_T", gen.swigConvertCppType("std::uint8_t")},
        {"SIZE_T", gen.swigConvertCppType("std::size_t")},
        {"MSG_ID", SwigMsgId::swigClassName(gen)}
    };

    return util::processTemplate(Templ, repl);    
}

std::string SwigMessage::swigFieldsAccDeclInternal() const
{
    StringsList accFuncs;
    accFuncs.reserve(m_swigFields.size());

    auto& gen = SwigGenerator::cast(generator());
    for (auto* f : m_swigFields) {
        static const std::string Templ = {
            "#^#CLASS_NAME#$#& field_#^#ACC_NAME#$#();\n"
        };

        util::ReplacementMap repl = {
            {"CLASS_NAME", gen.swigClassName(f->field())},
            {"ACC_NAME", comms::accessName(f->field().dslObj().name())}
        };

        accFuncs.push_back(util::processTemplate(Templ, repl));
    }

    return util::strListToString(accFuncs, "\n", "");
}

std::string SwigMessage::swigFieldsAccCodeInternal() const
{
    StringsList accFuncs;
    accFuncs.reserve(m_swigFields.size());

    auto& gen = SwigGenerator::cast(generator());
    for (auto* f : m_swigFields) {
        static const std::string Templ = {
            "#^#CLASS_NAME#$#& field_#^#ACC_NAME#$#()\n"
            "{\n"
            "    return static_cast<#^#CLASS_NAME#$#&>(Base::field_#^#ACC_NAME#$#());\n"
            "}\n"
            "const #^#CLASS_NAME#$#& field_#^#ACC_NAME#$#() const\n"
            "{\n"
            "    return static_cast<const #^#CLASS_NAME#$#&>(Base::field_#^#ACC_NAME#$#());\n"
            "}\n"            
        };

        util::ReplacementMap repl = {
            {"CLASS_NAME", gen.swigClassName(f->field())},
            {"ACC_NAME", comms::accessName(f->field().dslObj().name())}
        };

        accFuncs.push_back(util::processTemplate(Templ, repl));
    }

    return util::strListToString(accFuncs, "\n", "");
}

} // namespace commsdsl2swig
