/* This file is part of s10sh
 *
 * Copyright (C) 2006 by Lex Augusteijn <lex.augusteijn@chello.nl>
 *
 * S10sh IS FREE SOFTWARE, UNDER THE TERMS OF THE GPL VERSION 2
 * don't forget what free software means, even if today is so diffused.
 *
 * Parameter settings, tested for 300D
 *
 * ALL THIRD PARTY BRAND, PRODUCT AND SERVICE NAMES MENTIONED ARE
 * THE TRADEMARK OR REGISTERED TRADEMARK OF THEIR RESPECTIVE OWNERS
 */

#include <stdio.h>
#include <string.h>

#include "s10sh.h"
#include "common.h"
#include "param.h"

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*a))

/* WARNING
   The tables below were extracted for the 300D.
   Other camera's might need other tables.
   If so, set the value for the static variable to that table.
   It might be useful to make a module per camera type that exports its top-level table.
   The code has been developed for little endianness only.
*/

/* Structure of parameter block.
   0x00 - 0x03	Size of block
   0x05 - 0x07	Quality
   0x0a 0x0c 0x0d Mode (M, Av, etc)
   0x0b		Sound
   0x0e		Drive
   0x12		AF points
   0x14		WB
   0x16		AF mode
   0x18 - 0x1c	Parameter (menu)
   0x1e		ISO
   0x20		Av
   0x21		Av defined
   0x22		Tv
   0x23		Tv defined
   0x24		EXP
   0x25		FEC
   0x26		AEB
*/
typedef unsigned char uchar;

/* Possible value of a parameter */
typedef struct param_value {
  char  *name;			/* Name of parameter value */
  int    num_values;		/* Number of values */
  int    usb_offset;		/* Offset in usb packet for single value */
  uchar  usb_value;		/* Single value */
  int   *usb_offsets;		/* Offsets in usb packet */
  uchar *usb_values;		/* Values in usb packet */
} *param_value_t;

/* TODO
   The field 'values' should be indexed by camera_model.
*/
/* Type representing a parameter */
typedef struct param {
  char *name;			/* Parameter name */
  camera_type *models;	/* Camera models supporting this parameter, 0 terminated */
  int num_values;		/* Number of values */
  param_value_t values;		/* Values */
  char **modes;			/* Modes in which this param can be changes, NULL means allmodes */
  uchar (*transform)(uchar,int); /* Value transformer */
  char *help;			/* Help text */
} *param_t;

char *current_camera_mode;

static camera_type eos_models[] = { EOS10D, DRebel, EOS20D, EOS350D, UNKOWN_CAMERA };

static struct param_value Iso_values[] = {
  { "100",  1, 0x1e, 0x48 },
  { "200",  1, 0x1e, 0x50 },
  { "400",  1, 0x1e, 0x58 },
  { "800",  1, 0x1e, 0x60 },
  { "1600", 1, 0x1e, 0x68 },
  { "3200", 1, 0x1e, 0x70 },
};

/* Offset for Tv is 0x22, 0x23 is set to 0xff if Tv is undefined */
static struct param_value Tv_values[] = {
  { "Bulb",    1, 0x22, 0x04 },
  { "30",      1, 0x22, 0x10 },
  { "25",      1, 0x22, 0x13 },
  { "20",      1, 0x22, 0x15 },
  { "15",      1, 0x22, 0x18 },
  { "13",      1, 0x22, 0x1b },
  { "10",      1, 0x22, 0x1d },
  { "8",       1, 0x22, 0x20 },
  { "6",       1, 0x22, 0x23 },
  { "5",       1, 0x22, 0x25 },
  { "4",       1, 0x22, 0x28 },
  { "3.2",     1, 0x22, 0x2b },
  { "2.5",     1, 0x22, 0x2d },
  { "2",       1, 0x22, 0x30 },
  { "1.6",     1, 0x22, 0x33 },
  { "1.3",     1, 0x22, 0x35 },
  { "1",       1, 0x22, 0x38 },
  { "0.8",     1, 0x22, 0x3b },
  { "0.6",     1, 0x22, 0x3d },
  { "0.5",     1, 0x22, 0x40 },
  { "0.4",     1, 0x22, 0x43 },
  { "0.3",     1, 0x22, 0x45 },
  { "1/4",     1, 0x22, 0x48 },
  { "1/5",     1, 0x22, 0x4b },
  { "1/6",     1, 0x22, 0x4d },
  { "1/8",     1, 0x22, 0x50 },
  { "1/10",    1, 0x22, 0x53 },
  { "1/13",    1, 0x22, 0x55 },
  { "1/15",    1, 0x22, 0x58 },
  { "1/20",    1, 0x22, 0x5b },
  { "1/25",    1, 0x22, 0x5d },
  { "1/30",    1, 0x22, 0x60 },
  { "1/40",    1, 0x22, 0x63 },
  { "1/50",    1, 0x22, 0x65 },
  { "1/60",    1, 0x22, 0x68 },
  { "1/80",    1, 0x22, 0x6b },
  { "1/100",   1, 0x22, 0x6d },
  { "1/125",   1, 0x22, 0x70 },
  { "1/160",   1, 0x22, 0x73 },
  { "1/200",   1, 0x22, 0x75 },
  { "1/250",   1, 0x22, 0x78 },
  { "1/320",   1, 0x22, 0x7b },
  { "1/400",   1, 0x22, 0x7d },
  { "1/500",   1, 0x22, 0x80 },
  { "1/640",   1, 0x22, 0x83 },
  { "1/800",   1, 0x22, 0x85 },
  { "1/1000",  1, 0x22, 0x88 },
  { "1/1250",  1, 0x22, 0x8b },
  { "1/1600",  1, 0x22, 0x8d },
  { "1/2000",  1, 0x22, 0x90 },
  { "1/2500",  1, 0x22, 0x93 },
  { "1/3200",  1, 0x22, 0x95 },
  { "1/4000",  1, 0x22, 0x98 },
};

/* Offset for Av is 0x20, 0x21 is set to 0xff if Av is undefined */
struct param_value Av_values[] = {
  { "1.8", 1, 0x20, 0x15 },
  { "2.0", 1, 0x20, 0x18 },
  { "2.2", 1, 0x20, 0x1b },
  { "2.5", 1, 0x20, 0x1d },
  { "2.8", 1, 0x20, 0x20 },
  { "3.2", 1, 0x20, 0x23 },
  { "3.5", 1, 0x20, 0x25 },
  { "4.0", 1, 0x20, 0x28 },
  { "4.5", 1, 0x20, 0x2b },
  { "5.0", 1, 0x20, 0x2d },
  { "5.6", 1, 0x20, 0x30 },
  { "6.3", 1, 0x20, 0x33 },
  { "7.1", 1, 0x20, 0x35 },
  { "8.0", 1, 0x20, 0x38 },
  { "9.0", 1, 0x20, 0x3b },
  { "10",  1, 0x20, 0x3d },
  { "11",  1, 0x20, 0x40 },
  { "13",  1, 0x20, 0x43 },
  { "14",  1, 0x20, 0x45 },
  { "16",  1, 0x20, 0x48 },
  { "18",  1, 0x20, 0x4b },
  { "20",  1, 0x20, 0x4d },
  { "22",  1, 0x20, 0x50 },
  { "25",  1, 0x20, 0x53 },
  { "29",  1, 0x20, 0x55 },
  { "32",  1, 0x20, 0x58 },
};

struct param_value WB_values[] = {
 { "Auto",     1, 0x14, 0x00  },
 { "Daylighy", 1, 0x14, 0x01  },
 { "Cloudy",   1, 0x14, 0x02  },
 { "Tungsten", 1, 0x14, 0x03  },
 { "Fluor",    1, 0x14, 0x04  },
 { "Flash",    1, 0x14, 0x05  },
 { "Shade",    1, 0x14, 0x08  },
};

struct param_value EXP_values[] = {
  { "2",    1, 0x24, 0x08 },
  { "5/3",  1, 0x24, 0x0b },
  { "4/3",  1, 0x24, 0x0d },
  { "1",    1, 0x24, 0x10 },
  { "2/3",  1, 0x24, 0x13 },
  { "1/3",  1, 0x24, 0x15 },
  { "0",    1, 0x24, 0x18 },
  { "-1/3", 1, 0x24, 0x1b },
  { "-2/3", 1, 0x24, 0x1d },
  { "-1",   1, 0x24, 0x20 },
  { "-4/3", 1, 0x24, 0x23 },
  { "-5/3", 1, 0x24, 0x25 },
  { "-2",   1, 0x24, 0x28 },
};

static uchar exp_trans (uchar val, int i)
{
  if (camera_model == EOS350D) return 0x18-val;
  else return val;
}

struct param_value FEC_values[] = {
  { "2",    1, 0x25, 0x08 },
  { "5/3",  1, 0x25, 0x0b },
  { "4/3",  1, 0x25, 0x0d },
  { "1",    1, 0x25, 0x10 },
  { "2/3",  1, 0x25, 0x13 },
  { "1/3",  1, 0x25, 0x15 },
  { "0",    1, 0x25, 0x18 },
  { "-1/3", 1, 0x25, 0x1b },
  { "-2/3", 1, 0x25, 0x1d },
  { "-1",   1, 0x25, 0x20 },
  { "-4/3", 1, 0x25, 0x23 },
  { "-5/3", 1, 0x25, 0x25 },
  { "-2",   1, 0x25, 0x28 },
};

struct param_value AEB_values[] = {
 { "0", 1, 0x26, 0x18 },
 { "1", 1, 0x26, 0x10 },
 { "2", 1, 0x26, 0x08 },
};

struct param_value AF_values[] = {
/* The offset and values seem correct, but they are not changed through the USB interface */
 { "OS", 1, 0x16, 0x00 },
 { "SE", 1, 0x16, 0x01 },
 { "AI", 1, 0x16, 0x02 },
};

static int   Par_offset[]       = { 0x18, 0x19, 0x1a, 0x1b, 0x1c }; 
static uchar Par_Par1_values[]  = { 0x01, 0x01, 0x01, 0X7f, 0x00 };	   
static uchar Par_Par2_values[]  = { 0x00, 0x00, 0x00, 0x7f, 0x05 };	   
static uchar Par_Adobe_values[] = { 0x0f, 0x0f, 0x0f, 0x7f, 0x04 };	   
static uchar Par_Set1_values[]  = { 0x00, 0x00, 0x00, 0x7f, 0x01 };	   
static uchar Par_Set2_values[]  = { 0x00, 0x00, 0x00, 0x7f, 0x02 };	   
static uchar Par_Set3_values[]  = { 0x00, 0x00, 0x00, 0x7f, 0x03 };	   

static struct param_value Par_values[] = {
  { "Par-1", ARRAY_SIZE(Par_offset), 0, 0, Par_offset, Par_Par1_values  },	    
  { "Par-2", ARRAY_SIZE(Par_offset), 0, 0, Par_offset, Par_Par2_values  },	    
  { "Adobe", ARRAY_SIZE(Par_offset), 0, 0, Par_offset, Par_Adobe_values },
  { "Set-1", ARRAY_SIZE(Par_offset), 0, 0, Par_offset, Par_Set1_values  },	    
  { "Set-2", ARRAY_SIZE(Par_offset), 0, 0, Par_offset, Par_Set2_values  },	    
  { "Set-3", ARRAY_SIZE(Par_offset), 0, 0, Par_offset, Par_Set3_values  },	    
};

static struct param_value Drive_values[] = {
  { "Single",      1, 0x0e, 0x00 },
  { "Continuous",  1, 0x0e, 0x01 },
  { "Timer",       1, 0x0e, 0x02 },
};

static int   Quality_offset[]     = { 0x05, 0x06, 0x07 };
static uchar Quality_L_values[]   = { 0x02, 0x01, 0x00 };       
static uchar Quality_Lf_values[]  = { 0x03, 0x01, 0x00 };       
static uchar Quality_M_values[]   = { 0x02, 0x01, 0x01 };       
static uchar Quality_Mf_values[]  = { 0x03, 0x01, 0x01 };       
static uchar Quality_S_values[]   = { 0x02, 0x01, 0x02 };       
static uchar Quality_Sf_values[]  = { 0x03, 0x01, 0x02 };       
static uchar Quality_Raw_values[] = { 0x04, 0x02, 0x00 };       

static struct param_value Quality_values[] = {
  { "L",   ARRAY_SIZE(Quality_offset), 0, 0, Quality_offset, Quality_L_values   },    
  { "Lf",  ARRAY_SIZE(Quality_offset), 0, 0, Quality_offset, Quality_Lf_values  },    
  { "M",   ARRAY_SIZE(Quality_offset), 0, 0, Quality_offset, Quality_M_values   },    
  { "Mf",  ARRAY_SIZE(Quality_offset), 0, 0, Quality_offset, Quality_Mf_values  },    
  { "S",   ARRAY_SIZE(Quality_offset), 0, 0, Quality_offset, Quality_S_values   },    
  { "Sf",  ARRAY_SIZE(Quality_offset), 0, 0, Quality_offset, Quality_Sf_values  },    
  { "Raw", ARRAY_SIZE(Quality_offset), 0, 0, Quality_offset, Quality_Raw_values },    
};

static struct param_value Timer_values[] = {
};

static struct param_value Sound_values[] = {
  { "Off", 1, 0x0b, 0x00 },
  { "On",  1, 0x0b, 0x01 },
};

static struct param_value AFP_values[] = {
  { "0", 1, 0x12, 0x01 },
  { "1", 1, 0x12, 0x06 },
  { "2", 1, 0x12, 0x04 },
  { "3", 1, 0x12, 0x07 },
  { "4", 1, 0x12, 0x03 },
  { "5", 1, 0x12, 0x08 },
  { "6", 1, 0x12, 0x02 },
  { "7", 1, 0x12, 0x05 },
};

static struct param_value Meter_values[] = {
  { "Eval",    1, 0x10, 0x03 },
  { "Partial", 1, 0x10, 0x04 },
  { "Center",  1, 0x10, 0x05 },
};

static int   Mode_offset[]           = { /*0x0a,*/ 0x0c, 0x0d };
static uchar Mode_P_values[]         = { /*0x00,*/ 0x01, 0x01 };	   
static uchar Mode_Tv_values[]        = { /*0x00,*/ 0x02, 0x01 };	   
static uchar Mode_Av_values[]        = { /*0x00,*/ 0x03, 0x01 };	   
static uchar Mode_M_values[]         = { /*0x00,*/ 0x04, 0x01 };	   
static uchar Mode_A_DEP_values[]     = { /*0x00,*/ 0x05, 0x01 };	   
static uchar Mode_Auto_values[]      = { /*0x05,*/ 0x00, 0x00 };	   
static uchar Mode_Portrait_values[]  = { /*0x05,*/ 0x00, 0x08 };	   
static uchar Mode_Landscape_values[] = { /*0x05,*/ 0x00, 0x02 };	   
static uchar Mode_Close_up_values[]  = { /*0x05,*/ 0x00, 0x0a };	   
static uchar Mode_Sports_values[]    = { /*0x05,*/ 0x00, 0x09 };	   
static uchar Mode_Night_values[]     = { /*0x05,*/ 0x00, 0x05 };	   
static uchar Mode_Flash_off_values[] = { /*0x00,*/ 0x00, 0x00 };	   

static struct param_value Mode_values[] = {
 { "P",         ARRAY_SIZE(Mode_offset), 0, 0, Mode_offset, Mode_P_values },
 { "Tv",        ARRAY_SIZE(Mode_offset), 0, 0, Mode_offset, Mode_Tv_values },
 { "Av",        ARRAY_SIZE(Mode_offset), 0, 0, Mode_offset, Mode_Av_values },
 { "M",         ARRAY_SIZE(Mode_offset), 0, 0, Mode_offset, Mode_M_values },
 { "A-DEP",     ARRAY_SIZE(Mode_offset), 0, 0, Mode_offset, Mode_A_DEP_values },
 { "Auto",      ARRAY_SIZE(Mode_offset), 0, 0, Mode_offset, Mode_Auto_values },
 { "Portrait",  ARRAY_SIZE(Mode_offset), 0, 0, Mode_offset, Mode_Portrait_values },
 { "Landscape", ARRAY_SIZE(Mode_offset), 0, 0, Mode_offset, Mode_Landscape_values },
 { "Close-up",  ARRAY_SIZE(Mode_offset), 0, 0, Mode_offset, Mode_Close_up_values },
 { "Sports",    ARRAY_SIZE(Mode_offset), 0, 0, Mode_offset, Mode_Sports_values },
 { "Night",     ARRAY_SIZE(Mode_offset), 0, 0, Mode_offset, Mode_Night_values },
 { "Flash-off", ARRAY_SIZE(Mode_offset), 0, 0, Mode_offset, Mode_Flash_off_values },
};

#define BASIC_MODES 	"Auto", "Portrait", "Landscape", "Close-up", "Sports", "Night", "Flash-off"
#define CREATIVE_MODES	"P", "Tv", "Av", "M", "A-DEP"
#define ALL_MODES NULL
#define all_modes NULL

static char *no_modes[]       = { NULL };
static char *basic_modes[]    = { BASIC_MODES, NULL };
static char *creative_modes[] = { CREATIVE_MODES, NULL };
static char *tv_modes[]       = { "P", "Tv", "M", "A-DEP", NULL };
static char *av_modes[]       = { "Av", "M", NULL };
static char *exp_modes[]      = { "P", "Tv", "Av", "A-DEP", NULL };

static struct param canon_params[] = {
  { "ISO",         eos_models, ARRAY_SIZE(Iso_values),     Iso_values,     creative_modes, NULL, NULL },			    
  { "Tv",          eos_models, ARRAY_SIZE(Tv_values),      Tv_values,      tv_modes,	   NULL, "Shutter speed" },		    
  { "Av",          eos_models, ARRAY_SIZE(Av_values),      Av_values,      av_modes,	   NULL, "Aperture" },		    
  { "WB",          eos_models, ARRAY_SIZE(WB_values),      WB_values,      all_modes,	   NULL, "White balance" },		    
  { "EXP",         eos_models, ARRAY_SIZE(EXP_values),     EXP_values,     exp_modes,	   exp_trans, "Exposure compensation" },	    
  { "FEC",         eos_models, ARRAY_SIZE(FEC_values),     FEC_values,     creative_modes, exp_trans, "Flash exposure compensation" }, 
  { "AEB",         eos_models, ARRAY_SIZE(AEB_values),     AEB_values,     creative_modes, exp_trans, "Auto exposure bracketing" },    
  { "AF",          eos_models, ARRAY_SIZE(AF_values),      AF_values,      creative_modes, NULL, "Auto focus mode" }, 	    
  { "Parameters",  eos_models, ARRAY_SIZE(Par_values),     Par_values,     all_modes,	   NULL, "Menu parameters" }, 	    
  { "Drive",       eos_models, ARRAY_SIZE(Drive_values),   Drive_values,   all_modes,	   NULL, "Drive mode" },		    
  { "Mode",        eos_models, ARRAY_SIZE(Mode_values),    Mode_values,    no_modes,	   NULL, "Shooting mode" },		    
  { "Quality",     eos_models, ARRAY_SIZE(Quality_values), Quality_values, all_modes,	   NULL, "Image quality" },		    
  { "Timer",       eos_models, ARRAY_SIZE(Timer_values),   Timer_values,   no_modes,	   NULL, "Self timer value" },	    
  { "Sound",       eos_models, ARRAY_SIZE(Sound_values),   Sound_values,   all_modes,	   NULL, "Sound" },			    
  { "AF-point",    eos_models, ARRAY_SIZE(AFP_values),     AFP_values,     no_modes,	   NULL, "AF point" },		    
  { "Metering",    eos_models, ARRAY_SIZE(Meter_values),   Meter_values,   creative_modes, NULL, "Metering mode" },		    
};

static struct param *params = canon_params;
static int      params_size = ARRAY_SIZE(canon_params);

static param_t 
find_param (char *name)
{
  int i;
  if (!name) return NULL;
  for (i = 0; i < params_size; i++)
  {
    if (!strcmp (name, params[i].name)) return &params[i];
  }
  return NULL;
}

static int 
compatible_with_mode (char *name)
{
  param_t p = find_param (name);
  if (!p) return 0;
  
  char **modes = p->modes;
  if (modes == all_modes) return 1;
  
  for (; *modes; modes++) {
    if (strcmp (*modes, current_camera_mode)==0) return 1;
  }
  return 0;
}

static int 
param_supported (param_t p)
{
  camera_type *m;
  for (m = p->models; m && *m; m++) {
    if (*m == camera_model) return 1;
  }
  return 0;
}

enum param_result
get_param_value (char *name, char **pvalue, char *usb_buffer)
{
  param_t p = find_param (name);
  param_value_t pv;
  unsigned char value;
  int i, j;
  
  if (!p) return PARAM_NO_SUCH_NAME;
  if (!param_supported(p)) return PARAM_NOT_SUPPORTED;
  
  for (i = 0; i < p->num_values; i++) {
    pv = &p->values[i];
    if (pv->usb_offset) {
      value = usb_buffer[pv->usb_offset];
      if (p->transform) value = p->transform(value, 0);
      if (pv->usb_value == value) { *pvalue = pv->name; return PARAM_OK; }
    } else {
      for (j = 0; j < pv->num_values; j++) {
        value = usb_buffer[pv->usb_offsets[j]];
        if (p->transform) value = p->transform(value, j);
        if (pv->usb_values[j] != value) break;
      }
      if (j == pv->num_values) { *pvalue = pv->name; return PARAM_OK; }
    }
  }
  return PARAM_NO_SUCH_VALUE;
}

/* Debug: set param at hex offset name to value */
static enum param_result
set_hex_par (char *name, char *value, char *usb_buffer)
{
  int offset;
  int val;
  int n;
  int len = *(int*)&usb_buffer[0];
  n = sscanf (name, "0x%x", &offset);
  if (n != 1) return PARAM_NOT_SUPPORTED;
  if (offset > len) return PARAM_NOT_SUPPORTED;
  n = sscanf (value, "0x%x", &val);
  if (n != 1) return PARAM_NOT_SUPPORTED;
  printf ("len = 0x%x, offset = 0x%x, val = 0x%x\n", len, offset, val);
  usb_buffer[offset] = val;
  return PARAM_OK;
}

enum param_result
set_param_value (char *name, char *value, char *usb_buffer)
{
  if (name[0] == '0') {
    return set_hex_par (name, value, usb_buffer);
  }
  
  param_t p = find_param (name);
  
  if (p->modes && !*p->modes) return PARAM_CANNOT_BE_SET;

  if (strcmp(name, "Mode") != 0) {
    get_param_value ("Mode", &current_camera_mode, usb_buffer);
    if (!compatible_with_mode (name)) return PARAM_NOT_FOR_THIS_MODE;
  }
  
  if (!p) return PARAM_NO_SUCH_NAME;
  if (!param_supported(p)) return PARAM_NOT_SUPPORTED;
  
  param_value_t pv = NULL;
  int i;
  for (i = 0; i < p->num_values; i++) {
    pv = &p->values[i];
    if (!strcmp(pv->name, value)) break;
    pv = NULL;
  }
  if (!pv) return PARAM_NO_SUCH_VALUE;
  
  uchar val;
  if (pv->usb_offset) {
    val = pv->usb_value;
    if (p->transform) val = p->transform(val, 0);
    usb_buffer[pv->usb_offset] = val;
  } else {
    for (i = 0; i < pv->num_values; i++) {
      val = pv->usb_values[i];
      if (p->transform) val = p->transform(val, 0);
      usb_buffer[pv->usb_offsets[i]] = val;
    }
  }
  return PARAM_OK;
}

int
param_help (int line, int pagesize)
{
  int i;
  printf ("Parameter values:\n"); line++;
  if (!(line % pagesize)) { printf ("--- MORE ---\n"); getchar(); }
  
  for (i = 0; i < params_size; i++)
  { param_t p = &params[i];
    if (!param_supported(p)) continue;
    if (!p->num_values) continue;
    int n = 0;
    printf ("   %-11s", p->name);
    if (p->help) {
      printf ("%s\n", p->help);printf ("   %-11s", " ");
    }
    printf ("(");
    int j;
    char *sep = "";
    for (j = 0; j < p->num_values; j++) {
      n += printf ("%s", sep);
      if (n > 60) { printf ("\n    %-11s", " "); n = 0; }
      n += printf ("%s", p->values[j].name);
      sep = ", ";
    }
    printf (")\n"); line++; 
    if (!(line % pagesize)) { printf("--- MORE ---\n"); getchar(); }
  }
  return line;
}
