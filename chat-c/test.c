#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> // for sleep
#define ARRAY_SIZE 10
char message[256];
char Buffer[256];
void get_deps_str(int *int_array, int size, char *str_array);
void get_deps_int(char *str_array, int size, int *int_array);

typedef struct _info {
    int id;
    time_t time;
    struct _info *next;
} Info;



int main()
{
    Info *MyInfo = (Info *)malloc(sizeof(Info));
    MyInfo->id = 10;
    MyInfo->next = NULL;
    time_t start_t, end_t;

    time(&(MyInfo->time));
    time(&end_t);

    while (1)
    {
        if (difftime(end_t, MyInfo->time) > 5.0)
        {
            fflush(stdout);
            printf("\r[%f] seconds elapsed", difftime(end_t, MyInfo->time));
            fflush(stdout);
            time(&start_t);
        }
        time(&end_t);
        sleep(3);
    }
    free(MyInfo);

    int array[ARRAY_SIZE] = {10, 20, 30, 40, 5, 6, 7, 8, 9, 10};
     // assuming each integer is at most 10 digits, plus comma and null terminator
    char str_array[ARRAY_SIZE * 12];
    memset(str_array, 0, sizeof(str_array));
    get_deps_str(array, ARRAY_SIZE, str_array);
    
    printf("str_array: strlen(%lu) %s\n", strlen(str_array), str_array);
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