//
// Copyright 2018 - 2022 (C). Alex Robenko. All rights reserved.
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

#include "commsdsl/version.h"
#include "commsdsl/gen/util.h"

#include "SwigGenerator.h"
#include "SwigProgramOptions.h"

#include <stdexcept>
#include <iostream>
#include <cassert>
#include <fstream>

namespace commsdsl2swig
{

std::vector<std::string> getFilesList(
    const std::string& fileName,
    const std::string& prefix)
{
    std::vector<std::string> result;
    do {
        if (fileName.empty()) {
            break;
        }
        
        std::ifstream stream(fileName);
        if (!stream) {
            break;
        }
        
        std::string contents(std::istreambuf_iterator<char>(stream), (std::istreambuf_iterator<char>()));

        result = commsdsl::gen::util::strSplitByAnyChar(contents, "\r\n");
        if (prefix.empty()) {
            break;
        }

        for (auto& f : result) {
            f = commsdsl::gen::util::pathAddElem(prefix, f);
        }
    } while (false);
    return result;
}

} // namespace commsdsl2swig

int main(int argc, const char* argv[])
{
    static_cast<void>(argc);
    static_cast<void>(argv);

    try {
        commsdsl2swig::SwigProgramOptions options;
        options.parse(argc, argv);
        if (options.helpRequested()) {
            std::cout << "Usage:\n\t" << argv[0] << " [OPTIONS] schema_file1 [schema_file2] [schema_file3] ...\n\n";
            std::cout << options.helpStr();
            return 0;
        }

        if (options.versionRequested()) {
            std::cout << 
                commsdsl::versionMajor() << '.' << 
                commsdsl::versionMinor() << '.' <<
                commsdsl::versionPatch() << std::endl;
            return 0;
        }        

        commsdsl2swig::SwigGenerator generator;
        auto& logger = generator.logger();

        if (options.quietRequested()) {
            logger.setMinLevel(commsdsl::parse::ErrorLevel_Warning);
        }

        if (options.warnAsErrRequested()) {
            logger.setWarnAsError();
        }

        if (options.hasNamespaceOverride()) {
            generator.setNamespaceOverride(options.getNamespace());
        }

        if (options.hasForcedInterface()) {
            generator.swigSetForcedInterface(options.getForcedInterface());
        }

        generator.setOutputDir(options.getOutputDirectory());
        generator.setCodeDir(options.getCodeInputDirectory());
        generator.setMultipleSchemasEnabled(options.multipleSchemasEnabled());
        generator.setMinRemoteVersion(options.getMinRemoteVersion());
        generator.swigSetMainNamespaceInNamesForced(options.isMainNamespaceInNamesForced());
        generator.swigSetHasProtocolVersion(options.hasProtocolVersion());
        generator.swigSetMessagesListFile(options.messagesListFile());

        auto files = commsdsl2swig::getFilesList(options.getFilesListFile(), options.getFilesListPrefix());
        auto otherFiles = options.getFiles();
        files.insert(files.end(), otherFiles.begin(), otherFiles.end());

        if (files.empty()) {
            logger.error("No input files are provided");
            return -1;
        }

        if (!generator.prepare(files)) {
            return -1;
        }

        if (!generator.write()) {
            return -1;
        }
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
        assert(false);
    }

    return -1;
}
