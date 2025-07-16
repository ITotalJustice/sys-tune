#pragma once

#include <switch.h>
#include "tune.h"

#define R_UNLESS(bool_expr, res) \
    ({                           \
        if (!(bool_expr)) {      \
            return res;          \
        }                        \
    })

#define R_TRY(res_expr)                   \
    ({                                    \
        const auto temp_res = (res_expr); \
        if (R_FAILED(temp_res))           \
            return temp_res;              \
    })

#define R_ABORT_UNLESS(res_expr)   \
    ({                             \
        auto tmp_res = (res_expr); \
        if (R_FAILED(tmp_res))     \
            diagAbortWithResult(tmp_res);   \
    })
