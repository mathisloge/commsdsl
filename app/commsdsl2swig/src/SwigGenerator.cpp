//
// Copyright 2021 - 2022 (C). Alex Robenko. All rights reserved.
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

#include "SwigGenerator.h"

#include "Swig.h"
#include "SwigBitfieldField.h"
#include "SwigBundleField.h"
#include "SwigChecksumLayer.h"
#include "SwigCmake.h"
#include "SwigComms.h"
#include "SwigCustomLayer.h"
#include "SwigDataBuf.h"
#include "SwigDataField.h"
#include "SwigEnumField.h"
#include "SwigFloatField.h"
#include "SwigFrame.h"
#include "SwigIdLayer.h"
#include "SwigInterface.h"
#include "SwigIntField.h"
#include "SwigListField.h"
#include "SwigMessage.h"
#include "SwigMsgHandler.h"
#include "SwigMsgId.h"
#include "SwigNamespace.h"
#include "SwigOptionalField.h"
#include "SwigPayloadLayer.h"
#include "SwigRefField.h"
#include "SwigSchema.h"
#include "SwigSetField.h"
#include "SwigSizeLayer.h"
#include "SwigStringField.h"
#include "SwigSyncLayer.h"
#include "SwigValueLayer.h"
#include "SwigVariantField.h"

#include "commsdsl/gen/comms.h"
#include "commsdsl/gen/strings.h"
#include "commsdsl/gen/util.h"
#include "commsdsl/version.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <filesystem>
#include <map>

namespace comms = commsdsl::gen::comms;
namespace fs = std::filesystem;
namespace strings = commsdsl::gen::strings;
namespace util = commsdsl::gen::util;

namespace commsdsl2swig
{

const std::string& SwigGenerator::fileGeneratedComment()
{
    static const std::string Str =
        "// Generated by commsdsl2swig v" + std::to_string(commsdsl::versionMajor()) +
        '.' + std::to_string(commsdsl::versionMinor()) + '.' +
        std::to_string(commsdsl::versionPatch()) + '\n';
    return Str;
}

std::string SwigGenerator::swigInputCodePathFor(const Elem& elem) const
{
    return getCodeDir() + '/' + strings::includeDirStr() + '/' + comms::relHeaderPathFor(elem, *this);
}

std::string SwigGenerator::swigInputCodePathForFile(const std::string& name) const
{
    return getCodeDir() + '/' + name;
}

std::string SwigGenerator::swigClassName(const Elem& elem) const
{
    bool addMainNamespace = m_mainNamespaceInNamesForced || (schemas().size() > 1U); 
    auto str = comms::scopeFor(elem, *this, addMainNamespace);
    return swigScopeToName(str);
}

std::string SwigGenerator::swigClassNameForRoot(const std::string& name) const
{
    bool addMainNamespace = m_mainNamespaceInNamesForced || (schemas().size() > 1U); 
    auto str = comms::scopeForRoot(name, *this, addMainNamespace);
    return swigScopeToName(str);
}

std::string SwigGenerator::swigProtocolClassNameForRoot(const std::string& name) const
{
    bool addMainNamespace = m_mainNamespaceInNamesForced || (schemas().size() > 1U); 
    auto schemaIdx = currentSchemaIdx();
    auto* thisGen = const_cast<SwigGenerator*>(this);
    thisGen->chooseProtocolSchema();
    auto str = comms::scopeForRoot(name, *this, addMainNamespace);
    thisGen->chooseCurrentSchema(schemaIdx);
    return swigScopeToName(str);
}

const std::string& SwigGenerator::swigConvertCppType(const std::string& str) const
{
    static const std::map<std::string, std::string> Map = {
        {"std::int8_t", "signed char"},
        {"std::uint8_t", "unsigned char"},
        {"std::int16_t", "short"},
        {"std::uint16_t", "unsigned short"},
        {"std::int32_t", "int"},
        {"std::uint32_t", "unsigned"},
        {"std::int64_t", "long long"},
        {"std::uint64_t", "unsigned long long"},
        {"std::size_t", "unsigned long"},        
    };

    auto iter = Map.find(str);
    if (iter == Map.end()) {
        return str;
    }

    return iter->second;
}

const std::string& SwigGenerator::swigConvertIntType(commsdsl::parse::IntField::Type value, std::size_t len) const
{
    return swigConvertCppType(comms::cppIntTypeFor(value, len));
}

std::string SwigGenerator::swigScopeToName(const std::string& scope)
{
    return util::strReplace(scope, "::", "_");
}

std::string SwigGenerator::swigDefInclude(const std::string& path)
{
    return "%include \"include/" + path + '\"';
}

bool SwigGenerator::prepareImpl()
{
    if (!Base::prepareImpl()) {
        return false;
    }

    bool result =  
        prepareDefaultInterfaceInternal();

    if (!result) {
        return false;
    }

    if (m_forcedInterface.empty()) {
        return true;
    }
    
    auto* iFace = findInterface(m_forcedInterface);
    if (iFace == nullptr) {
        logger().error("The selected forced interface \"" + m_forcedInterface + "\" hasn't been found");
        return false;
    }

    return true;
}

bool SwigGenerator::writeImpl()
{
    for (auto idx = 0U; idx < schemas().size(); ++idx) {
        chooseCurrentSchema(idx);
        bool result = 
            SwigMsgId::write(*this);

        if (!result) {
            return false;
        }
    }

    return 
        SwigComms::swigWrite(*this) &&
        SwigDataBuf::write(*this) &&
        SwigMsgHandler::write(*this) &&
        Swig::swigWrite(*this) &&
        SwigCmake::swigWrite(*this) &&
        swigWriteExtraFilesInternal();

}

void SwigGenerator::setMainNamespaceInNamesForced(bool value)
{
    m_mainNamespaceInNamesForced = value;
}

void SwigGenerator::setForcedInterface(const std::string& value)
{
    m_forcedInterface = value;
}

const SwigInterface* SwigGenerator::swigMainInterface() const
{
    do {
        if (m_forcedInterface.empty()) {
            break;
        }

        auto iFace = findInterface(m_forcedInterface);
        if (iFace == nullptr) {
            break;
        }

        return static_cast<const SwigInterface*>(iFace);
    } while (false);

    auto allInterfaces = getAllInterfaces();
    assert(!allInterfaces.empty());
    return static_cast<const SwigInterface*>(allInterfaces.front());
}

SwigGenerator::SchemaPtr SwigGenerator::createSchemaImpl(commsdsl::parse::Schema dslObj, Elem* parent)
{
    return std::make_unique<SwigSchema>(*this, dslObj, parent);
}

SwigGenerator::NamespacePtr SwigGenerator::createNamespaceImpl(commsdsl::parse::Namespace dslObj, Elem* parent)
{
    return std::make_unique<SwigNamespace>(*this, dslObj, parent);
}

SwigGenerator::InterfacePtr SwigGenerator::createInterfaceImpl(commsdsl::parse::Interface dslObj, Elem* parent)
{
    return std::make_unique<SwigInterface>(*this, dslObj, parent);
}

SwigGenerator::MessagePtr SwigGenerator::createMessageImpl(commsdsl::parse::Message dslObj, Elem* parent)
{
    return std::make_unique<SwigMessage>(*this, dslObj, parent);
}

SwigGenerator::FramePtr SwigGenerator::createFrameImpl(commsdsl::parse::Frame dslObj, Elem* parent)
{
    return std::make_unique<SwigFrame>(*this, dslObj, parent);
}

SwigGenerator::FieldPtr SwigGenerator::createIntFieldImpl(commsdsl::parse::Field dslObj, Elem* parent)
{
    return std::make_unique<SwigIntField>(*this, dslObj, parent);
}

SwigGenerator::FieldPtr SwigGenerator::createEnumFieldImpl(commsdsl::parse::Field dslObj, Elem* parent)
{
    return std::make_unique<SwigEnumField>(*this, dslObj, parent);
}

SwigGenerator::FieldPtr SwigGenerator::createSetFieldImpl(commsdsl::parse::Field dslObj, Elem* parent)
{
    return std::make_unique<SwigSetField>(*this, dslObj, parent);
}

SwigGenerator::FieldPtr SwigGenerator::createFloatFieldImpl(commsdsl::parse::Field dslObj, Elem* parent)
{
    return std::make_unique<SwigFloatField>(*this, dslObj, parent);
}

SwigGenerator::FieldPtr SwigGenerator::createBitfieldFieldImpl(commsdsl::parse::Field dslObj, Elem* parent)
{
    return std::make_unique<SwigBitfieldField>(*this, dslObj, parent);
}

SwigGenerator::FieldPtr SwigGenerator::createBundleFieldImpl(commsdsl::parse::Field dslObj, Elem* parent)
{
    return std::make_unique<SwigBundleField>(*this, dslObj, parent);
}

SwigGenerator::FieldPtr SwigGenerator::createStringFieldImpl(commsdsl::parse::Field dslObj, Elem* parent)
{
    return std::make_unique<SwigStringField>(*this, dslObj, parent);
}

SwigGenerator::FieldPtr SwigGenerator::createDataFieldImpl(commsdsl::parse::Field dslObj, Elem* parent)
{
    return std::make_unique<SwigDataField>(*this, dslObj, parent);
}

SwigGenerator::FieldPtr SwigGenerator::createListFieldImpl(commsdsl::parse::Field dslObj, Elem* parent)
{
    return std::make_unique<SwigListField>(*this, dslObj, parent);
}

SwigGenerator::FieldPtr SwigGenerator::createRefFieldImpl(commsdsl::parse::Field dslObj, Elem* parent)
{
    return std::make_unique<SwigRefField>(*this, dslObj, parent);
}

SwigGenerator::FieldPtr SwigGenerator::createOptionalFieldImpl(commsdsl::parse::Field dslObj, Elem* parent)
{
    return std::make_unique<SwigOptionalField>(*this, dslObj, parent);
}

SwigGenerator::FieldPtr SwigGenerator::createVariantFieldImpl(commsdsl::parse::Field dslObj, Elem* parent)
{
    return std::make_unique<SwigVariantField>(*this, dslObj, parent);
}

SwigGenerator::LayerPtr SwigGenerator::createCustomLayerImpl(commsdsl::parse::Layer dslObj, Elem* parent)
{
    return std::make_unique<SwigCustomLayer>(*this, dslObj, parent);
}

SwigGenerator::LayerPtr SwigGenerator::createSyncLayerImpl(commsdsl::parse::Layer dslObj, Elem* parent)
{
    return std::make_unique<SwigSyncLayer>(*this, dslObj, parent);
}

SwigGenerator::LayerPtr SwigGenerator::createSizeLayerImpl(commsdsl::parse::Layer dslObj, Elem* parent)
{
    return std::make_unique<SwigSizeLayer>(*this, dslObj, parent);
}

SwigGenerator::LayerPtr SwigGenerator::createIdLayerImpl(commsdsl::parse::Layer dslObj, Elem* parent)
{
    return std::make_unique<SwigIdLayer>(*this, dslObj, parent);
}

SwigGenerator::LayerPtr SwigGenerator::createValueLayerImpl(commsdsl::parse::Layer dslObj, Elem* parent)
{
    return std::make_unique<SwigValueLayer>(*this, dslObj, parent);
}

SwigGenerator::LayerPtr SwigGenerator::createPayloadLayerImpl(commsdsl::parse::Layer dslObj, Elem* parent)
{
    return std::make_unique<SwigPayloadLayer>(*this, dslObj, parent);
}

SwigGenerator::LayerPtr SwigGenerator::createChecksumLayerImpl(commsdsl::parse::Layer dslObj, Elem* parent)
{
    return std::make_unique<SwigChecksumLayer>(*this, dslObj, parent);
}

bool SwigGenerator::swigWriteExtraFilesInternal() const
{
    auto& inputDir = getCodeDir();
    if (inputDir.empty()) {
        return true;
    }

    auto& outputDir = getOutputDir();
    auto pos = inputDir.size();
    auto endIter = fs::recursive_directory_iterator();
    for (auto iter = fs::recursive_directory_iterator(inputDir); iter != endIter; ++iter) {
        if (!iter->is_regular_file()) {
            continue;
        }
        

        auto srcPath = iter->path();
        auto ext = srcPath.extension().string();

        static const std::string ReservedExt[] = {
            strings::replaceFileSuffixStr(),
            strings::extendFileSuffixStr(),
            strings::publicFileSuffixStr(),
            strings::incFileSuffixStr(),
            strings::appendFileSuffixStr(),
        };        
        auto extIter = std::find(std::begin(ReservedExt), std::end(ReservedExt), ext);
        if (extIter != std::end(ReservedExt)) {
            continue;
        }

        auto pathStr = srcPath.string();
        auto posTmp = pos;
        while (posTmp < pathStr.size()) {
            if (pathStr[posTmp] == fs::path::preferred_separator) {
                ++posTmp;
                continue;
            }
            break;
        }

        if (pathStr.size() <= posTmp) {
            continue;
        }

        std::string relPath(pathStr, posTmp);
        auto& protSchema = protocolSchema();
        auto schemaNs = util::strToName(protSchema.schemaName());
        do {
            if (protSchema.mainNamespace() == schemaNs) {
                break;
            }

            auto srcPrefix = (fs::path(strings::includeDirStr()) / schemaNs).string();
            if (!util::strStartsWith(relPath, srcPrefix)) {
                break;
            }

            auto dstPrefix = (fs::path(strings::includeDirStr()) / protSchema.mainNamespace()).string();
            relPath = dstPrefix + std::string(relPath, srcPrefix.size());
        } while (false);

        auto destPath = fs::path(outputDir) / relPath;
        logger().info("Copying " + destPath.string());

        if (!createDirectory(destPath.parent_path().string())) {
            return false;
        }

        std::error_code ec;
        fs::copy_file(srcPath, destPath, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            logger().error("Failed to copy with reason: " + ec.message());
            return false;
        }

        if (protSchema.mainNamespace() != schemaNs) {
            // The namespace has changed

            auto destStr = destPath.string();
            std::ifstream stream(destStr);
            if (!stream) {
                logger().error("Failed to open " + destStr + " for modification.");
                return false;
            }

            std::string content((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
            stream.close();

            util::strReplace(content, "namespace " + schemaNs, "namespace " + protSchema.mainNamespace());
            std::ofstream outStream(destStr, std::ios_base::trunc);
            if (!outStream) {
                logger().error("Failed to modify " + destStr + ".");
                return false;
            }

            outStream << content;
            logger().info("Updated " + destStr + " to have proper main namespace.");
        }
    }
    return true;
}


bool SwigGenerator::prepareDefaultInterfaceInternal()
{
    auto allInterfaces = getAllInterfaces();
    if (!allInterfaces.empty()) {
        return true;
    }

    auto* defaultNamespace = addDefaultNamespace();
    auto* interface = defaultNamespace->addDefaultInterface();
    if (interface == nullptr) {
        logger().error("Failed to create default interface");
        return false;
    }

    return true;
}

} // namespace commsdsl2swig
