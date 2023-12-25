#ifndef JSON_HELPERS_H
#define JSON_HELPERS_H

#include "cJSON/cJSON.h"

//void removeClientByFd(const char* filename, const char* client_fd) {
//    cJSON* root = cJSON_Parse("{}");  // Create an empty JSON object
//
//    FILE* file = fopen(filename, "r");
//    if (file) {
//        fseek(file, 0, SEEK_END);
//        long fileSize = ftell(file);
//        fseek(file, 0, SEEK_SET);
//
//        if (fileSize > 0) {
//            char* fileContent = (char*)malloc(fileSize + 1);
//            fread(fileContent, 1, fileSize, file);
//            fileContent[fileSize] = '\0';
//
//            cJSON_Delete(root);  // Discard the empty object
//            root = cJSON_Parse(fileContent);  // Parse the existing JSON content
//
//            free(fileContent);
//        }
//
//        fclose(file);
//    }
//
//    cJSON* clientsArray = cJSON_GetObjectItem(root, "clients");
//    if (clientsArray) {
//        // Iterate through the array and remove the client with matching fd
//        cJSON* client = NULL;
//        cJSON_ArrayForEach(client, clientsArray) {
//            cJSON* fdItem = cJSON_GetObjectItem(client, "fd");
//            if (fdItem && strcmp(fdItem->valuestring, client_fd) == 0) {
//                cJSON_DeleteItemFromArray(clientsArray, client);
//                break;  // Stop searching after finding the client
//            }
//        }
//    }
//
//    // Save the updated JSON to a temporary file
//    const char* tempFilename = "temp_clients.json";
//    file = fopen(tempFilename, "w");
//    if (file) {
//        fputs(cJSON_Print(root), file);
//        fclose(file);
//    }
//
//    cJSON_Delete(root);  // Clean up cJSON objects
//
//    // Rename the temporary file to replace the original file
//    if (rename(tempFilename, filename) != 0) {
//        perror("Error renaming file");
//    }
//}
//
//



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
