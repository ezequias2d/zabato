#include <editor/object_importer.hpp>
#include <editor/object_resource.hpp>
#include <zabato/resource.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato::editor
{

void register_object_importer(resource_manager *manager)
{
    importer_registry::register_importer(make_shared<object_importer>(manager));
}

bool object_importer::is_resource_type(const rtti &type) const
{
    return type.is_exactly(object_resource::TYPE);
}

result<shared_ptr<resource>>
object_importer::import(class resource_manager &manager,
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

    auto xml = fs->read_all_text(path);
    if (xml.empty())
        return report_error(error_code::file_not_found, path.c_str());

    xml_serializer s;
    if (s.doc().Parse(xml.c_str(), xml.size()) != tinyxml2::XML_SUCCESS)
        return report_error(error_code::unable_to_read, "Failed to parse XML");

    tinyxml2::XMLElement *root = s.doc().RootElement();
    if (!root)
        return report_error(error_code::value, "No root element");

    object *obj = object::factory(s, *root);

    if (!obj)
        return report_error(error_code::value,
                            "Failed to create object from XML");

    shared_ptr<object_resource> res = make_shared<object_resource>(obj);
    return static_cast<shared_ptr<resource>>(res);
}

} // namespace zabato::editor
