/* This file is part of s10sh
 *
 * Copyright (C) 2000 by Salvatore Sanfilippo <antirez@invece.org>
 * Copyright (C) 2001 by Salvatore Sanfilippo <antirez@invece.org>
 *
 * S10sh IS FREE SOFTWARE, UNDER THE TERMS OF THE GPL VERSION 2
 * don't forget what free software means, even if today is so diffused.
 *
 * Simple text based progress bar
 *
 * ALL THIRD PARTY BRAND, PRODUCT AND SERVICE NAMES MENTIONED ARE
 * THE TRADEMARK OR REGISTERED TRADEMARK OF THEIR RESPECTIVE OWNERS
 */

#include <stdio.h>
#include "bar.h"

/* We must assumes 80 columns */
void progressbar(int op, int total, int done)
{
        static int n_written;
        int i, n_current;

        if (op == PROGRESS_RESET) {
                n_written = 0;
                printf("%s", PROGRESS_BAR);
                return;
        }

        if (total == 0)
                return;

        n_current = (int) ((float)done/total*80);
        if (n_current > n_written) {
                for (i = 0; i < (n_current-n_written); i++)
                        putchar(PROGRESS_CHAR);
        }
        fflush(stdout);
        n_written = n_current;
}
