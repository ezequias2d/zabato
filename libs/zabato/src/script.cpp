#include <zabato/fs.hpp>
#include <zabato/script.hpp>

namespace zabato
{
using namespace zabato::fs;

const rtti script_instance::TYPE =
    rtti("zabato.script_instance", &controller::TYPE);

class script_importer : public importer
{
public:
    string_view name() const override { return "Default Script Importer"; }

    bool supports(const string &extension) const override
    {
        return extension == ".lua";
    }

    result<shared_ptr<resource>>
    import(fs::file_system &fs,
           gpu *gpu_ctx,
           const string &path,
           const tinyxml2::XMLElement *settings) override
    {
        auto file = fs.open(path.c_str(), open_mode::read);
        if (!file)
            return error_code::file_not_found;

        file->seek(0, origin::end);
        size_t size = file->tell();
        file->seek(0, origin::begin);

        string content;
        content.resize(size);
        file->read(buffer((uint8_t *)content.data(), size));

        shared_ptr<script> res = make_shared<script>(content);
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
