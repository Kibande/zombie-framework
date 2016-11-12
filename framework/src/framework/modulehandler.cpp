
#include <framework/errorbuffer.hpp>
#include <framework/framework.hpp>
#include <framework/module.hpp>
#include <framework/modulehandler.hpp>

#include <littl/HashMap.hpp>
#include <littl/Library.hpp>

namespace zfw
{
    using namespace li;

    class ModuleHandler : public IModuleHandler
    {
        public:
            ModuleHandler(ErrorBuffer_t* eb) : eb(eb) {}
            virtual ~ModuleHandler();

            virtual void* CreateInterface(const char* moduleName, const char* interfaceName) override;

        protected:
            struct Module_t
            {
                Library* lib;
                zfw_CreateInterface_t zfw_CreateInterface;
            };

            ErrorBuffer_t* eb;
            HashMap<String, Module_t> modules;

            Module_t* LoadModule(const char* moduleName);
    };

    IModuleHandler* p_CreateModuleHandler(ErrorBuffer_t* eb)
    {
        return new ModuleHandler(eb);
    }

    ModuleHandler::~ModuleHandler()
    {
        iterate2 (i, modules)
            delete (*i).value.lib;
    }

    void* ModuleHandler::CreateInterface(const char* moduleName, const char* interfaceName)
    {
        Module_t* module = modules.find(String(moduleName));

        if (module == nullptr)
            module = LoadModule(moduleName);

        if (module == nullptr)
            return nullptr;

        void* inst = module->zfw_CreateInterface(interfaceName);

        if (inst == nullptr)
            return ErrorBuffer::SetError2(eb, EX_LIBRARY_INTERFACE_ERR, 1,
                    "desc", (const char*) sprintf_t<255>("Module '%s' failed to provide interface '%s'.", (const char*) moduleName, interfaceName)
                    ), nullptr;

        return inst;
    }

    ModuleHandler::Module_t* ModuleHandler::LoadModule(const char* moduleName)
    {
        sprintf_t<255> filename("%s" ZOMBIE_LIBRARY_SUFFIX, moduleName);

        Module_t module;
        module.lib = Library::open(filename);

        if (module.lib == nullptr)
            return ErrorBuffer::SetError(eb, EX_LIBRARY_OPEN_ERR,
                    "desc", (const char*) sprintf_t<255>("Failed to open library '%s'", (const char*) filename),
                    nullptr),
                    nullptr;

        module.zfw_CreateInterface = module.lib->getEntry<zfw_CreateInterface_t>("zfw_CreateInterface");

        if (module.zfw_CreateInterface == nullptr)
        {
            delete module.lib;

            return ErrorBuffer::SetError(eb, EX_LIBRARY_INTERFACE_ERR,
                    "desc", (const char*) sprintf_t<255>("Missing entry 'zfw_CreateInterface' in library '%s'", (const char*) filename),
                    nullptr),
                    nullptr;
        }

        return &modules.set(String(moduleName), (Module_t&&) module);
    }
}
