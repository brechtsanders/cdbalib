#ifndef INCLUDED_CDBACONFIG_H
#define INCLUDED_CDBACONFIG_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum cdba_config_settings_type {
  cfg_int,
  cfg_flt,
  cfg_txt
};

struct cdba_config_settings_mapping_struct {
  const char* config_name;
  size_t settings_offset;
  enum cdba_config_settings_type type;
};

const char* cdba_config_parse (void* settings, const char* config, struct cdba_config_settings_mapping_struct* mapping);

#ifdef __cplusplus
}
#endif

#endif
