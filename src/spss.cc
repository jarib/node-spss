#include <assert.h>
#include <node_api.h>

#include <iostream>
#include "spssdio.h"

#define LOG(...) fprintf(stderr, __VA_ARGS__)

bool assert_spss_(napi_env env, int status, const char* file, int line) {
  if (status != SPSS_OK) {
    size_t size = 100;
    char buffer[size];
    snprintf(buffer, size, "SPSS ERROR %d at %s:%d\n", status, file, line);

    napi_throw_type_error(env, nullptr, buffer);
  }

  return status == SPSS_OK;
}

#define assert_spss(env, status) assert_spss_(env, status, __FILE__, __LINE__)

bool add_string_value_labels(napi_env env, napi_value variable_obj, int handle,
                             char* variable_name) {
  int status;
  char** variable_values;
  char** variable_value_labels;
  int num_value_labels = 0;

  status = spssGetVarCValueLabels(handle, variable_name, &variable_values,
                                  &variable_value_labels, &num_value_labels);

  if (status != SPSS_OK && status != SPSS_NO_LABELS) {
    assert_spss(env, status);
    return false;
  }

  if (num_value_labels > 0) {
    napi_value napi_variable_value_list;
    status = napi_create_array_with_length(env, num_value_labels,
                                           &napi_variable_value_list);
    assert(status == napi_ok);

    for (int vi = 0; vi < num_value_labels; ++vi) {
      napi_value napi_variable_value;
      status = napi_create_string_utf8(env, variable_values[vi],
                                       NAPI_AUTO_LENGTH, &napi_variable_value);
      assert(status == napi_ok);

      napi_value napi_variable_label;
      status = napi_create_string_utf8(env, variable_value_labels[vi],
                                       NAPI_AUTO_LENGTH, &napi_variable_label);
      assert(status == napi_ok);

      napi_value variable_value_obj;
      status = napi_create_object(env, &variable_value_obj);
      assert(status == napi_ok);

      status = napi_set_named_property(env, variable_value_obj, "value",
                                       napi_variable_value);
      assert(status == napi_ok);

      status = napi_set_named_property(env, variable_value_obj, "label",
                                       napi_variable_label);
      assert(status == napi_ok);

      status = napi_set_element(env, napi_variable_value_list, vi,
                                variable_value_obj);
      assert(status == napi_ok);
    }

    status = napi_set_named_property(env, variable_obj, "valueLabels",
                                     napi_variable_value_list);
    assert(status == napi_ok);
  }

  spssFreeVarCValueLabels(variable_values, variable_value_labels,
                          num_value_labels);
  return true;
}

bool add_numeric_value_labels(napi_env env, napi_value variable_obj, int handle,
                              char* variable_name) {
  int status;
  double* variable_values;
  char** variable_value_labels;
  int num_value_labels = 0;

  status = spssGetVarNValueLabels(handle, variable_name, &variable_values,
                                  &variable_value_labels, &num_value_labels);

  if (status != SPSS_OK && status != SPSS_NO_LABELS) {
    assert_spss(env, status);
    return false;
  }

  if (num_value_labels > 0) {
    napi_value napi_variable_value_list;
    status = napi_create_array_with_length(env, num_value_labels,
                                           &napi_variable_value_list);
    assert(status == napi_ok);

    for (int vi = 0; vi < num_value_labels; ++vi) {
      napi_value napi_variable_value;
      status =
          napi_create_double(env, variable_values[vi], &napi_variable_value);
      assert(status == napi_ok);

      napi_value napi_variable_label;
      status = napi_create_string_utf8(env, variable_value_labels[vi],
                                       NAPI_AUTO_LENGTH, &napi_variable_label);
      assert(status == napi_ok);

      napi_value variable_value_obj;
      status = napi_create_object(env, &variable_value_obj);
      assert(status == napi_ok);

      status = napi_set_named_property(env, variable_value_obj, "value",
                                       napi_variable_value);
      assert(status == napi_ok);

      status = napi_set_named_property(env, variable_value_obj, "label",
                                       napi_variable_label);
      assert(status == napi_ok);

      status = napi_set_element(env, napi_variable_value_list, vi,
                                variable_value_obj);
      assert(status == napi_ok);
    }

    status = napi_set_named_property(env, variable_obj, "valueLabels",
                                     napi_variable_value_list);
    assert(status == napi_ok);
  }

  spssFreeVarNValueLabels(variable_values, variable_value_labels,
                          num_value_labels);
  return true;
}

napi_value convert_file(napi_env env, char* file_path) {
  int status;

  napi_value result;
  status = napi_create_object(env, &result);
  assert(status == napi_ok);

  napi_value variables;
  status = napi_create_object(env, &variables);
  assert(status == napi_ok);

  status = napi_set_named_property(env, result, "variables", variables);
  assert(status == napi_ok);

  // read file

  status = spssSetInterfaceEncoding(SPSS_ENCODING_UTF8);
  if (!assert_spss(env, status)) return nullptr;

  int handle;
  status = spssOpenRead(file_path, &handle);
  if (!assert_spss(env, status)) return nullptr;

  // read variable names and labels
  int number_of_variables;
  int* variable_types;
  char** variable_names;

  status = spssGetVarNames(handle, &number_of_variables, &variable_names,
                           &variable_types);
  if (!assert_spss(env, status)) return nullptr;

  napi_value napi_variable_list;
  status = napi_create_array_with_length(env, number_of_variables,
                                         &napi_variable_list);
  assert(status == napi_ok);

  double variable_handles[number_of_variables];

  LOG("Reading %d variables\n", number_of_variables);

  // loop through and get variable metadata
  for (int i = 0; i < number_of_variables; ++i) {
    status = spssGetVarHandle(handle, variable_names[i], &variable_handles[i]);
    if (!assert_spss(env, status)) return nullptr;

    // create an object per variable
    napi_value variable_obj;
    status = napi_create_object(env, &variable_obj);
    assert(status == napi_ok);

    // get all attributes
    char** attr_names;
    char** attr_values;
    int num_attrs = 0;

    status = spssGetVarAttributes(handle, variable_names[i], &attr_names,
                                  &attr_values, &num_attrs);
    if (!assert_spss(env, status)) return nullptr;

    if (num_attrs > 0) {
      // attributes
      napi_value attrs;
      status = napi_create_object(env, &attrs);
      assert(status == napi_ok);

      for (int ai = 0; ai < num_attrs; ++ai) {
        napi_value napi_attr_value;
        status = napi_create_string_utf8(env, attr_values[ai], NAPI_AUTO_LENGTH,
                                         &napi_attr_value);
        assert(status == napi_ok);

        status = napi_set_named_property(env, attrs, attr_names[ai],
                                         napi_attr_value);
        assert(status == napi_ok);
      }

      status = napi_set_named_property(env, variable_obj, "attributes", attrs);
      assert(status == napi_ok);
    }

    status = spssFreeAttributes(attr_names, attr_values, num_attrs);
    assert(status == napi_ok);

    // variable name

    napi_value napi_variable_name;
    status = napi_create_string_utf8(env, variable_names[i], NAPI_AUTO_LENGTH,
                                     &napi_variable_name);
    assert(status == napi_ok);

    status =
        napi_set_named_property(env, variable_obj, "name", napi_variable_name);
    assert(status == napi_ok);

    // variable label - optional

    char variable_label[SPSS_MAX_VARLABEL + 1];
    int variable_label_len;

    status = spssGetVarLabelLong(handle, variable_names[i], variable_label,
                                 sizeof(variable_label), &variable_label_len);

    if (status != SPSS_NO_LABEL) {
      if (!assert_spss(env, status)) return nullptr;

      napi_value napi_variable_label;
      status = napi_create_string_utf8(env, variable_label, variable_label_len,
                                       &napi_variable_label);
      assert(status == napi_ok);

      status = napi_set_named_property(env, variable_obj, "label",
                                       napi_variable_label);
      assert(status == napi_ok);
    }

    // variable type

    napi_value napi_variable_type;
    status = napi_create_string_utf8(
        env, variable_types[i] == 0 ? "numeric" : "string", NAPI_AUTO_LENGTH,
        &napi_variable_type);
    assert(status == napi_ok);

    status =
        napi_set_named_property(env, variable_obj, "type", napi_variable_type);
    assert(status == napi_ok);

    // get value labels
    bool value_labels_status;
    if (variable_types[i] > 0) {  // string
      value_labels_status =
          add_string_value_labels(env, variable_obj, handle, variable_names[i]);
    } else {  // numeric value labels
      value_labels_status = add_numeric_value_labels(env, variable_obj, handle,
                                                     variable_names[i]);
    }

    if (!value_labels_status) return nullptr;

    // done with this var – append the variable object to the list
    status = napi_set_element(env, napi_variable_list, i, variable_obj);
    assert(status == napi_ok);

    LOG(".");
  }

  LOG("\n");

  napi_value napi_variable_count;
  status = napi_create_int32(env, number_of_variables, &napi_variable_count);
  assert(status == napi_ok);

  status =
      napi_set_named_property(env, variables, "count", napi_variable_count);
  assert(status == napi_ok);

  status = napi_set_named_property(env, variables, "data", napi_variable_list);
  assert(status == napi_ok);

  long num_cases;
  status = spssGetNumberofCases(handle, &num_cases);
  if (!assert_spss(env, status)) return nullptr;

  long case_idx;

  napi_value napi_cases;
  status = napi_create_array_with_length(env, num_cases, &napi_cases);

  LOG("Reading %ld cases\n", num_cases);

  for (case_idx = 1; case_idx <= num_cases; ++case_idx) {
    status = spssReadCaseRecord(handle);
    if (!assert_spss(env, status)) return nullptr;

    napi_value napi_case;
    status =
        napi_create_array_with_length(env, number_of_variables, &napi_case);
    assert(status == napi_ok);

    for (int var_idx = 0; var_idx < number_of_variables; ++var_idx) {
      if (variable_types[var_idx] == 0) {
        // numeric

        double n_value;

        status =
            spssGetValueNumeric(handle, variable_handles[var_idx], &n_value);
        if (!assert_spss(env, status)) return nullptr;

        napi_value napi_n_value;
        status = napi_create_double(env, n_value, &napi_n_value);
        assert(status == napi_ok);

        status = napi_set_element(env, napi_case, var_idx, napi_n_value);
        assert(status == napi_ok);
      } else {
        // string
        size_t size = SPSS_MAX_LONGSTRING + 1;
        char c_value[size];

        status =
            spssGetValueChar(handle, variable_handles[var_idx], c_value, size);
        if (!assert_spss(env, status)) return nullptr;

        napi_value napi_c_value;
        status = napi_create_string_utf8(env, c_value, NAPI_AUTO_LENGTH,
                                         &napi_c_value);
        assert(status == napi_ok);

        status = napi_set_element(env, napi_case, var_idx, napi_c_value);
        assert(status == napi_ok);
      }
    }

    status = napi_set_element(env, napi_cases, case_idx, napi_case);
    assert(status == napi_ok);

    LOG(".");
  }

  status =
      spssFreeVarNames(variable_names, variable_types, number_of_variables);
  if (!assert_spss(env, status)) return nullptr;

  status = spssCloseRead(handle);
  if (!assert_spss(env, status)) return nullptr;

  // done with file
  status = napi_set_named_property(env, result, "cases", napi_cases);
  assert(status == napi_ok);

  return result;
}

napi_value Convert(napi_env env, napi_callback_info info) {
  napi_status status;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  assert(status == napi_ok);

  if (argc != 1) {
    napi_throw_type_error(
        env, nullptr, "Wrong number of arguments, expected path to .sav file");
    return nullptr;
  }

  napi_valuetype valuetype;
  status = napi_typeof(env, args[0], &valuetype);
  assert(status == napi_ok);

  if (valuetype != napi_string) {
    napi_throw_type_error(env, nullptr,
                          "Wrong argument type, expected path to .sav file");
    return nullptr;
  }

  size_t file_path_len = 0;

  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &file_path_len);
  assert(status == napi_ok);

  char str[file_path_len + 1];
  status = napi_get_value_string_utf8(env, args[0], str, sizeof(str),
                                      &file_path_len);
  assert(status == napi_ok);

  // TODO: make async?
  return convert_file(env, str);
}

#define DECLARE_NAPI_METHOD(name, func) \
  { name, 0, func, 0, 0, 0, napi_default, 0 }

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;
  napi_property_descriptor desc = DECLARE_NAPI_METHOD("convert", Convert);
  status = napi_define_properties(env, exports, 1, &desc);
  assert(status == napi_ok);
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)