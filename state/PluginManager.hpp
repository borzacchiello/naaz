#pragma once

#include <memory>
#include <string>
#include <map>

namespace naaz::state
{

class Plugin
{
  public:
    virtual ~Plugin() {}

    virtual const std::string&      name()  = 0;
    virtual std::shared_ptr<Plugin> clone() = 0;
};
typedef std::shared_ptr<Plugin> PluginPtr;

class PluginManager
{
    std::map<std::string, PluginPtr> m_plugins;

  public:
    PluginManager() {}
    PluginManager(const PluginManager& other);

    void      register_plugin(PluginPtr plugin);
    PluginPtr get_plugin(const std::string& name);

    std::unique_ptr<PluginManager> clone() const;
};

} // namespace naaz::state
