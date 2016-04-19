/* This file is part of s10sh
 *
 * Copyright (C) 2006 by Lex Augusteijn <lex.augusteijn@chello.nl>
 *
 * S10sh IS FREE SOFTWARE, UNDER THE TERMS OF THE GPL VERSION 2
 * don't forget what free software means, even if today is so diffused.
 *
 * Custom value settings, tested for 300D
 *
 * ALL THIRD PARTY BRAND, PRODUCT AND SERVICE NAMES MENTIONED ARE
 * THE TRADEMARK OR REGISTERED TRADEMARK OF THEIR RESPECTIVE OWNERS
 */

#include <stdio.h>
#include <string.h>

#include "s10sh.h"
#include "common.h"
#include "custom.h"

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*a))

/* WARNING
   The tables below were extracted for the 300D.
   Other camera's might need other tables.
   If so, set the value for the static variable to that table.
   It might be useful to make a module per camera type that exports its top-level table.
   The code has been developed for little endianness only.
*/

/* 
Custom function 0x1 = 0x1 0x0 0x1 Set button
Custom function 0x2 = 0x2 0x0 0x0 AF card
Custom function 0x3 = 0x3 0x0 0x1 Av sync speed
Custom function 0x4 = 0x4 0x0 0x0 Shutter/AE lock
Custom function 0x5 = 0x5 0x0 0x0 AF assist flash
Custom function 0x6 = 0x6 0x0 0x1 AE step
Custom function 0x7 = 0x7 0x0 0x0 AF point
Custom function 0x8 = 0x8 0x0 0x0 RAE + Jpeg
Custom function 0x9 = 0x9 0x0 0x0 Serie/auto
Custom function 0xa = 0xa 0x0 0x0 Mirrored display
Custom function 0xb = 0xb 0x0 0x0 Display position menu button
Custom function 0xc = 0xc 0x0 0x1 Mirror lock-up
Custom function 0xd = 0xd 0x0 0x0 Help button function
Custom function 0xe = 0xffffffff 0xffffffff 0x0 Autom. red. fill flash
Custom function 0xf = 0xffffffff 0xffffffff 0x0 Shutter curtain sync
Custom function 0x10 = 0xffffffff 0xffffffff 0x0 Safety shift in Av/Tv
Custom function 0x11 = 0xffffffff 0xffffffff Mirror lock-up time
*/

typedef unsigned char uchar;

/* Possible value of a custom value */
typedef struct custom_value {
  char  *name;			/* Name of custom value */
  uchar  usb_value;		/* The value */
} *custom_value_t;

/* Type representing a custom function */
typedef struct custom {
  char *name;			/* Custom value name */
  int index;			/* Custom function index */
  camera_type *models;		/* Camera models supporting this custom value, 0 terminated */
  int num_values;		/* Number of values */
  custom_value_t values;	/* Values */
  char *help;			/* Help text */
} *custom_t;

static camera_type eos_models[] = { EOS10D, DRebel, EOS20D, EOS350D, UNKOWN_CAMERA };

static struct custom_value Set_values[] = {
  { "Default",    0x00 },
  { "Quality",    0x01 },
  { "Parameters", 0x02 },
  { "Menu",       0x03 },
  { "Replay",     0x04 },
};

static struct custom_value CF_values[] = {
  { "Yes",    0x00 },
  { "No",     0x01 },
};

static struct custom_value Sync_values[] = {
  { "Auto",    0x00 },
  { "1/200",   0x01 },
};

static struct custom_value AE_values[] = {
  { "AF/AE",   0x00 },
  { "AE/AF",   0x01 },
  { "AF/AF",   0x02 },
  { "AE/AE",   0x03 },
};

static struct custom_value Beam_values[] = {
  { "Emits/Fires", 0x00 },
  { "Fires",       0x01 },
  { "External",    0x02 },
  { "Emits",       0x03 },
};

static struct custom_value AE_inc_values[] = {
  { "1/2", 0x00 },
  { "1/3", 0x01 },
};

static struct custom_value AFp_values[] = {
  { "Center", 0x00 },
  { "Bottom", 0x01 },
  { "Right",  0x02 },
  { "Xright", 0x03 },
  { "Auto",   0x04 },
  { "Xleft",  0x05 },
  { "Left",   0x06 },
  { "Top",    0x07 },
};

static struct custom_value RJ_values[] = {
  { "S",  0x00 },
  { "Sf", 0x01 },
  { "M",  0x02 },
  { "Mf", 0x03 },
  { "L",  0x04 },
  { "Lf", 0x05 },
};

static struct custom_value Bracket_values[] = {
  { "0-+/E",  0x00 },
  { "0-+/D",  0x01 },
  { "-0+/E",  0x02 },
  { "-0+/D",  0x03 },
};

static struct custom_value Super_values[] = {
  { "On",   0x00 },
  { "Off",  0x01 },
};

static struct custom_value Menu_values[] = {
  { "Previous/top", 0x00 },
  { "Previous",     0x01 },
  { "Top",          0x02 },
};

static struct custom_value MLU_values[] = {
  { "Off", 0x00 },        
  { "On",  0x01 },        
};

static struct custom_value Assist_values[] = {
  { "Normal", 0x00 },        
  { "Home",   0x01 },        
  { "HP",     0x02 },        
  { "Av",     0x03 },        
  { "FE",     0x04 },        
};

static struct custom canon_customs[] = {
  { "SET",       1, eos_models, ARRAY_SIZE(Set_values),     Set_values,     "SET button func when shooting" },  
  { "CF",        2, eos_models, ARRAY_SIZE(CF_values),      CF_values,      "Shutter release w/o CF card" },  
  { "Sync",      3, eos_models, ARRAY_SIZE(Sync_values),    Sync_values,    "Flash sync speed in Av mode" },  
  { "AElock",    4, eos_models, ARRAY_SIZE(AE_values),      AE_values,      "Shutter button / AE lock button" }, 
  { "Beam",      5, eos_models, ARRAY_SIZE(Beam_values),    Beam_values,    "AF-assist beam / Flash firing" },  
  { "AEinc",     6, eos_models, ARRAY_SIZE(AE_inc_values),  AE_inc_values,  "Exposure level increments" },  
  { "AF-point",  7, eos_models, ARRAY_SIZE(AFp_values),     AFp_values,     "AF point registration" },  
  { "RAW+JPEG",  8, eos_models, ARRAY_SIZE(RJ_values),      RJ_values,      "RAW+JPEG recording" },  
  { "Bracket",   9, eos_models, ARRAY_SIZE(Bracket_values), Bracket_values, "Bracket Sequence / Auto cancel" }, 
  { "Super",    10, eos_models, ARRAY_SIZE(Super_values),   Super_values,   "Superimposed display" },  
  { "Menu-pos", 11, eos_models, ARRAY_SIZE(Menu_values),    Menu_values,    "Menu button display position" },  
  { "MLU",      12, eos_models, ARRAY_SIZE(MLU_values),     MLU_values,     "Mirror lockup" },  
  { "Assist",   13, eos_models, ARRAY_SIZE(Assist_values),  Assist_values,  "Assist button function" },  
};

static struct custom *customs = canon_customs;
static int        custom_size = ARRAY_SIZE(canon_customs);

static custom_t 
find_custom (char *name)
{
  int i;
  if (!name) return NULL;
  for (i = 0; i < custom_size; i++)
  {
    if (!strcmp (name, customs[i].name)) return &customs[i];
  }
  return NULL;
}

static int 
custom_supported (custom_t p)
{
  camera_type *m;
  for (m = p->models; m && *m; m++) {
    if (*m == camera_model) return 1;
  }
  return 0;
}

enum param_result
get_custom_index (char *name, int *index)
{
  custom_t p = find_custom (name);
  
  if (!p) return PARAM_NO_SUCH_NAME;
  if (!custom_supported(p)) return PARAM_NOT_SUPPORTED;
  *index = p->index;
  return PARAM_OK;
}

enum param_result
get_custom_value (char *name, char **cvalue, uchar value)
{
  custom_t p = find_custom (name);
  
  custom_value_t cv;
  int i;
  
  if (!p) return PARAM_NO_SUCH_NAME;
  if (!custom_supported(p)) return PARAM_NOT_SUPPORTED;
  
  for (i = 0; i < p->num_values; i++) {
    cv = &p->values[i];
    if (cv->usb_value == value) { *cvalue = cv->name; return PARAM_OK; }
  }
  return PARAM_NO_SUCH_VALUE;
}

enum param_result 
set_custom_value (char *name, char *value, int *index, int *pval)
{
  custom_t p = find_custom (name);
  
  custom_value_t cv;
  int i;
  
  if (!p) return PARAM_NO_SUCH_NAME;
  if (!custom_supported(p)) return PARAM_NOT_SUPPORTED;
  *index = p->index; 
  
  for (i = 0; i < p->num_values; i++) {
    cv = &p->values[i];
    if (!strcmp(cv->name, value)) { 
      *pval = cv->usb_value; 
      return PARAM_OK; 
    }
  }
  return PARAM_NO_SUCH_VALUE;
}

int
custom_help (int line, int pagesize)
{
  int i;
  printf ("Custom values:\n"); line++;
  if (!(line % pagesize)) { printf ("--- MORE ---\n"); getchar(); }
  
  for (i = 0; i < custom_size; i++)
  { custom_t p = &customs[i];
    if (!custom_supported(p)) continue;
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
