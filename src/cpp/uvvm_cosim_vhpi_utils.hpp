#pragma once
#include <stdexcept>
#include <string>
#include <cstring>
#include <vhpi_user.h>

static inline std::string get_vhpi_str_param_by_index(const vhpiCbDataT* p_cb_data, int param_index)
{
  // String buffer size is pretty large to accomodate
  // BFM config strings that may get pretty long
  constexpr size_t C_MAX_STR_SIZE = 1024;

  vhpiHandleT h_param = vhpi_handle_by_index(vhpiParamDecls,
					     p_cb_data->obj,
					     param_index);
  vhpiCharT str_buff[C_MAX_STR_SIZE];
  vhpiValueT vhpi_val = {.format = vhpiStrVal};
  vhpi_val.bufSize = sizeof(str_buff);
  vhpi_val.value.str = str_buff;

  if (vhpi_get_value(h_param, &vhpi_val) != 0) {
    vhpi_printf("Failed to get param index %d as str", param_index);
    throw std::runtime_error(std::string("VHPI error: Failed to get parameter index ")
			     + std::to_string(param_index)
			     + std::string(" as str"));
  }

  // Note:
  // vhpiCharT is defined as unsigned char and std::string
  // doesn't have constructor for uchar

  return std::string(reinterpret_cast<char *>(vhpi_val.value.str));
}

static inline int get_vhpi_int_param_by_index(const vhpiCbDataT* p_cb_data, int param_index)
{
  vhpiHandleT h_param = vhpi_handle_by_index(vhpiParamDecls,
					     p_cb_data->obj,
					     param_index);
  vhpiValueT vhpi_val = {.format = vhpiIntVal};

  if (vhpi_get_value(h_param, &vhpi_val) != 0) {
    vhpi_printf("Failed to get param index %d as int", param_index);
    throw std::runtime_error(std::string("VHPI error: Failed to get parameter index ")
			     + std::to_string(param_index)
			     + std::string(" as int"));
  }

  return vhpi_val.value.intg;
}

static inline void return_vhpi_int(const vhpiCbDataT* p_cb_data, int value)
{
  vhpiValueT ret_val = {
    .format = vhpiIntVal,
    .value = { .intg = value }
  };
  vhpi_put_value(p_cb_data->obj, &ret_val, vhpiDeposit);
}

static inline void check_foreignf_registration(const vhpiHandleT& h, const char* func_name, vhpiForeignKindT kind)
{
  vhpiForeignDataT check;

  if (vhpi_get_foreignf_info(h, &check)) {
    vhpi_printf("vhpi_get_foreignf_info failed");
  }

  if (check.kind != kind) {
    vhpi_printf("Wrong kind registered for foreign function");
  }

  if (strcmp(check.modelName, func_name) != 0) {
    vhpi_printf("Wrong model name registered for foreign function");
  }

  // TODO:
  // Throw an exception if registration fails?
}

typedef void (*vhpi_cb_func_t)(const struct vhpiCbDataS *cb_data_p);

static inline void register_vhpi_foreign_method(vhpi_cb_func_t func,
                                                const char *func_name,
                                                const char *lib_name,
                                                const vhpiForeignKindT &kind) {
  // Non-const copies of these strings since libraryName and modelName
  // are annoyingly declared non-const in vhpiForeignDataT.  NVC does
  // make copies of these (so could have just re-casted to non-const,
  // but that's kind of nasty). But may run into issues with other
  // simulators here if they don't make copies of these strings...
  char func_name_non_const[strlen(func_name)+1];
  char lib_name_non_const[strlen(lib_name)+1];

  strcpy(func_name_non_const, func_name);
  strcpy(lib_name_non_const, lib_name);

  vhpiForeignDataT foreignData = {
    .kind        = kind,
    .libraryName = lib_name_non_const,
    .modelName   = func_name_non_const,
    .elabf       = NULL,
    .execf       = func
  };

  vhpiHandleT h = vhpi_register_foreignf(&foreignData);
  check_foreignf_registration(h, func_name, kind);
}
