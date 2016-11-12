#pragma once

namespace zfw
{
    // keep in sync with 'exceptionNames' in sys.cpp
    enum ExceptionType
    {
        EX_NO_ERROR = 0,
        EX_ACCESS_DENIED,
        EX_ALLOC_ERR,
        EX_ASSERTION_FAILED,
        EX_ASSET_CORRUPTED,
        EX_ASSET_FORMAT_UNSUPPORTED,
        EX_ASSET_OPEN_ERR,
        EX_BACKEND_ERR,
        EX_CONTROLLER_UNAVAILABLE,
        EX_DATAFILE_ERR,
        EX_DATAFILE_FORMAT_ERR,
        EX_DOCUMENT_MALFORMED,
        EX_FEATURE_DISABLED,
        EX_INTERNAL_STATE,
        EX_INVALID_ARGUMENT,
        EX_INVALID_OPERATION,
        EX_IO_ERROR,
        EX_LIBRARY_INTERFACE_ERR,
        EX_LIBRARY_OPEN_ERR,
        EX_LIST_OPEN_ERR,
        EX_NOT_FOUND,
        EX_NOT_IMPLEMENTED,
        EX_OBJECT_UNDEFINED,
        EX_OPERATION_FAILED,
        EX_SCRIPT_ERROR,
        EX_SERIALIZATION_ERR,
        EX_SHADER_COMPILE_ERR,
        EX_SHADER_LOAD_ERR,
        EX_WRITE_ERR,

        // New (2013/08)
        ERR_DATA_FORMAT,
        ERR_DATA_SIZE,
        ERR_DATA_STRUCTURE,

        // New (2014+)
        errApplication,
        errDataCorruption,
        errDataNotFound,
        errNoSuitableCodec,
        errRequestedEncodingNotSupported,
        errVariableAlreadyBound,
        errVariableNotBound,
        errVariableNotSet,

        MAX_EXCEPTION
    };
}