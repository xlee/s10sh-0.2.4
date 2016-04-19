/* This file is part of s10sh
 *
 * Copyright (C) 2006 by Lex Augusteijn <lex.augusteijn@chello.nl>
 *
 * S10sh IS FREE SOFTWARE, UNDER THE TERMS OF THE GPL VERSION 2
 * don't forget what free software means, even if today is so diffused.
 *
 * ALL THIRD PARTY BRAND, PRODUCT AND SERVICE NAMES MENTIONED ARE
 * THE TRADEMARK OR REGISTERED TRADEMARK OF THEIR RESPECTIVE OWNERS
 */

#ifndef _custom_h
#define _custom_h

#include "param.h"

enum param_result get_custom_index (char *name, int *index);

enum param_result get_custom_value (char *name, char **cvalue, unsigned char value);

enum param_result set_custom_value (char *name, char *value, int *index, int *pval);

int custom_help (int line, int page_size);

#endif /* _custom_h */
