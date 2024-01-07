#ifndef JSON_HELPERS_H
#define JSON_HELPERS_H

#include <fcntl.h>
// #include <sys/file.h>

#include "cJSON/cJSON.h"

char* extractNameByDescriptor(int descriptor, const char* filename) {
    cJSON* root = cJSON_Parse(filename);
    if (root == NULL) {
        printf("Eroare la parsarea JSON-ului.\n");
        return NULL;
    }

    cJSON *clients = cJSON_GetObjectItemCaseSensitive(root, "clients");
    if (cJSON_IsArray(clients)) {
        int array_size = cJSON_GetArraySize(clients);

        for (int i = 0; i < array_size; i++) {
            cJSON *client = cJSON_GetArrayItem(clients, i);
            cJSON *fd = cJSON_GetObjectItemCaseSensitive(client, "fd");

            if (cJSON_IsNumber(fd) && fd->valueint == descriptor) {
                cJSON *name = cJSON_GetObjectItemCaseSensitive(client, "name");
                if (cJSON_IsString(name)) {
                    char *name_value = strdup(name->valuestring);  // Duplicate stringul pentru a evita eliberarea prematură a memoriei
                    cJSON_Delete(root);
                    return name_value;
                } else {
                    printf("Numele nu este de tip string.\n");
                }

                // Dacă găsește un client cu descriptorul dat, iese din bucla
                break;
            }
        }
    }

    cJSON_Delete(root);
    return NULL;
}

void removeClientByFd(const char* filename, int client_fd) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening JSON file");
        return;
    }

    // Read the content of the JSON file
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char* json_content = (char*)malloc(file_size + 1);
    fread(json_content, 1, file_size, file);
    fclose(file);

    // Null-terminate the JSON content
    json_content[file_size] = '\0';

    // Parse the JSON content
    cJSON* root = cJSON_Parse(json_content);
    free(json_content);

    if (root == NULL) {
        perror("Error parsing JSON");
        return;
    }

    // Get the "clients" array
    cJSON* clients_array = cJSON_GetObjectItemCaseSensitive(root, "clients");
    if (cJSON_IsArray(clients_array)) {
        int array_size = cJSON_GetArraySize(clients_array);

        // Iterate through the array to find and remove the client with the specified fd
        for (int i = 0; i < array_size; ++i) {
            cJSON* client = cJSON_GetArrayItem(clients_array, i);
            cJSON* fd_item = cJSON_GetObjectItemCaseSensitive(client, "fd");

            if (cJSON_IsNumber(fd_item) && fd_item->valueint == client_fd) {
                cJSON_DeleteItemFromArray(clients_array, i);
                array_size--;
                i--;
                break;  // Found and removed the client, exit the loop
            }
        }
    }

    // Write the updated JSON content back to the file
    file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening JSON file for writing");
        cJSON_Delete(root);
        return;
    }

    char* updated_json = cJSON_Print(root);
    fprintf(file, "%s\n", updated_json);
    fclose(file);
    free(updated_json);

    // Cleanup
    cJSON_Delete(root);
}



void deleteFileContent(const char* filename) {
    FILE* file = fopen(filename, "w");  // Open for writing, truncating to zero length
    if (file) {
        fclose(file);
    } else {
        perror("Error opening file");
    }
}

void addClient(const char* filename, int client_fd, const char* client_name) {
    cJSON* root = cJSON_Parse("{}");  // Create an empty JSON object

    FILE* file = fopen(filename, "r");
    if (file) {
        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        if (fileSize > 0) {
            char* fileContent = (char*)malloc(fileSize + 1);
            fread(fileContent, 1, fileSize, file);
            fileContent[fileSize] = '\0';

            cJSON_Delete(root);  // Discard the empty object
            root = cJSON_Parse(fileContent);  // Parse the existing JSON content

            free(fileContent);
        }

        fclose(file);
    }

    cJSON* clientsArray = cJSON_GetObjectItem(root, "clients");
    if (!clientsArray) {
        // If the "clients" array doesn't exist, create a new one
        clientsArray = cJSON_CreateArray();
        cJSON_AddItemToObject(root, "clients", clientsArray);
    }

    cJSON* clientObject = cJSON_CreateObject();
    cJSON_AddStringToObject(clientObject, "name", client_name);
    cJSON_AddNumberToObject(clientObject, "fd", client_fd);
    // cJSON_AddItemToObject(clientObject, "score", cJSON_CreateNumber(0));


    cJSON_AddItemToArray(clientsArray, clientObject);

    // Save the updated JSON to the file
    file = fopen(filename, "w");
    if (file) {
        fputs(cJSON_Print(root), file);
        fclose(file);
    }

    cJSON_Delete(root);  // Clean up cJSON objects
}


#endif
