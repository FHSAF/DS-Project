#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define ARRAY_SIZE 10
char message[256];
char Buffer[256];
void get_deps_str(int *int_array, int size, char *str_array);
void get_deps_int(char *str_array, int size, int *int_array);

int main()
{
    int array[ARRAY_SIZE] = {10, 20, 30, 40, 5, 6, 7, 8, 9, 10};
     // assuming each integer is at most 10 digits, plus comma and null terminator
    char str_array[ARRAY_SIZE * 12];
    memset(str_array, 0, sizeof(str_array));
    get_deps_str(array, ARRAY_SIZE, str_array);
    
    printf("str_array: strlen(%d) %s\n", strlen(str_array), str_array);
    int int_array[ARRAY_SIZE];
    get_deps_int(str_array, ARRAY_SIZE, int_array);
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        printf("%d ", int_array[i]);
    }
    printf("\n");
}

void get_deps_str(int *int_array, int size, char *str_array)
{
    
    for (int i = 0; i < size; i++)
    {
        char buf[12];
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%d,", int_array[i]);
        strcat(str_array, buf);
    }
    str_array[strlen(str_array) - 1] = '\0'; // remove the last comma
}
void get_deps_int(char *str_array, int size, int *int_array)
{
    
    char *token = strtok(str_array, ",");
    for (int i = 0; token != NULL; i++)
    {
        int_array[i] = atoi(token);
        token = strtok(NULL, ",");
    }
}