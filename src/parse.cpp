#include <Rcpp.h>

// [[Rcpp::depends(rapidjsonr)]]
#include "rapidjson/document.h"

using namespace Rcpp;


// Will iterate over an object and return IntegerVector indicating the data
// type of each element. Compatible with these rapidjson objects: Document,
// Array, Value.
template<typename T>
IntegerVector get_dtypes(T& doc) {
  int doc_len = doc.Size();
  IntegerVector out(doc_len);
  int curr_dtype;
  for(int i = 0; i < doc_len; ++i) {
    curr_dtype = doc[i].GetType();
    if(curr_dtype == 2) {
      curr_dtype = 1;
    }
    if(curr_dtype == 6) {
      if(doc[i].IsDouble()) {
        curr_dtype = 8;
      } else {
        curr_dtype = 9;
      }
    }

    out[i] = curr_dtype;
  }

  return out;
}


// Dump objects from a rapidjson array to an R list.
// Can handle both named and unnamed JSON objects.
template<typename T>
List array_to_list(T& array, int& array_len) {
  List out(array_len);
  for(int i = 0; i < array_len; ++i) {
    switch(array[i].GetType()) {
    // bool - false
    case 1: {
      out[i] = array[i].GetBool();
      break;
    }

    // bool - true
    case 2: {
      out[i] = array[i].GetBool();
      break;
    }

    // string
    case 5: {
      out[i] = array[i].GetString();
      break;
    }

    // numeric
    case 6: {
      if(array[i].IsDouble()) {
        // double
        out[i] = array[i].GetDouble();
      } else {
        // int
        out[i] = array[i].GetInt();
      }
      break;
    }

    // array
    case 4: {
      int curr_array_len = array[i].Size();
      T curr_array = array[i].GetArray();
      out[i] = array_to_list<T>(curr_array, curr_array_len);
      break;
    }

    // some other data type not covered
    default: {
      stop("Uknown data type. Only able to parse int, double, string, bool");
    }

    }
  }

  return out;
}


// Parse an array object, return an SEXP that contains the objects from the
// array.
// Can handle both named and unnamed JSON objects.
template<typename T>
SEXP parse_array(T& array) {
  bool list_out = false;
  int array_len = array.Size();
  int data_type = array[0].GetType();

  if(data_type == 2) {
    data_type = 1;
  }

  if(data_type == 6) {
    if(array[0].IsDouble()) {
      data_type = 8;
    } else {
      data_type = 9;
    }
  }

  // Check to see if the array has different data types, which means return
  // will be a list. Exception to this rule is for bool types...false has
  // data type of 1, and true has data type of 2. So, effectively, data types
  // 1 and 2 are identical (since they're both bool).
  int curr_dtype;
  for(int i = 1; i < array_len; ++i) {
    curr_dtype = array[i].GetType();
    if(curr_dtype == 2) {
      curr_dtype = 1;
    }
    if(curr_dtype == 6) {
      if(array[i].IsDouble()) {
        curr_dtype = 8;
      } else {
        curr_dtype = 9;
      }
    }
    if(curr_dtype != data_type) {
      list_out = true;
      break;
    }
  }

  // If the values in the input array are not all of the same data type,
  // return the array values as an R list.
  if(list_out) {
    return array_to_list<T>(array, array_len);
  }

  // Get current value
  switch(data_type) {

  // bool
  case 1: {
    LogicalVector out(array_len);
    for(int i = 0; i < array_len; ++i) {
      out[i] = array[i].GetBool();
    }
    return out;
  }

  // string
  case 5: {
    CharacterVector out(array_len);
    for(int i = 0; i < array_len; ++i) {
      out[i] = array[i].GetString();
    }
    return out;
  }

  // double
  case 8: {
    NumericVector out(array_len);
    for(int i = 0; i < array_len; ++i) {
      out[i] = array[i].GetDouble();
    }
    return out;
  }

  // int
  case 9: {
    IntegerVector out(array_len);
    for(int i = 0; i < array_len; ++i) {
      out[i] = array[i].GetInt();
    }
    return out;
  }

  // array
  case 4: {
    List out(array_len);
    for(int i = 0; i < array_len; ++i) {
      T curr_array = array[i].GetArray();
      out[i] = parse_array<T>(curr_array);
    }
    return out;
  }
  }

  return R_NilValue;
}


// Parse rapidjson::Value object.
List parse_value(const rapidjson::Value& val) {
  int json_len = val.Size();
  List out(json_len);
  CharacterVector names(json_len);

  int i = 0;
  for (rapidjson::Value::ConstMemberIterator itr = val.MemberBegin(); itr != val.MemberEnd(); ++itr) {

    // Get current key
    names[i] = itr->name.GetString();

    // Get current value
    switch(itr->value.GetType()) {

    // bool - false
    case 1: {
      out[i] = itr->value.GetBool();
      break;
    }

    // bool - true
    case 2: {
      out[i] = itr->value.GetBool();
      break;
    }

    // string
    case 5: {
      out[i] = itr->value.GetString();
      break;
    }

    // numeric
    case 6: {
      if(itr->value.IsDouble()) {
        // double
        out[i] = itr->value.GetDouble();
      } else {
        // int
        out[i] = itr->value.GetInt();
      }
      break;
    }

    // array
    case 4: {
      rapidjson::Value::ConstArray curr_array = itr->value.GetArray();
      out[i] = parse_array<rapidjson::Value::ConstArray>(curr_array);
      break;
    }

    // json
    case 3: {
      out[i] = parse_value(itr->value);
      break;
    }

    // some other data type not covered
    default: {
      stop("Uknown data type. Only able to parse int, double, string, bool");
    }
    }

    // Bump i
    i++;
  }

  out.attr("names") = names;
  return out;
}


// Parse rapidjson::Document object.
List parse_document(rapidjson::Document& doc) {
  int json_len = doc.Size();
  List out(json_len);
  CharacterVector names(json_len);

  int i = 0;
  for(rapidjson::Value::ConstMemberIterator itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {

    // Get current key
    names[i] = itr->name.GetString();

    // Get current value
    switch(itr->value.GetType()) {

    // bool - false
    case 1: {
      out[i] = itr->value.GetBool();
      break;
    }

    // bool - true
    case 2: {
      out[i] = itr->value.GetBool();
      break;
    }

    // string
    case 5: {
      out[i] = itr->value.GetString();
      break;
    }

    // numeric
    case 6: {
      if(itr->value.IsDouble()) {
        // double
        out[i] = itr->value.GetDouble();
      } else {
        // int
        out[i] = itr->value.GetInt();
      }
      break;
    }

    // array
    case 4: {
      rapidjson::Value::ConstArray curr_array = itr->value.GetArray();
      out[i] = parse_array<rapidjson::Value::ConstArray>(curr_array);
      break;
    }

    // json
    case 3: {
      const rapidjson::Value& temp_val = itr->value;
      out[i] = parse_value(temp_val);
      break;
    }

    // some other data type not covered
    default: {
      stop("Uknown data type. Only able to parse int, double, string, bool");
    }
    }

    // Bump i
    i++;
  }

  out.attr("names") = names;
  return out;
}


// Parse rapidjson::Document object that contains unnamed JSON data of the
// same data type. Returns an R vector.
SEXP doc_to_vector(rapidjson::Document& doc, int& dtype) {
  int doc_len = doc.Size();

  // Get current value
  switch(dtype) {

  // bool
  case 1: {
    LogicalVector out(doc_len);
    for(int i = 0; i < doc_len; ++i) {
      out[i] = doc[i].GetBool();
    }
    return out;
  }

  // string
  case 5: {
    CharacterVector out(doc_len);
    for(int i = 0; i < doc_len; ++i) {
      out[i] = doc[i].GetString();
    }
    return out;
  }

  // double
  case 8: {
    NumericVector out(doc_len);
    for(int i = 0; i < doc_len; ++i) {
      out[i] = doc[i].GetDouble();
    }
    return out;
  }

  // int
  case 9: {
    IntegerVector out(doc_len);
    for(int i = 0; i < doc_len; ++i) {
      out[i] = doc[i].GetInt();
    }
    return out;
  }
  }

  return R_NilValue;
}


// Parse rapidjson::Document object that contains unnamed JSON data that
// contains a variety of data types. Returns an R list.
List doc_to_list(rapidjson::Document& doc) {
  int doc_len = doc.Size();
  List out(doc_len);

  for(int i = 0; i < doc_len; ++i) {

    // Get current value
    switch(doc[i].GetType()) {

    // bool - false
    case 1: {
      out[i] = doc[i].GetBool();
      break;
    }

    // bool - true
    case 2: {
      out[i] = doc[i].GetBool();
      break;
    }

    // string
    case 5: {
      out[i] = doc[i].GetString();
      break;
    }

    // numeric
    case 6: {
      if(doc[i].IsDouble()) {
        // double
        out[i] = doc[i].GetDouble();
      } else {
        // int
        out[i] = doc[i].GetInt();
      }
      break;
    }

    // array
    case 4: {
      rapidjson::Value::Array curr_array = doc[i].GetArray();
      out[i] = parse_array<rapidjson::Value::Array>(curr_array);
      break;
    }

    // object (named json object).
    case 3: {
      const rapidjson::Value& temp_val = doc[i];
      out[i] = parse_value(temp_val);
      break;
    }

    // some other data type not covered
    default: {
      stop("Uknown data type. Only able to parse int, double, string, bool");
    }
    }
  }

  return out;
}


//' Parse JSON String
//'
//' Takes a JSON string as input, returns an R list of key-value pairs
//'
//' @param json const char, JSON string to be parsed. Coming from R, this
//'  input should be a character vector of length 1.
//' @export
// [[Rcpp::export]]
SEXP from_json(const char * json) {
  rapidjson::Document doc;
  doc.Parse(json);

  // Make sure there were no parse errors
  if(doc.HasParseError()) {
    Rcerr << "parse error for json string: "<< json << std::endl;
    stop("json parse error");
  }

  // If input is not an array, pass doc through parse_document(), and return
  // the result.
  if(!doc.IsArray()) {
    return parse_document(doc);
  }

  // If input in an array, first check the data type of each value in the
  // array.
  IntegerVector dtypes = get_dtypes<rapidjson::Document>(doc);
  IntegerVector u_dtypes = unique(dtypes);

  // If all of the data types are simple and of the same type
  // (i.e. all strings, ints, double, or bool), then return a vector of values.
  if(u_dtypes.size() == 1) {
    int dt = u_dtypes[0];
    if(dt == 1 || dt == 5 || dt == 8 || dt == 9) {
      return doc_to_vector(doc, dt);
    }
  }

  // Otherwise, if there's a variety of data types, return a list of values.
  return doc_to_list(doc);
}
