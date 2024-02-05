// sql_functions.h

#ifndef SQLITE_FUNCTIONS_H
#define SQLITE_FUNCTIONS_H
#define DB_FILE "questions.db"

#include <sqlite3.h>

int callback(void *NotUsed, int argc, char **argv, char **azColName);

void displayAllQuestions(size_t bufferSize) {
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open(DB_FILE, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    // Display all questions
    char sql[] = "SELECT * FROM questions;";
    sqlite3_stmt *stmt;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        printf("ID: %d\n", sqlite3_column_int(stmt, 0));
        printf("Question: %s\n", sqlite3_column_text(stmt, 1));
        printf("Answer A: %s\n", sqlite3_column_text(stmt, 2));
        printf("Answer B: %s\n", sqlite3_column_text(stmt, 3));
        printf("Answer C: %s\n", sqlite3_column_text(stmt, 4));
        printf("Correct Answer: %s\n", sqlite3_column_text(stmt, 5));
        printf("\n");
    }

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void resetAutoIncrement() {
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open(DB_FILE, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    // Drop the existing table
    char dropTableSql[] = "DROP TABLE IF EXISTS questions;";
    rc = sqlite3_exec(db, dropTableSql, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        exit(1);
    }

    // Recreate the table
    char createTableSql[] = "CREATE TABLE questions ("
                            "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                            "question_text TEXT NOT NULL,"
                            "answer_a TEXT NOT NULL,"
                            "answer_b TEXT NOT NULL,"
                            "answer_c TEXT NOT NULL,"
                            "correct_answer TEXT NOT NULL);";
    rc = sqlite3_exec(db, createTableSql, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        exit(1);
    }

    sqlite3_close(db);
}


void initializeDatabase() {
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open(DB_FILE, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    // Create the questions table
    char *sql = "CREATE TABLE IF NOT EXISTS questions ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "question_text TEXT NOT NULL,"
                "answer_a TEXT NOT NULL,"
                "answer_b TEXT NOT NULL,"
                "answer_c TEXT NOT NULL,"
                "correct_answer TEXT NOT NULL);";

    rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        exit(1);
    }

    sqlite3_close(db);
}

void insertQuestion(const char *questionText, const char *answerA, const char *answerB, const char *answerC, const char *correctAnswer) {
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open(DB_FILE, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    // Insert a question into the database
    char sql[256];
    snprintf(sql, sizeof(sql),
             "INSERT INTO questions (question_text, answer_a, answer_b, answer_c, correct_answer) "
             "VALUES ('%s', '%s', '%s', '%s', '%s');",
             questionText, answerA, answerB, answerC, correctAnswer);

    rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    sqlite3_close(db);
}

void printCorrectAnswer(int id) {
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open(DB_FILE, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT correct_answer FROM questions WHERE ID=%d;", id);

    sqlite3_stmt *stmt;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        // Print the correct answer
        printf("Correct answer: %s\n" , sqlite3_column_text(stmt, 0));
    } else {
        fprintf(stderr, "No data found for question ID %d\n", id);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

char* getCorrectAnswer(int id) {
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open(DB_FILE, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT correct_answer FROM questions WHERE ID=%d;", id);

    sqlite3_stmt *stmt;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        // Get the correct answer as a string
        const char* correctAnswer = sqlite3_column_text(stmt, 0);

        // Allocate memory for the result
        char* result = malloc(strlen(correctAnswer) + 1);
        if (!result) {
            fprintf(stderr, "Memory allocation error\n");
            exit(1);
        }

        // Copy the correct answer to the result buffer
        strcpy(result, correctAnswer);

        sqlite3_finalize(stmt);
        sqlite3_close(db);

        return result;
    } else {
        fprintf(stderr, "No data found for question ID %d\n", id);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return NULL;
    }
}

char* displayQuestion(int id, size_t bufferSize) {
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open(DB_FILE, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    // Display the question and answer options
    char* resultBuffer = malloc(bufferSize);
    if (!resultBuffer) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }

    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT question_text, answer_a, answer_b, answer_c, correct_answer FROM questions WHERE ID=%d;", id);

    sqlite3_stmt *stmt;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        // Fetch the columns directly
        snprintf(resultBuffer, bufferSize, "Intrebarea %d\n%s\nVariante de raspuns:\na) %s\nb) %s\nc) %s\n",
                 id,
                 sqlite3_column_text(stmt, 0),
                 sqlite3_column_text(stmt, 1),
                 sqlite3_column_text(stmt, 2),
                 sqlite3_column_text(stmt, 3));
    } else {
        fprintf(stderr, "No data found for question ID %d\n", id);
        resultBuffer[0] = '\0';  // Empty string
    }


    sqlite3_finalize(stmt);
    sqlite3_close(db);
    printf("%s", resultBuffer);
    printCorrectAnswer(id);

    return resultBuffer;
}




char* getQuestion(int id) {
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open(DB_FILE, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    // Display the question
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT question_text FROM questions WHERE ID=%d;", id);


    char *question = NULL;

    rc = sqlite3_exec(db, sql, callback, &question, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    sqlite3_close(db);

    return question;
}


void addQuestions(){
    insertQuestion("What is the capital of France?", "Paris", "Berlin", "London", "a");
    insertQuestion("Which planet is known as the Red Planet?", "Venus", "Mars", "Jupiter", "b");
    insertQuestion("What’s the national animal of Australia?", "Tasmanian Devil", "Red Kangaroo", "Koala", "b");
    insertQuestion("In which year did the Titanic sink?", "1905", "1931", "1912", "c");
    insertQuestion("What is the largest mammal in the world?", "Blue Whale", "Elephant", "Hippopotamus", "a");
    insertQuestion("What is the longest river in the world?", "Amazon River", "Yangtze River", "Nile River", "c");
    insertQuestion("Which of the following empires had no written language?", "Roman", "Egyptian", "Incan", "c");
    insertQuestion("What’s the smallest country in the world?", "Monaco", "Vatican", "Nauru", "b");
    insertQuestion("What city do The Beatles come from?", "Liverpool", "Munchen", "Birmingham", "a");
    insertQuestion("What country has the highest life expectancy?", "Switzerland", "Canada", "Japan", "c");
    insertQuestion("Who was the Ancient Greek God of the Sun?", "Zeus", "Apollo", "Hermes", "b");
    insertQuestion("What country drinks the most coffee?", "Brazil", "Finland", "Italy", "b");
    insertQuestion("What is Cynophobia?", "Fear of dogs", "Fear of heights", "Fear of spiders", "a");
    insertQuestion("How many languages are written from right to left?", "10", "11", "12", "c");
    insertQuestion("Who was the first woman to win a Nobel Prize (in 1903)?", "Rosalind Franklin", "Maria Curie", "Jane Goodall", "b");
}

void deleteDatabaseContent() {
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open(DB_FILE, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    // Execute DELETE statements for each table to delete all content
    const char *deleteQuestions = "DELETE FROM questions;";
    const char *deleteClients = "DELETE FROM clients;";

    rc = sqlite3_exec(db, deleteQuestions, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    rc = sqlite3_exec(db, deleteClients, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    printf("Database content deleted.\n");

    sqlite3_close(db);
}

int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}


#endif  // SQLITE_FUNCTIONS_H
