#include <zabato/asset_bundle.hpp>
#include <zabato/ice.hpp>
#include <zabato/resource.hpp>

namespace zabato
{

const rtti asset_bundle::TYPE("zabato.asset_bundle", &resource::TYPE);

void asset_bundle::add_resource(const string &name, shared_ptr<resource> res)
{
    m_resources.push_back({name, res});
}

shared_ptr<resource> asset_bundle::get_resource(const string &name) const
{
    for (const auto &sub : m_resources)
    {
        if (sub.name == name)
            return sub.res;
    }
    return nullptr;
}

shared_ptr<resource>
asset_bundle::get_first_resource_of_type(const rtti &type) const
{
    for (const auto &sub : m_resources)
    {
        if (sub.res && sub.res->type().is_derived(type))
            return sub.res;
    }
    return nullptr;
}

template <>
result<void> deserialize<asset_bundle>(ice_reader &reader, asset_bundle &obj)
{
    // TODO:
    return {};
}
} // namespace zabato
