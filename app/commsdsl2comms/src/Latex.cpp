//
// Copyright 2018 - 2020 (C). Alex Robenko. All rights reserved.
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

#include "Latex.h"

#include <fstream>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "Generator.h"
#include "common.h"

namespace bf = boost::filesystem;
namespace ba = boost::algorithm;

namespace commsdsl2comms
{

    namespace
    {

        std::vector<std::string> getAppendReq(const std::string &file)
        {
            std::vector<std::string> result;
            result.push_back(common::docStr());
            result.push_back(file);
            return result;
        }

    } // namespace

    bool Latex::write(Generator &generator)
    {
        Latex obj(generator);
        return obj.writePlatforms() &&
               obj.writeFrame() &&
               obj.writeMessages();
    }

    bool Latex::writePlatforms() const
    {
        static const std::string PlatformsFile("platforms.tex");
        auto filePath = m_generator.startProtocolDocWrite(PlatformsFile);

        if (filePath.empty())
        {
            return true;
        }

        std::ofstream stream(filePath);
        if (!stream)
        {
            m_generator.logger().error("Failed to open \"" + filePath + "\" for writing.");
            return false;
        }

        stream << "\\section{Platforms}" << std::endl
               << "\\begin{description}" << std::endl;

        for (const auto &platform : m_generator.platforms())
        {
            stream << "\\item[" << platform << "] " << std::endl;
        }
        stream << "\\end{description}" << std::endl;
        stream.flush();
        if (!stream.good())
        {
            m_generator.logger().error("Failed to write \"" + filePath + "\".");
            return false;
        }
        return true;
    }

    bool Latex::writeFrame() const
    {
        static const std::string FramesFile("frames.tex");
        auto filePath = m_generator.startProtocolDocWrite(FramesFile);

        if (filePath.empty())
        {
            return true;
        }

        std::ofstream stream(filePath);
        if (!stream)
        {
            m_generator.logger().error("Failed to open \"" + filePath + "\" for writing.");
            return false;
        }

        stream << "\\section{Frames}" << std::endl;

        for (const auto frame : m_generator.getAllFrames())
        {
            const auto &frame_dsl = frame->getDsl();
            stream << "\\subsection{" << frame_dsl.name() << "}" << std::endl;
            stream << frame_dsl.description() << std::endl;
            stream << "\\paragraph{fields}" << std::endl;

            stream << "\\begin{description}" << std::endl;
            for (const auto &layer : frame->getDsl().layers())
            {
                switch(layer.kind()) {
                    case commsdsl::Layer::Kind::Checksum: 
                    commsdsl::ChecksumLayer x(layer);
                    stream << "from " << x.fromLayer() << " to " << x.untilLayer() << std::endl;
                    break;
                }
                stream << "\\item[" << layer.name() << "] " << layer.description() << std::endl;
            }
            stream << "\\end{description}" << std::endl;
        }

        return true;
    }

    bool Latex::writeMessages() const { return true; }
} // namespace commsdsl2comms
