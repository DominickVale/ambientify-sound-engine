//
// Created by dominick on 3/3/22.
//
#include <fmod_errors.h>
#include "common.h"

void (*Common_Private_Error)(FMOD_RESULT, const char *, int);

void ERRCHECK_fn(FMOD_RESULT result, const char *file, int line)
{
    if (result != FMOD_OK)
    {
        if (Common_Private_Error)
        {
            Common_Private_Error(result, file, line);
        }
        __android_log_print(
                ANDROID_LOG_ERROR, ambientify::constants::LOG_ID,
                "%s(%d): FMOD error %d - %s", file, line, result, FMOD_ErrorString(result));
            throw ambientify::commons::ASoundEngineException(
                fmt::format("{}(line {}): FMOD error {} - {}", file, line, result, FMOD_ErrorString(result)));
    }
}

