#include "cdbaconfig.h"
#include "cdbalib.h"
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>

const char* cdba_config_parse (void* settings, const char* config, struct cdba_config_settings_mapping_struct* mapping)
{
  const char* p;
  const char* varname;
  int varnamelen;
  const char* value;
  int valuelen;
  int quoted;
  const char* q;
  char* r;
  char* rawvalue;
  const struct cdba_config_settings_mapping_struct* cfgmap;
  if (!settings || !config)
    return NULL;
  p = config;
  while (*p) {
    //skip whitespace
    while (*p && (isspace(*p) || *p == ';'))
      p++;
    if (!*p)
      break;
    //determine variable name
    varname = p;
    while (*p && *p != '=' && !isspace(*p))
      p++;
    varnamelen = p - varname;
    //skip to value
    while (*p && isspace(*p))
      p++;
    if (*p != '=')
      return p;
    p++;
    //determine value
    value = p;
    quoted = 0;
    while (*p && (quoted || (!isspace(*p) && *p != ';'))) {
      if (*p == '"') {
        quoted = ~quoted;
      } else if (quoted && *p == '\\') {
        if (*(p + 1))
          p++;
      }
      p++;
    }
    valuelen = p - value;
    //unquote/escape value
    if ((rawvalue = (char*)malloc(valuelen + 1)) == NULL)
      return value;
    q = value;
    r = rawvalue;
    quoted = 0;
    while (q != p) {
      if (*q == '"') {
        quoted = ~quoted;
        q++;
        continue;
      } else if (quoted && *q == '\\') {
        if (*(q + 1))
          q++;
      }
      *r++ = *q++;
    }
    *r = 0;
    //find mapping to structure member
    cfgmap = mapping;
    while (cfgmap->config_name) {
      if (strncmp(cfgmap->config_name, varname, varnamelen) == 0 && cfgmap->config_name[varnamelen] == 0)
        break;
      cfgmap++;
    }
    if (!cfgmap->config_name)
      return varname;
    //store value in structure member
    switch (cfgmap->type) {
      case cfg_txt:
        if (*(char**)&(((int8_t*)settings)[cfgmap->settings_offset]))
          free(*(char**)&(((int8_t*)settings)[cfgmap->settings_offset]));
        *(char**)&(((int8_t*)settings)[cfgmap->settings_offset]) = (rawvalue ? strdup(rawvalue) : NULL);
        break;
      case cfg_int:
        *(db_int*)&(((int8_t*)settings)[cfgmap->settings_offset]) = strtoimax(rawvalue, NULL, 10);
        break;
      case cfg_flt:
        *(db_flt*)&(((int8_t*)settings)[cfgmap->settings_offset]) = strtod(rawvalue, NULL);
        break;
      default:
        break;
    }
    //clean up unquoted/escaped value
    free(rawvalue);
  }
  return NULL;
}
