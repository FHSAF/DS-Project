#include "client.h"

int compare_vector(int *v1, int *v2, int size)
{
    for (int i = 0; i < size; i++)
    {
        if (v1[i] != v2[i])
        {
            if (v1[i] > v2[i])
                return 1;
            else
                return -1;
        }
    }
    return (0);
}