#pragma once

#include <reflection/api.hpp>

#include <littl/Stream.hpp>

namespace zfw
{
    class IVarSystem
    {
        public:
            enum VariableAccessFlag_t
            {
                kVariableMustExist = 1,
                kVariableMustBeBound = 2,
                kVariableAccessFlagMask = 3,
            };

            enum AccessModeFlag_t
            {
                kReadOnly = 1,
                kWriteOnly = 2,
                kReadWrite = 3,
            };

            enum SerializationFlag_t
            {
                //kVariableMustExist
                kListedOnly = 16
            };

            virtual ~IVarSystem() {}

            // see VariableAccessFlag_t for possible flags
            virtual bool GetVariable(const char* name, const char** value_out, int flags) = 0;
            virtual bool SetVariable(const char* name, const char* value, int flags) = 0;

            // see AccessModeFlag_t for possible access flags
            // see VariableAccessFlag_t for possible flags
            virtual bool BindVariable(const char* name, reflection::ReflectedValue_t var, int access, int flags) = 0;
            //virtual bool UnbindVariable(const char* name, bool rememberValue) = 0;

            // see SerializationFlag_t for possible flags
            virtual bool SerializeVariables(li::OutputStream* output, const char* outputNameOrNull, int flags,
                    const char** names, size_t numNames) = 0;
            virtual bool DeserializeVariables(li::InputStream* input, const char* outputNameOrNull, int flags,
                    const char** names, size_t numNames) = 0;

            template <typename T>
            bool GetVariable(const char* name, T* value_out, int flags)
            {
                const char* value;

                if (GetVariable(name, &value, flags))
                    return reflection::reflectFromString(*value_out, value);
                else
                    return false;
            }

            template <typename T>
            T GetVariableOrDefault(const char* name, const T& default_)
            {
                T value;

                if (GetVariable(name, &value, 0))
                    return value;
                else
                    return default_;
            }

            const char* GetVariableOrEmptyString(const char* name)
            {
                const char* value;

                if (GetVariable(name, &value, 0))
                    return value;
                else
                    return "";
            }
    };
}
