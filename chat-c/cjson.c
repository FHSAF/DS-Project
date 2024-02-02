#include <stdio.h>
#include <cJSON.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
// gcc -I /path/to/cjson your_program.c -lcjson -o your_program
// {
//     "name": "John Doe",
//     "age": 30,
//     "cars": [
//         "Fiat",
//         "BMW"
//     ]
// }

cJSON *create_json() {
    cJSON *name = cJSON_CreateString("John Doe");
    cJSON *age = cJSON_CreateNumber(30);
    cJSON *car1 = cJSON_CreateString("Fiat");
    cJSON *car2 = cJSON_CreateString("BMW");
    cJSON *cars = cJSON_CreateArray();
    cJSON_AddItemToArray(cars, car1);
    cJSON_AddItemToArray(cars, car2);
    cJSON *person = cJSON_CreateObject();
    cJSON_AddItemToObject(person, "name", name);
    cJSON_AddItemToObject(person, "age", age);
    cJSON_AddItemToObject(person, "cars", cars);
    return person;
}



void send_json(int socket, cJSON *json) {
    char *string = cJSON_Print(json);
    if (send(socket, string, strlen(string), 0) < 0) {
        printf("Failed to send the message\n");
    }
    free(string);
}

cJSON *receive_json(int socket) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    if (recv(socket, buffer, sizeof(buffer), 0) < 0) {
        printf("Failed to receive the message\n");
        return NULL;
    }
    cJSON *json = cJSON_Parse(buffer);
    return json;
}

void parse_json(char *message) {
    cJSON *json = cJSON_Parse(message);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return;
    }

    cJSON *name = cJSON_GetObjectItemCaseSensitive(json, "name");
    if (cJSON_IsString(name) && (name->valuestring != NULL)) {
        printf("Name: %s\n", name->valuestring);
    }

    cJSON *age = cJSON_GetObjectItemCaseSensitive(json, "age");
    if (cJSON_IsNumber(age)) {
        printf("Age: %d\n", age->valuedouble);
    }

    cJSON *cars = cJSON_GetObjectItemCaseSensitive(json, "cars");
    cJSON *car;
    cJSON_ArrayForEach(car, cars) {
        if (cJSON_IsString(car) && (car->valuestring != NULL)) {
            printf("Car: %s\n", car->valuestring);
        }
    }

    cJSON_Delete(json);
}