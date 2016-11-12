
#include <framework/system.hpp>
#include <framework/varsystem.hpp>
#include <framework/utility/errorbuffer.hpp>
#include <framework/utility/essentials.hpp>

#include <string>
#include <unordered_map>

namespace zfw
{
    using std::string;

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    class VarSystem : public IVarSystem
    {
        public:
            VarSystem(ISystem* sys) : sys(sys) {}

            virtual bool GetVariable(const char* name, const char** value_out, int flags) override;
            virtual bool SetVariable(const char* name, const char* value, int flags) override;

            virtual bool BindVariable(const char* name, reflection::ReflectedValue_t var, int access,
                    int flags) override;
            //virtual bool UnbindVariable(const char* name, bool rememberValue) = 0;

            virtual bool SerializeVariables(OutputStream* output, const char* outputNameOrNull, int flags,
                    const char** names, size_t numNames) override;
            virtual bool DeserializeVariables(InputStream* input, const char* inputNameOrNull, int flags,
                    const char** names, size_t numNames) override;

        private:
            void p_SetErrorAlreadyBound(const char* name, const char* function);
            void p_SetErrorDoesntExist(const char* name, const char* function);
            void p_SetErrorNotBound(const char* name, const char* function);
            bool p_WriteToStream(OutputStream* output, const char* outputNameOrNull, const char* name, const char* value);

            ISystem* sys;

            string valueBuffer;

            std::unordered_map<string, string> unboundVars;
            std::unordered_map<string, reflection::ReflectedValue_t> boundVars;
    };

    // ====================================================================== //
    //  class VarSystem
    // ====================================================================== //

    IVarSystem* p_CreateVarSystem(ISystem* sys)
    {
        // Will be called in System pre-init!

        return new VarSystem(sys);
    }

    static int s_ParseVarSpec(const char* line, li::String& name_out, li::String& value_out)
    {
        if (!line || line[0] == ';')
            return 0;

        const char* nameBegin, * nameEnd;

        for ( ; *line != 0; line++)
        {
            if (!isspace(*line))
            {
                nameBegin = line;
                break;
            }
        }

        if (!*line)
            return -1;

        for ( ; *line != 0; line++)
        {
            if (isspace(*line))
            {
                nameEnd = line;
                break;
            }
        }

        if (!*line)
            return -1;

        for ( ; *line != 0; line++)
        {
            if (!isspace(*line))
            {
                name_out.set(nameBegin, nameEnd - nameBegin);
                value_out.set(line);
                return 1;
            }
        }

        return -1;
    }

    static bool s_WriteToStream(OutputStream* output, const char* outputNameOrNull, const char* name, const char* value)
    {
        if (!output->write(name)
                || !output->write("\t\t")
                || !output->write(value)
                || !output->write('\n')
                )
            return ErrorBuffer::SetWriteError(outputNameOrNull, li_functionName), false;

        return true;
    }

    void VarSystem::p_SetErrorAlreadyBound(const char* name, const char* function)
    {
        ErrorBuffer::SetError3(errVariableAlreadyBound, 3,
            "desc", sprintf_255("Variable '%s' is already bound.", name),
            "variableName", name,
            "function", function
            );
    }

    void VarSystem::p_SetErrorDoesntExist(const char* name, const char* function)
    {
        ErrorBuffer::SetError3(errVariableNotSet, 3,
                "desc", sprintf_255("Required variable '%s' is not set.", name),
                "variableName", name,
                "function", function
                );
    }

    void VarSystem::p_SetErrorNotBound(const char* name, const char* function)
    {
        ErrorBuffer::SetError3(errVariableNotBound, 3,
                "desc", sprintf_255("Required variable '%s' is not bound.", name),
                "variableName", name,
                "function", function
                );
    }

    bool VarSystem::BindVariable(const char* name, reflection::ReflectedValue_t var, int access, int flags)
    {
        string key(name);

        // Check if the variable isn't already bound...
        auto it = boundVars.find(key);

        // ...if it is, it's an error
        if (it != boundVars.end())
            return p_SetErrorAlreadyBound(name, li_functionName), false;

        // Register variable as bound
        auto& variable = boundVars[name];
        variable = var;

        // Look for an unbound variable with the same name
        // If it exists, move its value to the newly bound one
        auto it2 = unboundVars.find(key);

        if (it2 != unboundVars.end())
        {
            reflection::reflectFromString(variable, move(it2->second));
            unboundVars.erase(it2);
        }

        return true;
    }

    bool VarSystem::DeserializeVariables(InputStream* input, const char* inputNameOrNull, int flags,
            const char** names, size_t numNames)
    {
        while (!input->eof())
        {
            li::String line = input->readLine();
            li::String name, value;

            int rc = s_ParseVarSpec(line, name, value);

            if (rc < 0)
            {
                sys->Printf(kLogWarning, "Invalid variable specification in '%s': '%s'",
                        inputNameOrNull ? inputNameOrNull : "<stream>", line.c_str());
                continue;
            }
            else if (rc > 0 && !this->SetVariable(name, value, flags & kVariableAccessFlagMask))
                return false;
        }

        return true;
    }

    bool VarSystem::GetVariable(const char* name, const char** value_out, int flags)
    {
        string key(name);

        // Look among bound variables first
        auto it = boundVars.find(key);

        if (it != boundVars.end())
        {
            // Convert value to string and return
            valueBuffer = reflection::reflectToString((*it).second);
            *value_out = valueBuffer.c_str();
            return true;
        }
        else if (flags & kVariableMustBeBound)
            return p_SetErrorNotBound(name, li_functionName), false;

        // Not found? Check unbound variables now
        auto it2 = unboundVars.find(key);

        if (it2 != unboundVars.end())
        {
            valueBuffer = it2->second;
            *value_out = valueBuffer.c_str();
            return true;
        }
        else
        {
            if (flags & kVariableMustExist)
                p_SetErrorDoesntExist(name, li_functionName);

            return false;
        }
    }

    bool VarSystem::SerializeVariables(OutputStream* output, const char* outputNameOrNull, int flags,
            const char** names, size_t numNames)
    {
        if (flags & kListedOnly)
        {
            for (size_t i = 0; i < numNames; i++)
            {
                const char* value;

                if (this->GetVariable(names[i], &value, flags & kVariableAccessFlagMask))
                {
                    if (!s_WriteToStream(output, outputNameOrNull, names[i], value))
                        return false;
                }
                else if (flags & kVariableMustExist)
                    return false;
            }
        }
        else
        {
            for (auto pair : boundVars)
            {
                valueBuffer = reflection::reflectToString(pair.second);

                if (!s_WriteToStream(output, outputNameOrNull, pair.first.c_str(), valueBuffer.c_str()))
                    return false;
            }

            for (auto pair : unboundVars)
            {
                if (!s_WriteToStream(output, outputNameOrNull, pair.first.c_str(), pair.second.c_str()))
                    return false;
            }
        }

        return true;
    }

    bool VarSystem::SetVariable(const char* name, const char* value, int flags)
    {
        string key(name);

        // Look among bound variables first
        auto it = boundVars.find(key);

        if (it != boundVars.end())
        {
            // Convert value to its native type
            zombie_assert(reflection::reflectFromString(it->second, value));
            return true;
        }
        else if (flags & kVariableMustBeBound)
            return p_SetErrorNotBound(name, li_functionName), false;

        if (flags & kVariableMustExist)
        {
            // Make sure the variable already exists
            auto it2 = unboundVars.find(key);

            if (it2 == unboundVars.end())
                return p_SetErrorDoesntExist(name, li_functionName), false;

            // Reuse the iterator for faster access
            it2->second = value;
        }
        else
            unboundVars[key] = string(value);

        return true;
    }
}
