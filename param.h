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

#ifndef _param_h
#define _param_h

enum param_result {
  PARAM_OK,
  PARAM_NO_SUCH_NAME,
  PARAM_NO_SUCH_VALUE,
  PARAM_NOT_FOR_THIS_MODE,
  PARAM_NOT_SUPPORTED,
  PARAM_CANNOT_BE_SET,
};

extern char *current_camera_mode;

enum param_result get_param_value (char *name, char **value, char *usb_buffer);
enum param_result set_param_value (char *name, char *value, char *usb_buffer);
int param_help (int line, int page_size);

#endif /* _param_h */
