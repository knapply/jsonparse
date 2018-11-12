#ifndef R_JSONPARSE_H
#define R_JSONPARSE_H

#include <Rcpp.h>

// [[Rcpp::depends(rapidjsonr)]]
#include "rapidjson/document.h"

using namespace Rcpp;

List parse_value(const rapidjson::Value& val);

namespace js_vars {
  extern std::unordered_set<int> dtypes;
}

#endif
