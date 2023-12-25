// sql_functions.h

#ifndef SQLITE_FUNCTIONS_H
#define SQLITE_FUNCTIONS_H
#define DB_FILE "questions.db"

#include <sqlite3.h>

int callback(void *NotUsed, int argc, char **argv, char **azColName);

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

void displayQuestions(int id, char *resultBuffer, size_t bufferSize) {
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open(DB_FILE, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    // Display all questions
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT * FROM questions WHERE ID=%d;", id);

    rc = sqlite3_exec(db, sql, callback, resultBuffer, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    sqlite3_close(db);
}

void addQuestions(){
    insertQuestion("What is the capital of France?", "Paris", "Berlin", "London", "Paris");
    insertQuestion("Which planet is known as the Red Planet?", "Venus", "Mars", "Jupiter", "Mars");
}

int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

#endif  // SQLITE_FUNCTIONS_H
