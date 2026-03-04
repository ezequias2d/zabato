#include "tinyxml2.h"
#include <zabato/asset_bundle.hpp>
#include <zabato/object.hpp>
#include <zabato/resource.hpp>
#include <zabato/rtti.hpp>

namespace zabato
{

const rtti resource::TYPE("zabato.resource", nullptr);

result<shared_ptr<resource>>
resource_manager::import_resource(const string &path)
{
    resource_ptr resource;
    if (m_resources.try_get_value(path, resource))
        return resource;

    if (!m_fs)
        return report_error(error_code::value,
                            "File system not set in resource_manager");

    string file_path = path;
    string sub_asset;

    size_t at_pos = path.find('@');
    if (at_pos != string::npos)
    {
        file_path = path.substr(0, at_pos);
        sub_asset = path.substr(at_pos + 1);
    }

    tinyxml2::XMLDocument doc;
    const tinyxml2::XMLElement *settings = nullptr;
    shared_ptr<importer> importer        = nullptr;

    string xml_path = file_path;
    size_t last_dot = xml_path.rfind('.');
    if (last_dot != string::npos)
    {
        xml_path = xml_path + ".xml";
        if (m_fs->exists(xml_path))
        {
            auto xml = m_fs->read_all_text(xml_path);
            if (doc.Parse(xml.c_str(), xml.size()) == tinyxml2::XML_SUCCESS)
            {
                auto root = doc.FirstChildElement("import");
                if (root)
                {
                    auto importer_elem = root->FirstChildElement("importer");
                    if (importer_elem && importer_elem->GetText())
                    {
                        const char *importer_name =
                            importer_elem->Attribute("name");
                        importer = importer_registry::find_importer_by_name(
                            importer_elem->GetText());
                    }
                    settings = root->FirstChildElement("settings");
                }
            }
        }
    }

    if (!importer)
    {
        if (last_dot != string::npos)
        {
            auto ext = file_path.substr(last_dot);
            importer = importer_registry::find_importer(ext);
        }
    }

    if (importer)
    {
        auto res = importer->import(*this, file_path, settings);
        if (res.has_error())
            return res.error;

        auto resource = res.value;
        shared_ptr<asset_bundle> bundle =
            dynamic_pointer_cast<asset_bundle>(resource);
        if (bundle)
        {
            m_resources.add_or_set(file_path, bundle);
            for (auto sub : bundle->get_resources())
                m_resources.add_or_set(file_path + "@" + sub.name, sub.res);

            if (!sub_asset.empty())
            {
                auto sub = bundle->get_resource(sub_asset);
                if (!sub)
                    return report_error(error_code::value,
                                        "Sub asset not found");

                m_resources.add_or_set(file_path + "@" + sub_asset, sub);
                return sub;
            }
        }

        resource = res.value;
        m_resources.add_or_set(path, resource);
        return resource;
    }

    return report_error(error_code::value, "No importer found for the file");
}

result<shared_ptr<resource>>
resource_manager::import_typed_resource(const string &path, const rtti &type)
{
    auto import_res = import_resource(path);
    if (import_res.has_error())
        return import_res.error;

    auto res = import_res.value;
    if (res->type().is_derived(type))
        return res;

    if (res->type().is_derived(asset_bundle::TYPE))
    {
        auto bundle = static_pointer_cast<asset_bundle>(res);
        auto sub    = bundle->get_first_resource_of_type(type);
        if (sub)
            return sub;
        return report_error(error_code::value,
                            "No resource of the type found in the bundle");
    }

    return report_error(error_code::value, "Resource is not of the type");
}

} // namespace zabato
