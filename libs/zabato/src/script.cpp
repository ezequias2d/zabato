#include <zabato/fs.hpp>
#include <zabato/reflection.hpp>
#include <zabato/script.hpp>

namespace zabato
{
using namespace zabato::fs;

const rtti script_instance::TYPE =
    rtti("zabato.script_instance", &controller::TYPE, script_instance::reflect);

void script_instance::reflect(reflection &r) { controller::reflect(r); }

class script_importer : public importer
{
public:
    string_view name() const override { return "Default Script Importer"; }

    bool supports(const string &extension) const override
    {
        return extension == ".lua";
    }

    result<shared_ptr<resource>>
    import(class resource_manager &manager,
           const string &path,
           const tinyxml2::XMLElement *settings = nullptr) override
    {
        auto fs = manager.get_file_system();
        if (!fs)
            return report_error(error_code::value,
                                "File system not set in resource_manager");

        auto gpu = manager.get_gpu();
        if (!gpu)
            return report_error(error_code::value,
                                "GPU not set in resource_manager");

        auto script_txt = fs->read_all_text(path);
        if (script_txt.empty())
            return report_error(error_code::unable_to_match,
                                "Unable to read script file");

        shared_ptr<script> res = make_shared<script>(script_txt);
        return shared_ptr<resource>(res);
    }

    bool is_resource_type(const zabato::rtti &type) const override
    {
        return type.is_exactly(script::TYPE);
    }
};

void register_script_importer()
{
    importer_registry::register_importer(make_shared<script_importer>());
}

} // namespace zabato
