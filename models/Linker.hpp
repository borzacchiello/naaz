#pragma once

#include <map>

#include "Model.hpp"

namespace naaz::models
{

class Linker
{
    std::map<std::string, const Model*> m_models;

  public:
    static Linker& The();

    void register_model(const std::string& name, const Model* m);

    void link(state::State& state) const;
};

} // namespace naaz::models
