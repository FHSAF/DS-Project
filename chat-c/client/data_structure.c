#include "client.h"


void remove_from_holdback_queue(HoldBackQueue **head, int clk_index)
{
    if (*head == NULL)
    {
        printf("[remove_from_holdback_queue] Queue is empty.\n");
        return;
    }
    if ((*head)->clk_index == clk_index)
    {
        HoldBackQueue *temp = *head;
        *head = (*head)->next;
        free(temp);
        return;
    }
    HoldBackQueue *current = *head;
    while (current->next != NULL)
    {
        if (current->next->clk_index == clk_index)
        {
            HoldBackQueue *temp = current->next;
            current->next = current->next->next;
            free(temp);
            return;
        }
        current = current->next;
    }
    printf("[remove_from_holdback_queue] Message not found.\n");
    return;
}

int array_compare(int *arr1, int *arr2)
{
    for (int i = 0; i < MAX_GROUP_SIZE; i++)
    {
        if (arr1[i] != arr2[i])
        {
            if (arr1[i] > arr2[i])
                return (1);
        }
    }
    return (0);
}

void print_holdback_queue(HoldBackQueue *head)
{
    HoldBackQueue *current = head;
    while (current != NULL)
    {
        printf("\n%s\n\n", current->content);
        current = current->next;
    }
    return;
}

void append_to_holdback_queue(HoldBackQueue **head, char *clean_message)
{
    char sender_dep_str[MAX_GROUP_SIZE * 5];
    sender_dep_str[MAX_GROUP_SIZE] = '\0';
    char content[BUFFER_SIZE];
    int sender_dep[MAX_GROUP_SIZE];
    int sender_id = 0;
    memset(content, 0, sizeof(content));
    memset(sender_dep_str, 0, sizeof(sender_dep_str));
    

    HoldBackQueue *newNode = (HoldBackQueue *)malloc(sizeof(HoldBackQueue));
    if (newNode == NULL)
    {
        printf("[append_to_holdback_queue] malloc() failed.\n");
        return;
    }
    newNode->clk_index = 0;
    newNode->id = 0;
    
    memset(newNode->content, 0, BUFFER_SIZE);
    memset(newNode->dependency, 0, sizeof(newNode->dependency));
    if (sscanf(clean_message, "\n\tGROUP_ID:%d\n\tSENDER_ID:%d\n\tSENDER_CLK_INDEX:%d\n\tDEPS:%s\n\tMESSAGE:%s", 
    &sender_id, &newNode->id, &newNode->clk_index, sender_dep_str, content) != 5)
    {
        printf("[append_to_holdback_queue] sscanf() failed.\n");
        return;
    }

    get_deps_int(sender_dep_str, sender_dep);

    memcpy(newNode->dependency, sender_dep, sizeof(sender_dep));
    memcpy(newNode->content, clean_message, strlen(clean_message) + 1);
    newNode->next = NULL;

    if (*head == NULL)
    {
        printf("[INFO] Queue is empty.\n");
        *head = newNode;
        return;
    }
    HoldBackQueue *current = *head;
    while (current->next != NULL)
    {
        current = current->next;
    }
    current->next = newNode;
}

char * clear_message(char *not_clean_message)
{
    memset(message, 0, BUFFER_SIZE);
    char *end = strstr(not_clean_message, "\n\n");
    if (end == NULL)
    {
        printf("[clear_message] end of message not found\n");
        return (0);
    }
    memcpy(message, not_clean_message, end - not_clean_message);
    return (message);
}

void free_holdback_queue(HoldBackQueue *head)
{
    HoldBackQueue *current = head;
    while (current != NULL)
    {
        HoldBackQueue *temp = current;
        current = current->next;
        free(temp);
    }
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
void get_deps_int(char *str_array, int *int_array)
{
    
    char *token = strtok(str_array, ",");
    for (int i = 0; token != NULL; i++)
    {
        int_array[i] = atoi(token);
        token = strtok(NULL, ",");
    }
}