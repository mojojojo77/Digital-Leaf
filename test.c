#include <stdio.h>
#include <string.h>

char *strsep(char **stringp, const char *delim) {
    char *start = *stringp;
    char *p;

    if (start == NULL) return NULL;  // No more tokens

    p = strpbrk(start, delim);  // Find the first delimiter in the string
    if (p) {
        *p = '\0';  // Replace delimiter with null terminator
        *stringp = p + 1;  // Update string pointer to the next token
    } else {
        *stringp = NULL;  // No more delimiters, set string pointer to NULL
    }

    return start;
}


int main() {
    char a[] = "This;;is;a;;string;;";
    char *token;
    char *str = a;
    
    while ((token = strsep(&str, "\n")) != NULL) {
        printf("Token: \"%s\"\n", token);
    }
    
    return 0;
}
