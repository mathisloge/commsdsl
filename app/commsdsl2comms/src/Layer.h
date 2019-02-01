//
// Copyright 2018 (C). Alex Robenko. All rights reserved.
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

#include <string>
#include <memory>
#include <vector>

#include "commsdsl/Layer.h"
#include "Field.h"
#include "common.h"

namespace commsdsl2comms
{

class Generator;
class Layer
{
public:
    using Ptr = std::unique_ptr<Layer>;
    using LayersList = std::vector<Ptr>;

    virtual ~Layer() = default;

    const std::string& name() const
    {
        return m_dslObj.name();
    }

    commsdsl::Layer::Kind kind() const
    {
        return m_dslObj.kind();
    }

    using IncludesList = common::StringsList;
    void updateIncludes(IncludesList& includes) const;
    void updatePluginIncludes(IncludesList& includes) const;

    bool prepare();

    std::string getClassDefinition(
        const std::string& scope,
        std::string& prevLayer,
        bool& hasInputMessages) const;

    static Ptr create(Generator& generator, commsdsl::Layer dslObj);

    std::string getDefaultOptions(const std::string& scope) const;

    bool rearange(LayersList& layers, bool& success)
    {
        return rearangeImpl(layers, success);
    }

    std::string getFieldScopeForPlugin(const std::string& scope) const;
    std::string getFieldAccNameForPlugin() const;
    std::string getPluginCreatePropsFunc(const std::string& scope) const;

protected:
    Layer(Generator& generator, commsdsl::Layer field)
      : m_generator(generator),
        m_dslObj(field) {}

    Generator& generator() const
    {
        return m_generator;
    }

    const commsdsl::Layer& dslObj() const
    {
        return m_dslObj;
    }

    const Field* getField() const;

    std::string getPrefix() const;
    std::string getFieldDefinition(const std::string& scope) const;
    std::string getFieldType() const;
    std::string getExtraOpt(const std::string& scope) const;

    FieldPtr& memberField()
    {
        return m_field;
    }

    void setFieldForcedFailOnInvalid();


    virtual bool prepareImpl();
    virtual void updateIncludesImpl(IncludesList& includes) const;
    virtual std::string getClassDefinitionImpl(
        const std::string& scope,
        std::string& prevLayer,
        bool& hasInputMessages) const = 0;
    virtual std::string getExtraDefaultOptionsImpl(const std::string& scope) const;
    virtual bool rearangeImpl(LayersList& layers, bool& success);
    virtual bool isCustomizableImpl() const;

private:

    bool isCustomizable() const
    {
        return isCustomizableImpl();
    }

    Generator& m_generator;
    commsdsl::Layer m_dslObj;
    FieldPtr m_field;
    bool m_forcedFieldFailOnInvalid = false;
};

using LayerPtr = Layer::Ptr;

} // namespace commsdsl2comms