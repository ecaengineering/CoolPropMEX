#ifndef COOLPROP_MEX_STRING_UTILS_H
#define COOLPROP_MEX_STRING_UTILS_H

#include "mex.h"

inline char* getString(const mxArray *arr) {
    if (mxIsChar(arr)) {
        // Traditional char array
        return mxArrayToString(arr);
    } else if (mxIsClass(arr, "string")) {
        // MATLAB string (R2016b+)
        mxArray *lhs[1];
        mxArray *rhs[1];
        rhs[0] = const_cast<mxArray*>(arr);
        if (mexCallMATLAB(1, lhs, 1, rhs, "char") == 0) {
            return mxArrayToString(lhs[0]);
        }
    }
    return NULL;
}

#endif