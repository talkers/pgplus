#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>

#include <mem_test_user.h>

int main (void)
	{
	char *string_1;
        char *string_2;
        char *string_3;
        char *string_4;
        char *ar_1;
        char *ar_2;

        /* we won't free string_1
         * we will free string_2 */
        string_1 = malloc (sizeof (char) * 64);
        string_2 = malloc (sizeof (char) * 128);
        free (string_1);

        /* we won't free string_3 */
        string_3 = malloc (sizeof (char) * 256);
        string_3 = realloc (string_3, sizeof (char) * 512);

        /* we won't free string_4 */
        string_4 = strdup ("This is a test.");

        /* we won't free ar_1
         * we will free ar_2 using cfree */
        ar_1 = calloc (8, sizeof (char) * 16);
        ar_2 = calloc (16, sizeof (char) * 32);
        cfree (ar_2);

        exit (0);

	}
