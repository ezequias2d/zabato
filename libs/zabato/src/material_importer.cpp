#include <zabato/gpu.hpp>
#include <zabato/material.hpp>
#include <zabato/material_importer.hpp>
#include <zabato/resource.hpp>
#include <zabato/string.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato
{
bool material_importer::is_resource_type(const zabato::rtti &type) const
{
    return material::TYPE.is_derived(type);
}

result<shared_ptr<resource>>
material_importer::import(class resource_manager &manager,
                          const string &path,
                          const tinyxml2::XMLElement *settings)
{
    auto fs = manager.get_file_system();
    if (!fs)
        return report_error(error_code::value,
                            "File system not set in resource_manager");

    auto gpu = manager.get_gpu();
    if (!gpu)
        return report_error(error_code::value,
                            "GPU not set in resource_manager");

    // Read file
    auto xml = fs->read_all_text(path);
    if (xml.empty())
        return report_error(error_code::unable_to_match, "XML Parse Error");

    tinyxml2::XMLDocument doc;
    if (doc.Parse(xml.c_str(), xml.size()) != tinyxml2::XML_SUCCESS)
        return report_error(error_code::unable_to_match, "XML Parse Error");

    tinyxml2::XMLElement *root = doc.FirstChildElement("material");
    if (!root)
        return report_error(error_code::unable_to_match,
                            "Invalid root element, expected <material>");

    auto mat = make_shared<material>();
    xml_serializer serializer;
    mat->load_xml(serializer, *root);
    mat->set_resource_manager(&manager);

    return static_pointer_cast<resource>(mat);
}

void material_importer::register_importer()
{
    importer_registry::register_importer(make_shared<material_importer>());
}

} // namespace zabato
