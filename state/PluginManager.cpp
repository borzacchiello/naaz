#include "PluginManager.hpp"

#include "../util/ioutil.hpp"

namespace naaz::state
{

PluginManager::PluginManager(const PluginManager& other)
{
    for (auto [name, pl] : other.m_plugins)
        m_plugins[name] = pl->clone();
}

void PluginManager::register_plugin(PluginPtr plugin)
{
    if (m_plugins.contains(plugin->name())) {
        err("PluginManager")
            << "register_plugin(): the plugin " << plugin->name()
            << " was already registered" << std::endl;
        exit_fail();
    }

    m_plugins.emplace(plugin->name(), plugin);
}

PluginPtr PluginManager::get_plugin(const std::string& name)
{
    if (!m_plugins.contains(name))
        return nullptr;
    return m_plugins.at(name);
}

std::unique_ptr<PluginManager> PluginManager::clone() const
{
    return std::unique_ptr<PluginManager>(new PluginManager(*this));
}

} // namespace naaz::state
