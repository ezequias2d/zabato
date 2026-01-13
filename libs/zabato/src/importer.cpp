#include <zabato/importer.hpp>

namespace zabato
{

vector<shared_ptr<importer>> importer_registry::s_importers;

void importer_registry::register_importer(shared_ptr<importer> imp)
{
    s_importers.push_back(imp);
}

shared_ptr<importer> importer_registry::find_importer(const string &extension)
{
    for (const auto &imp : s_importers)
    {
        if (imp->supports(extension))
        {
            return imp;
        }
    }
    return nullptr;
}

shared_ptr<importer>
importer_registry::find_importer_by_name(const string &name)
{
    for (const auto &imp : s_importers)
    {
        if (imp->name() == name)
        {
            return imp;
        }
    }
    return nullptr;
}

vector<shared_ptr<importer>>
importer_registry::find_importers(const string &extension)
{
    vector<shared_ptr<importer>> result;
    for (const auto &imp : s_importers)
    {
        if (imp->supports(extension))
        {
            result.push_back(imp);
        }
    }
    return result;
}

} // namespace zabato
