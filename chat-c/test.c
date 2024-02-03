#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define ARRAY_SIZE 10
char message[256];
char Buffer[256];

int main()
{
    int array[ARRAY_SIZE] = {10, 20, 30, 40, 5, 6, 7, 8, 9, 10};
     // assuming each integer is at most 10 digits, plus comma and null terminator
    for (int i = 0; i < ARRAY_SIZE; i++) {
        char buf[12];

        sprintf(buf, "%d,", array[i]);
        strcat(array_str, buf);
    }
    array_str[strlen(array_str) - 1] = '\0'; // remove the last comma
    printf("array_str: %s(%lu)\n", array_str, strlen(array_str));
    memset(message, 0, sizeof(message));
    sprintf(message, "MESSAGE:Hello, World!:ARRAY:%s\n\n", array_str);
    fflush(stdout);
    printf("message: %s\n", message);
    
    sscanf(message, "MESSAGE:%[^:]:ARRAY:%s", Buffer, array_str);
    

    // char *array_str = strstr(message, "ARRAY:") + 6; // find the start of the array string
    // int array[ARRAY_SIZE];
    char *token = strtok(array_str, ",");
    for (int i = 0; token != NULL; i++) {
        array[i] = atoi(token);
        token = strtok(NULL, ",");
    }
    for (int i = 0; i < ARRAY_SIZE; i++) {
        printf("%d ", array[i]);
    } 
    printf("\n");
}

char * get_deps_str(int *int_array, int size)
{
    char str_arrray[ARRAY_SIZE * 12];
    for (int i = 0; i < size; i++)
    {
        char buf[12];
        sprintf(buf, "%d,", int_array[i]);
        strcat(str_arrray, buf);
    }
    str_arrray[strlen(str_arrray) - 1] = '\0'; // remove the last comma
    return (str_arrray);
}
int * get_deps_int(char *str_array, int size)
{
    int int_array[ARRAY_SIZE];
    char *token = strtok(str_array, ",");
    for (int i = 0; token != NULL; i++)
    {
        int_array[i] = atoi(token);
        token = strtok(NULL, ",");
    }
    for (int i = 0; i < size; i++)
    {
        printf("%d ", int_array[i]);
    }
    printf("\n");
    return (int_array);
}