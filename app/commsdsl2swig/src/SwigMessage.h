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

#pragma once

#include "SwigField.h"

#include "commsdsl/gen/Message.h"
#include "commsdsl/gen/util.h"

namespace commsdsl2swig
{

class SwigGenerator;
class SwigMessage final: public commsdsl::gen::Message
{
    using Base = commsdsl::gen::Message;

public:
    explicit SwigMessage(SwigGenerator& generator, commsdsl::parse::Message dslObj, Elem* parent);
    virtual ~SwigMessage();

protected:
    virtual bool prepareImpl() override;    
    virtual bool writeImpl() const override;

private:
    using SwigFieldsList = SwigField::SwigFieldsList;
    using StringsList = commsdsl::gen::util::StringsList;

    std::string swigFieldDefsInternal() const;
    std::string swigClassDefInternal() const;
    std::string swigFieldsAccessInternal() const;

    SwigFieldsList m_swigFields;        
};

} // namespace commsdsl2swig
