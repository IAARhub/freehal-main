/*
 * This file is part of FreeHAL 2009.
 *
 * Copyright(c) 2006, 2007, 2008, 2009 Tobias Schulz and contributors.
 * http://freehal.org
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "sqlite3.h"

static sqlite3* sqlite_connection = 0;
static const char* sqlite_filename = 0;
static char*** sqlite_results;
static long int num_of_records[2];

static char* sqlite_sql_create_table = ""
"CREATE TABLE `facts` (`pk` INTEGER PRIMARY KEY AUTOINCREMENT, "
"`from` varchar(250), `verb` varchar(50), `verbgroup` varchar(50), `subjects` varchar(50), `objects` varchar(50), `adverbs` varchar(50), `mix_1` varchar(150), `questionword` varchar(50), `prio` varchar(50), `rel` integer(50), `type` integer(50), `truth` double(50), `hash_clauses` integer(50), "
"UNIQUE(`verb`, `subjects`, `objects`, `adverbs`, `truth`, `hash_clauses`));"
"CREATE INDEX `idx_facts_rel` ON `facts` (`rel`);"
"CREATE INDEX `idx_facts_truth` ON `facts` (`truth`);"
"CREATE INDEX `idx_facts_verb` ON `facts` (`verb`); "
"CREATE INDEX `idx_facts_verbgroup` ON `facts` (`verbgroup`); "
"CREATE INDEX `idx_facts_subjects` ON `facts` (`subjects`); "
"CREATE INDEX `idx_facts_objects` ON `facts` (`objects`); "
"CREATE INDEX `idx_facts_adverbs` ON `facts` (`adverbs`); "
"CREATE INDEX `idx_facts_mix_1` ON `facts` (`mix_1`); "

"CREATE TABLE `cache_facts` (`pk` INTEGER PRIMARY KEY AUTOINCREMENT, "
"`from` varchar(250), `verb` varchar(50), `verbgroup` varchar(50), `subjects` varchar(50), `objects` varchar(50), `adverbs` varchar(50), `mix_1` varchar(150), `questionword` varchar(50), `prio` varchar(50), `rel` integer(50), `type` integer(50), `truth` double(50), `hash_clauses` integer(50), "
"UNIQUE(`verb`, `subjects`, `objects`, `adverbs`, `truth`, `hash_clauses`));"
"CREATE INDEX `idx_cache_facts_rel` ON `cache_facts` (`rel`);"
"CREATE INDEX `idx_cache_facts_truth` ON `cache_facts` (`truth`);"
"CREATE INDEX `idx_cache_facts_verb` ON `cache_facts` (`verb`); "
"CREATE INDEX `idx_cache_facts_verbgroup` ON `cache_facts` (`verbgroup`); "
"CREATE INDEX `idx_cache_facts_subjects` ON `cache_facts` (`subjects`); "
"CREATE INDEX `idx_cache_facts_objects` ON `cache_facts` (`objects`); "
"CREATE INDEX `idx_cache_facts_adverbs` ON `cache_facts` (`adverbs`); "
"CREATE INDEX `idx_cache_facts_mix_1` ON `cache_facts` (`mix_1`); "

"CREATE TABLE `clauses` (`pk` INTEGER PRIMARY KEY AUTOINCREMENT, "
"`from` varchar(250), `verb` varchar(50), `verbgroup` varchar(50), `subjects` varchar(50), `objects` varchar(50), `adverbs` varchar(50), `mix_1` varchar(150), `questionword` varchar(50), `prio` varchar(50), `rel` integer(50), `type` integer(50), `truth` double(50), `hash_clauses` integer(50), "
"UNIQUE(`verb`, `subjects`, `objects`, `adverbs`, `truth`, `hash_clauses`));"
"CREATE INDEX `idx_clauses_rel` ON `clauses` (`rel`);"
"CREATE INDEX `idx_clauses_truth` ON `clauses` (`truth`);"
"CREATE INDEX `idx_clauses_verb` ON `clauses` (`verb`); "
"CREATE INDEX `idx_clauses_verbgroup` ON `clauses` (`verbgroup`); "
"CREATE INDEX `idx_clauses_subjects` ON `clauses` (`subjects`); "
"CREATE INDEX `idx_clauses_objects` ON `clauses` (`objects`); "
"CREATE INDEX `idx_clauses_adverbs` ON `clauses` (`adverbs`); "
"CREATE INDEX `idx_clauses_mix_1` ON `clauses` (`mix_1`); "

"CREATE TABLE `linking` (`pk` INTEGER PRIMARY KEY AUTOINCREMENT, "
"`f1` INTEGER, `f2` INTEGER, `link` varchar(50), "
"UNIQUE(`f1`, `f2`, `link`));"
"CREATE INDEX `idx_link_pk` ON `linking` (`pk`);"
"CREATE INDEX `idx_link_f1` ON `linking` (`f1`);"
"CREATE INDEX `idx_link_f2` ON `linking` (`f2`);"

"CREATE TABLE `rel_word_fact` (`word` varchar(50), `fact` INTEGER, `table` varchar(50), "
"verb_flag_want char(2),verb_flag_must char(2),verb_flag_can char(2),verb_flag_may char(2),verb_flag_should char(2),"
"UNIQUE(`word`, `fact`));"
"CREATE INDEX `idx_rel_word_fact_word` ON `rel_word_fact` (`word`);"
"CREATE INDEX `idx_rel_word_fact_fact` ON `rel_word_fact` (`fact`);"

"CREATE TABLE `rel_fact_flag` (`fact` INTEGER, "
"verb_flag_want char(2),verb_flag_must char(2),verb_flag_can char(2),verb_flag_may char(2),verb_flag_should char(2),"
"UNIQUE(`fact`));"
"CREATE INDEX `idx_fact_rel_fact_flag` ON `rel_fact_flag` (`fact`);"
"";

static int select_primary_key(void* key, int argc, char **argv, char **azColName) {
    strcpy((char*)key, argv[0]);
    return 0;
}

static int callback_clause(void* arg, int argc, char **argv, char **azColName) {
    char** record = *(char***)arg;
    --record;
    
    while (record[0]) ++record;
    
    int i;
    for(i=0; i<argc; ++i){
        record[i] = malloc(5);
        strcpy(record[i], "NULL");
    }

    for(i=0; i<argc; ++i){
        fprintf(output(), "Entity: %s\n", argv[i]);
        if (argv[i] && strlen(argv[i])) {
            free(record[i]);
            record[i] = malloc(strlen(argv[i])+1);
            strcpy(record[i], argv[i]);
        }
    }
    
    return 0;
}

static int callback_synonyms (void* arg, int argc, char **argv, char **azColName) {
    if (!argv[0]) {
        return 0;
    }
    
    char** synonyms = *(char***)(arg);
    int n = 0;
    while (synonyms[n] && n < 4000) {
        ++n;
    }
    
    if (!synonyms[n] && (n < 3 || strcmp(synonyms[n-1], argv[0]))) {
        synonyms[n] = strdup(argv[0]);
        if (synonyms[n]) {
            if (synonyms[n][0] == 'a' && synonyms[n][1] == ' ') {
                strcpy(synonyms[n]+1, synonyms[n]+2);
                synonyms[n][0] = '*';
            }
            if (synonyms[n][0] == 'a' && synonyms[n][1] == 'n' && synonyms[n][2] == ' ') {
                strcpy(synonyms[n]+1, synonyms[n]+3);
                synonyms[n][0] = '*';
            }
            if (synonyms[n][0] == 'e' && synonyms[n][1] == 'i' && synonyms[n][2] == 'n' && synonyms[n][3] == ' ') {
                strcpy(synonyms[n]+1, synonyms[n]+4);
                synonyms[n][0] = '*';
            }
            if (synonyms[n][0] == 'e' && synonyms[n][1] == 'i' && synonyms[n][2] == 'n' && synonyms[n][4] == ' ') {
                strcpy(synonyms[n]+1, synonyms[n]+5);
                synonyms[n][0] = '*';
            }
            if (synonyms[n][0] == 'e' && synonyms[n][1] == 'i' && synonyms[n][2] == 'n' && synonyms[n][5] == ' ') {
                strcpy(synonyms[n]+1, synonyms[n]+6);
                synonyms[n][0] = '*';
            }
        }
        if (n+1 < 4000)
            synonyms[n+1] = 0;

        if (n+1 < 4000)
            fprintf(output(), "%d: %s\n", n, synonyms[n]);
        else
            return 1;
        
    }
    
    return 0;
}

static int callback(void* arg, int argc, char **argv, char **azColName) {
    struct DATASET* sqlite_current_data = (struct DATASET*)(arg);
    
    if (sqlite_current_data->size > 99997) {
        return 1;
    }
    if (0 == sqlite_current_data->data) {
        sqlite_current_data->data = calloc(99999, 1);
    }
    
    sqlite_current_data->column_count = argc - 1 + 4*(MAX_CLAUSES - 1);
    
    int i;
    char** record = calloc(sizeof(char**)*(argc+2)*MAX_CLAUSES+1, 1);
    if (argv[0]) {
        size_t size = strlen(argv[0]);
        ++size;
        record[0] = calloc(size, 1);
        strcpy(record[0], argv[0]);
    }
    for(i=0; i < argc-2; ++i){
        if (argv[i+2] && strlen(argv[i+2])) {
            size_t size = strlen(argv[i+2]);
            ++size;
            record[i+1] = calloc(size, 1);
            strcpy(record[i+1], argv[i+2]);
        }
        else {
            record[i+1] = malloc(5);
            strcpy(record[i+1], "NULL");
        }
    }
    
    short there_are_sub_clauses = (argv[1] && 0 == strcmp(argv[1], "-1"));
    
    if (there_are_sub_clauses) fprintf(output(), "There are sub clauses. Ok.\n");
    else                       fprintf(output(), "There aren't any sub clauses. Ok.\n");
    
    /// Fetch subclauses.
    if (there_are_sub_clauses) {
        char* sql = malloc(5120);
        *sql = 0;
        strcat(sql, "SELECT `verb`, `subjects`, `objects`, `adverbs`, `questionword` FROM `clauses` WHERE `rel` = ");
        strcat(sql, argv[0]);
        strcat(sql, ";");
        
        fprintf(output(), "%s\n", sql);
        
        void* offset = record + argc - 1;
        
        char* err;
        while (sqlite3_exec(sqlite_connection, sql, callback_clause, &offset, &err)) {
            if (strstr(err, "are not unique")) {
                break;
            }
            fprintf(output(), "Error while executing SQL: %s\n\n%s\n\n", sql, err);
            break;
        }
        sqlite3_free(err);
        free(sql);
    }

    sqlite_current_data->data[sqlite_current_data->size] = record;

    ++sqlite_current_data->size;
    time_t time_now;
    time(&time_now);
    fprintf(output(), "Timeout is: %li\n", time_now);
    fprintf(output(), "Time    is: %li\n", sqlite_current_data->timeout);
    if (sqlite_current_data->timeout < time_now) {
        if (sqlite_current_data->size >= 1) {
            return 1;
        }
    }
    return 0;
}


char* sql_sqlite_mask_sql(char* sql) {
    halstring ssql;
    ssql.s = sql;
    ssql.do_free = 1;
    halstring* ssql_ref = &ssql;
    ssql_ref = replace(ssql_ref, " a ", "*");
    ssql_ref = replace(ssql_ref, "\"a ", "\"*");
    ssql_ref = replace(ssql_ref, " a\"", "*\"");
    ssql_ref = replace(ssql_ref, "\"a\"", "\"*\"");
    ssql_ref = replace(ssql_ref, " b ", "*");
    ssql_ref = replace(ssql_ref, "\"b ", "\"*");
    ssql_ref = replace(ssql_ref, " b\"", "*\"");
    ssql_ref = replace(ssql_ref, "\"b\"", "\"*\"");
    ssql_ref = replace(ssql_ref, " c ", "*");
    ssql_ref = replace(ssql_ref, "\"c ", "\"*");
    ssql_ref = replace(ssql_ref, " c\"", "*\"");
    ssql_ref = replace(ssql_ref, "\"c\"", "\"*\"");
    ssql_ref = replace(ssql_ref, " d ", "*");
    ssql_ref = replace(ssql_ref, "\"d ", "\"*");
    ssql_ref = replace(ssql_ref, " d\"", "*\"");
    ssql_ref = replace(ssql_ref, "\"d\"", "\"*\"");
    ssql_ref = replace(ssql_ref, " e ", "*");
    ssql_ref = replace(ssql_ref, "\"e ", "\"*");
    ssql_ref = replace(ssql_ref, " e\"", "*\"");
    ssql_ref = replace(ssql_ref, "\"e\"", "\"*\"");
    ssql_ref = replace(ssql_ref, " f ", "*");
    ssql_ref = replace(ssql_ref, "\"f ", "\"*");
    ssql_ref = replace(ssql_ref, " f\"", "*\"");
    ssql_ref = replace(ssql_ref, "\"f\"", "\"*\"");
    strcpy(sql, ssql_ref->s);
    
    return sql;
}

int sql_sqlite_set_filename(const char* filename) {
    sqlite_filename = filename;
}

int sql_sqlite_begin() {
    if (0 == sqlite_connection) {
        fprintf(output(), "%s%s\n", "Open SQLite connection to file: ", sqlite_filename);
        if (sqlite3_open(sqlite_filename, &sqlite_connection)) {
            fprintf(output(), "%s%s\n", "Could not open SQLite connection to file: ", sqlite_filename);
            sqlite_connection = 0;
            return NO_CONNECTION;
        }
    }

    char* err;
    sqlite3_exec(sqlite_connection, "BEGIN;", NULL, NULL, &err);
    sqlite3_free(err);
}

int sql_sqlite_end() {
    if (0 == sqlite_connection) {
        fprintf(output(), "%s%s\n", "Open SQLite connection to file: ", sqlite_filename);
        if (sqlite3_open(sqlite_filename, &sqlite_connection)) {
            fprintf(output(), "%s%s\n", "Could not open SQLite connection to file: ", sqlite_filename);
            sqlite_connection = 0;
            return NO_CONNECTION;
        }
    }

    char* err;
    sqlite3_exec(sqlite_connection, "COMMIT;", NULL, NULL, &err);
    sqlite3_free(err);
    sqlite3_close(sqlite_connection);
    sqlite_connection = 0;
}

int sql_sqlite_add_link (const char* link, int key_1, int key_2) {
    if (0 == sqlite_connection) {
        fprintf(output(), "%s%s\n", "Open SQLite connection to file: ", sqlite_filename);
        if (sqlite3_open(sqlite_filename, &sqlite_connection)) {
            fprintf(output(), "%s%s\n", "Could not open SQLite connection to file: ", sqlite_filename);
            sqlite_connection = 0;
            return NO_CONNECTION;
        }
    }
    
    if (!link) {
        return INVALID;
    }

    char sql[5120];
    *sql = 0;
    strcat(sql, "INSERT INTO linking (`link`, `f1`, `f2`) VALUES (\"");

    char str_fact_1[40];
    snprintf(str_fact_1, 39, "%d", key_1);
    char str_fact_2[40];
    snprintf(str_fact_2, 39, "%d", key_2);

    strcat(sql, link);
    strcat(sql, "\", ");
    strcat(sql, str_fact_1);
    strcat(sql, ", ");
    strcat(sql, str_fact_2);
    strcat(sql, ");");
    
    strcpy(sql, sql_sqlite_mask_sql(sql));
    fprintf(output(), "%s", sql);

    char* err;
    while (sqlite3_exec(sqlite_connection, sql, NULL, NULL, &err)) {
        if (strstr(err, "are not unique")) {
            /// Link is not unique - it already exists in the database
            break;
        }
        fprintf(output(), "Error while executing SQL: %s\n\n%s\n\n", sql, err);
        if (strstr(err, "no such table")) {
            if (sqlite3_exec(sqlite_connection, sqlite_sql_create_table, NULL, NULL, &err)) {
                fprintf(output(), "Error while executing SQL: %s\n\n%s\n\n", sql, err);
            }
        }
        else {
            break;
        }
    }
    sqlite3_free(err);
}

int detect_words(int* num_of_words, char** words, struct RECORD* r) {
    char* subj  = strdup(r->subjects ? r->subjects : "");
    char* obj   = strdup(r->objects  ? r->objects  : "");
    char* advs  = strdup(r->adverbs  ? r->adverbs  : "");
    char* extra = strdup(r->extra    ? r->extra    : "");
    char* buffer;
    
    *num_of_words = 1;
    words[0] = strdup("0");
    
    buffer = strtok(subj, " ;.)(-,");
    while (buffer && strlen(buffer) && strcmp(buffer, "0")) {
        words[*num_of_words] = strdup(buffer);
        buffer = strtok(NULL, " ;.)(-,");
        ++(*num_of_words);
        if (*num_of_words >= 500) break;
    }
    
    buffer = strtok( obj, " ;.)(-,");
    while (buffer && strlen(buffer) && strcmp(buffer, "0")) {
        words[*num_of_words] = strdup(buffer);
        buffer = strtok(NULL, " ;.)(-,");
        ++(*num_of_words);
        if (*num_of_words >= 500) break;
    }
    
    buffer = strtok(advs, " ;.)(-,");
    while (buffer && strlen(buffer) && strcmp(buffer, "0")) {
        words[*num_of_words] = strdup(buffer);
        buffer = strtok(NULL, " ;.)(-,");
        ++(*num_of_words);
        if (*num_of_words >= 500) break;
    }
    
    if (extra) {
        buffer = strtok(extra, " ;.)(-,");
        while (buffer && strlen(buffer) && strcmp(buffer, "0")) {
            words[*num_of_words] = strdup(buffer);
            buffer = strtok(NULL, " ;.)(-,");
            ++(*num_of_words);
            if (*num_of_words >= 500) break;
        }
    }

    --(*num_of_words);
    
    free(subj);
    free(obj);
    free(advs);
    free(extra);
}

int sql_sqlite_add_record(struct RECORD* r, const char* relation_to) {
    if (0 == sqlite_connection) {
        fprintf(output(), "%s%s\n", "Open SQLite connection to file: ", sqlite_filename);
        if (sqlite3_open(sqlite_filename, &sqlite_connection)) {
            fprintf(output(), "%s%s\n", "Could not open SQLite connection to file: ", sqlite_filename);
            sqlite_connection = 0;
            return NO_CONNECTION;
        }
    }
    
    if (!r) {
        return INVALID;
    }
    if ((0 == r->subjects || 0 == *(r->subjects) || 0 == strcmp(r->subjects, "nothing")) || (0 == r->objects)) {
        return INVALID;
    }
    if ( *r->verb == ' ' && !relation_to ) {
        return INVALID;
    }
    if ( (*r->subjects >= '0' && *r->subjects <= '9') && relation_to ) {
        return INVALID;
    }
    
    if (r->verb) {
        char* override_verb = halmalloc(strlen(r->verb)*2+1, "sql_sqlite_add_record");
        *override_verb = 0;
        char* buffer;
        buffer = strtok(r->verb, " ");
        while (buffer) {
            int size_of_buffer = strlen(buffer);
            short there_was_a_flag_change = 1;
            if (size_of_buffer > 7 && 0 == strcmp(buffer+size_of_buffer-7, "/should")) {
                r->verb_flag_should = 1;
            }
            else if (size_of_buffer > 4 && 0 == strcmp(buffer+size_of_buffer-4, "/may")) {
                r->verb_flag_may = 1;
            }
            else if (size_of_buffer > 4 && 0 == strcmp(buffer+size_of_buffer-4, "/can")) {
                r->verb_flag_can = 1;
            }
            else if (size_of_buffer > 5 && 0 == strcmp(buffer+size_of_buffer-5, "/must")) {
                r->verb_flag_must = 1;
            }
            else if (size_of_buffer > 5 && 0 == strcmp(buffer+size_of_buffer-5, "/want")) {
                r->verb_flag_want = 1;
            }
            else {            
                strcat(override_verb, buffer);
                there_was_a_flag_change = 0;
            }
            buffer = strtok(NULL, " ");
            
            if (!buffer) buffer = strtok(NULL, " ");
            if (!buffer) buffer = strtok(NULL, " ");
            
            if (!there_was_a_flag_change && buffer) strcat(override_verb, " ");
        }
        strncpy(r->verb, override_verb, LINE_SIZE);
        halfree(override_verb);
    }
    
    if (num_of_records[relation_to?1:0] == 0) {
        char sql[5120];
        *sql = 0;
        strcat(sql, "SELECT COUNT(pk) FROM ");
        if (relation_to)
            strcat(sql, "clauses");
        else
            strcat(sql, "facts");
        strcat(sql, ";");
        fprintf(output(), sql);
    
        char key[99];
        char* err;
        if (sqlite3_exec(sqlite_connection, sql, select_primary_key, key, &err)) {
            fprintf(output(), "Error while executing SQL: %s\n\n%s\n\n", sql, err);
        }
        sqlite3_free(err);
        
        if (key && key[0]) {
            num_of_records[relation_to?1:0] = atol(key);
        }
        fprintf(output(), "%i", num_of_records[relation_to?1:0]);
    }
    ++(num_of_records[relation_to?1:0]);

    // 5120+sizeof(struct RECORD)
    char* sql = malloc(102400);
    *sql = 0;
    strcat(sql, "INSERT INTO ");
    if (relation_to)
        strcat(sql, "clauses");
    else
        strcat(sql, "facts");
    strcat(sql, " (`pk`, `hash_clauses`, `mix_1`, `verb`, `verbgroup`, `subjects`, `objects`, `adverbs`, `questionword`, `prio`, `from`, `rel`, `truth`) VALUES (");
    char num_of_records_str[10];
    snprintf(num_of_records_str, 9, "%d", num_of_records[relation_to?1:0]);
    strcat(sql, num_of_records_str);
    strcat(sql, ", ");
    char hash_clauses_str[10];
    snprintf(hash_clauses_str, 9, "%d", r->hash_clauses);
    strcat(sql, hash_clauses_str);
    strcat(sql, ", \"");
    strcat(sql, " ");
    strcat(sql, r->subjects);
    strcat(sql, " ");
    strcat(sql, r->objects);
    strcat(sql, " ");
    strcat(sql, r->adverbs);
    strcat(sql, " ");
    strcat(sql, "\", \"");
    strcat(sql, r->verb);
    strcat(sql, "\", \"");
    if (0==strcmp(r->verb, "=") || 0==strcmp(r->verb, "ist") || 0==strcmp(r->verb, "bist") || 0==strcmp(r->verb, "bin") || 0==strcmp(r->verb, "sind") || 0==strcmp(r->verb, "sein") || 0==strcmp(r->verb, "heisst") || 0==strcmp(r->verb, "heisse") || 0==strcmp(r->verb, "heissen") || 0==strcmp(r->verb, "war") || 0==strcmp(r->verb, "is") || 0==strcmp(r->verb, "are") || 0==strcmp(r->verb, "am") || 0==strcmp(r->verb, "was")) {
        strcat(sql, "be");
    }
    if (0==strcmp(r->verb, "haben") || 0==strcmp(r->verb, "habe") || 0==strcmp(r->verb, "hat") || 0==strcmp(r->verb, "hast") || 0==strcmp(r->verb, "hab") || 0==strcmp(r->verb, "have") || 0==strcmp(r->verb, "has")) {
        strcat(sql, "have");
    }
    strcat(sql, "\", \"");
    if (r->subjects[0] != ' ')
        strcat(sql, r->subjects);
    strcat(sql, "\", \"");
    if (r->objects[0] != ' ')
        strcat(sql, r->objects);
    strcat(sql, "\", \"");
    if (r->adverbs[0] != ' ')
        strcat(sql, r->adverbs);
    strcat(sql, "\", \"");
    if (r->questionword) strcat(sql, r->questionword);
    else                 strcat(sql, "NULL");
    strcat(sql, "\", ");
    char prio_str[10];
    snprintf(prio_str, 9, "%d", r->prio?r->prio:50);
    strcat(sql, prio_str);
    strcat(sql, ", \"");
    strcat(sql, r->from);
    strcat(sql, "\", ");
    short there_are_sub_clauses = r->clauses && r->clauses[0] && !relation_to;
    if (relation_to)                strcat(sql, relation_to);
    else if (there_are_sub_clauses) strcat(sql, "-1");
    else                            strcat(sql, "-1");
    strcat(sql, ", ");
    char truth[10];
    snprintf(truth, 9, "%f", r->truth);
    strcat(sql, truth);
    strcat(sql, ");\n");

    strcpy(sql, sql_sqlite_mask_sql(sql));

    if (num_of_records_str) {
        FILE* target = fopen("_input_key", "w+b");
        halwrite(num_of_records_str, 1, strlen(num_of_records_str), target);
        halclose(target);
    }

            /*    char* err;
    while (sqlite3_exec(sqlite_connection, sql, NULL, NULL, &err)) {
        if (strstr(err, "are not unique")) {
            /// Fact is not unique - it already exists in the database
            {
                if (num_of_records_str) {
                    FILE* target = fopen("_input_key", "w+b");
                    halwrite(num_of_records_str, 1, strlen(num_of_records_str), target);
                    halclose(target);
                }
            }

            break;
        }
        fprintf(output(), "Error while executing SQL: %s\n\n%s\n\n", sql, err);
        if (strstr(err, "no such table")) {
            if (sqlite3_exec(sqlite_connection, sqlite_sql_create_table, NULL, NULL, &err)) {
                fprintf(output(), "Error while executing SQL: %s\n\n%s\n\n", sql, err);
            }
        }
        else {
            break;
        }
    }
    sqlite3_free(err);
*/
    {
        char* key = num_of_records_str;

        if (key && key[0]) {
            /// Add fact-flag relations
//            char* sql = malloc(5120+sizeof(r));
            //*sql = 0;

            strcat(sql, "INSERT INTO rel_fact_flag (`fact`, `verb_flag_want`, `verb_flag_must`, `verb_flag_can`, `verb_flag_may`, `verb_flag_should`) VALUES (");
            strcat(sql, key);
            strcat(sql, ", ");
            strcat(sql, r->verb_flag_want?"1":"0");
            strcat(sql, ", ");
            strcat(sql, r->verb_flag_must?"1":"0");
            strcat(sql, ", ");
            strcat(sql, r->verb_flag_can?"1":"0");
            strcat(sql, ", ");
            strcat(sql, r->verb_flag_may?"1":"0");
            strcat(sql, ", ");
            strcat(sql, r->verb_flag_should?"1":"0");
            strcat(sql, ");\n");

/*
            char* err;
            if (sqlite3_exec(sqlite_connection, sql, NULL, key, &err)) {
                if (strstr(err, "is not unique")) {
                    /// do nothing
                }
                else {
                    fprintf(output(), "Error while executing SQL: %s\n\n%s\n\n", sql, err);
                }
            }
            sqlite3_free(err);
            free(sql);
            
            /// Add word-fact relations
            sql = malloc(5120+sizeof(r));
            *sql = 0;
*/

            int num_of_words = 0;
            char** words = calloc(501*sizeof(char*), 1);
            strcpy(r->extra, "");
            detect_words(&num_of_words, words, r);
            
            while (num_of_words >= 0) {
                if (words[num_of_words]) {
                    if (words[num_of_words][0] != '0') {
                        //strcpy(sql, "");
                        strcat(sql, "INSERT INTO rel_word_fact (`word`, `fact`, `table`, `verb_flag_want`, `verb_flag_must`, `verb_flag_can`, `verb_flag_may`, `verb_flag_should`) VALUES (");
                        strcat(sql, "\n\"");
                        strcat(sql, words[num_of_words]);
                        strcat(sql, "\", \n");
                        strcat(sql, key);
                        strcat(sql, ", \n\"");
                        if (relation_to)
                            strcat(sql, "clauses");
                        else
                            strcat(sql, "facts");
                        strcat(sql, "\"");
                        strcat(sql, ", ");
                        strcat(sql, r->verb_flag_want?"1":"0");
                        strcat(sql, ", ");
                        strcat(sql, r->verb_flag_must?"1":"0");
                        strcat(sql, ", ");
                        strcat(sql, r->verb_flag_can?"1":"0");
                        strcat(sql, ", ");
                        strcat(sql, r->verb_flag_may?"1":"0");
                        strcat(sql, ", ");
                        strcat(sql, r->verb_flag_should?"1":"0");
                        strcat(sql, ");");

                        //printf("SQL: %s (%d)\n", sql, num_of_words);
    /*
                        char* err;
                        if (sqlite3_exec(sqlite_connection, sql, select_primary_key, key, &err)) {
                            if (strstr(err, "are not unique")) {
                                /// do nothing
                            }
                            else {
                                //printf("Error while executing SQL: %s\n%s\n", err, sql);
                            }
                        }
                        sqlite3_free(err);
    */
                        free(words[num_of_words]);
                        words[num_of_words] = 0;
                    }
                }
                --num_of_words;
            }
//            free(sql);
            free(words);

            FILE* target = fopen("_input_key", "w+b");
            halwrite(key, 1, strlen(key), target);
            halclose(target);
        }

        if (r->clauses && r->clauses[0] && !relation_to) {
            int n;
            for (n = 0; n+1 < MAX_CLAUSES && r->clauses && r->clauses[n] && r->clauses[n]; ++n) {
                sql_sqlite_add_record(r->clauses[n], key);
                free(r->clauses[n]);
                r->clauses[n] = 0;
            }
            if (r->clauses[n]) free(r->clauses[n]);
            r->clauses[n] = 0;
        }
    }
    
    
    char sqlite_filename_sql[9999];
    strcpy(sqlite_filename_sql, sqlite_filename);
    strcat(sqlite_filename_sql, ".sql");
    FILE* database_sql = fopen(sqlite_filename_sql, "a");
    fprintf(database_sql, "%s", sql);
    halclose(database_sql);
    
    char* err;
    while (sqlite3_exec(sqlite_connection, sql, NULL, NULL, &err)) {
        if (strstr(err, "unique")) {
            /// Fact is not unique - it already exists in the database
            break;
        }
        fprintf(output(), "Error while executing SQL: %s\n\n%s\n\n", sql, err);
        if (strstr(err, "no such table")) {
            sqlite3_free(err);
            if (sqlite3_exec(sqlite_connection, sqlite_sql_create_table, NULL, NULL, &err)) {
                fprintf(output(), "Error while executing SQL: %s\n\n%s\n\n", sql, err);
            }
            else {
                sqlite3_exec(sqlite_connection, sql, NULL, NULL, &err);
            }
        }
        else {
            break;
        }
    }
    sqlite3_free(err);
    free(sql);
}

struct DATASET sql_sqlite_get_records(struct RECORD* r) {
    if (0 == sqlite_connection) {
        fprintf(output(), "%s%s\n", "Open SQLite connection to file: ", sqlite_filename);
        if (sqlite3_open(sqlite_filename, &sqlite_connection)) {
            fprintf(output(), "%s%s\n", "Could not open SQLite connection to file: ", sqlite_filename);
            sqlite_connection = 0;
            struct DATASET set;
            set.err = NO_CONNECTION;
            return set;
        }
    }

    char* sql = malloc(512000+sizeof(r));
    *sql = 0;
    short need_and;


    // Fetch subject before the other parameters because of the SQL join statement
    char* subjects_buffer = 0;
    if (r->subjects && *r->subjects != '0' && strlen(r->subjects)) {
        subjects_buffer = r->subjects;
    }
    else if (r->extra && *r->extra != '0' && strlen(r->extra) && strcmp(r->context, "q_how")) {
        subjects_buffer = r->extra;
    }
    
    char* objects_buffer = 0;
    if (r->objects && *r->objects != '0' && strlen(r->objects)) {
        objects_buffer = r->objects;
    }
    else if (r->extra && *r->extra != '0' && strlen(r->extra) && strcmp(r->context, "q_how") && strcmp(r->context, "q_what_prep") ) {
        objects_buffer = r->extra;
    }


    // Fetch subject synonyms (maximum: 4000)
    char** subject_synonyms = calloc(4000*sizeof(char*), 1);
    if (subjects_buffer) {
        char* buf = 0;
        if (subjects_buffer) buf = strdup(subjects_buffer);
        char* bbuf = buf;
        int size = strlen(buf);
        int j;
        for (j = 0; j < size; ++j) {
            if (buf[j] == ' ') {
                buf[j] = '\0';
                buf += j+1;
                size = strlen(buf);
                j = 0;
                fprintf(output(), "New buf: %s\n", buf);
            }
        }
        strcat(sql, "SELECT objects  FROM facts WHERE truth = 1 AND verbgroup = \"be\" AND (\"");
        if (buf) strcat(sql, buf);
        strcat(sql, "\" GLOB subjects OR subjects ");
        if (buf && strstr(buf, "*"))
            strcat(sql, "GLOB");
        else
            strcat(sql, "=");
        strcat(sql, " \"");
        if (buf) strcat(sql, buf);
        strcat(sql, "\") AND objects <> \"*\" AND subjects <> \"*\" AND adverbs = \"\"");
        
        if (bbuf) free(bbuf);
        
        fprintf(output(), "%s\n", sql);
        
        char* err;
        while (sqlite3_exec(sqlite_connection, sql, callback_synonyms, &subject_synonyms, &err)) {
            if (strstr(err, "are not unique")) {
                break;
            }
            if (strstr(err, "callback requested query abort")) {
                break;
            }
            fprintf(output(), "Error while executing SQL: %s\n\n%s\n\n", sql, err);
            if (strstr(err, "no such table")) {
                sqlite3_free(err);
                if (sqlite3_exec(sqlite_connection, sqlite_sql_create_table, NULL, NULL, &err)) {
                    fprintf(output(), "Error while executing SQL: %s\n\n%s\n\n", sql, err);
                }
            }
            else {
                break;
            }
        }
        sqlite3_free(err);
    
        free(sql);
        sql = malloc(512000+sizeof(r));
        *sql = 0;
    }
    

    // Fetch object synonyms (maximum: 4000)
    char** object_synonyms = calloc(4000*sizeof(char*), 1);
    if (objects_buffer) {
        char* buf = 0;
        if (objects_buffer) buf = strdup(objects_buffer);
        char* bbuf = buf;
        int size = strlen(buf);
        int j;
        for (j = 0; j < size; ++j) {
            if (buf[j] == ' ') {
                buf[j] = '\0';
                buf += j+1;
                size = strlen(buf);
                j = 0;
                fprintf(output(), "New buf: %s\n", buf);
            }
        }
        strcat(sql, "SELECT subjects  FROM facts WHERE truth = 1 AND verbgroup = \"be\" AND (\"");
        if (buf) strcat(sql, buf);
        strcat(sql, "\" GLOB objects OR objects ");
        if (buf && strstr(buf, "*"))
            strcat(sql, "GLOB");
        else
            strcat(sql, "=");
        strcat(sql, " \"");
        if (buf) strcat(sql, buf);
        strcat(sql, "\") AND subjects <> \"*\" AND objects <> \"*\" AND adverbs = \"\"");
        
        if (bbuf) free(bbuf);
        
        fprintf(output(), "%s\n", sql);
        
        char* err;
        while (sqlite3_exec(sqlite_connection, sql, callback_synonyms, &object_synonyms, &err)) {
            if (strstr(err, "are not unique")) {
                break;
            }
            if (strstr(err, "callback requested query abort")) {
                break;
            }
            fprintf(output(), "Error while executing SQL: %s\n\n%s\n\n", sql, err);
            if (strstr(err, "no such table")) {
                sqlite3_free(err);
                if (sqlite3_exec(sqlite_connection, sqlite_sql_create_table, NULL, NULL, &err)) {
                    fprintf(output(), "Error while executing SQL: %s\n\n%s\n\n", sql, err);
                }
            }
            else {
                break;
            }
        }
        sqlite3_free(err);
    
        free(sql);
        sql = malloc(512000+sizeof(r));
        *sql = 0;
    }
    
    char** important_records = calloc(4000*sizeof(char*), 1);
    {
        {
            int num_of_words = 0;
            char** words = calloc(501*sizeof(char*), 1);
            detect_words(&num_of_words, words, r);
            strcat(sql, "SELECT fact FROM rel_word_fact WHERE (");
            int n = 0;
            while (subject_synonyms[n] && n < 4000) {
                char* subject_synonym_buf = subject_synonyms[n];
                if (subject_synonym_buf[0] == '*')
                    subject_synonym_buf += 1;
                
                if (strstr(subject_synonym_buf, "*")) {
                    strcat(sql, "rel_word_fact.word GLOB \"");
                    strcat(sql, subject_synonym_buf);
                    strcat(sql, "\" OR ");
                }
                else {
                    strcat(sql, "rel_word_fact.word = \"");
                    strcat(sql, subject_synonym_buf);
                    strcat(sql, "\" OR ");
                }
                ++n;
            }
            while (num_of_words >= 0) {
                if (words[num_of_words]) {
                    if (words[num_of_words][0] != '0') {
                        if (strstr(words[num_of_words], "*")) {
                            strcat(sql, "rel_word_fact.word GLOB \"");
                            strcat(sql, words[num_of_words]);
                            strcat(sql, "\" ");
                        }
                        else {
                            strcat(sql, "rel_word_fact.word = \"");
                            strcat(sql, words[num_of_words]);
                            strcat(sql, "\" ");
                        }
                        free(words[num_of_words]);

                        --num_of_words;

                        if (num_of_words>=0) {
                            if (num_of_words >= 0) strcat(sql, "OR ");
                        }
                    }
                    else {
                        strcat(sql, " 0 ");
                        --num_of_words;
                    }
                }
                else {
                    --num_of_words;
                }
            }
            strcat(sql, ")");
            free(words);
        }
        if (r->verb_flag_want)
            strcat(sql, " AND EXISTS(SELECT 1 FROM rel_fact_flag AS rff WHERE rff.fact = pk AND verb_flag_want   = 1) ");
        if (r->verb_flag_must)
            strcat(sql, " AND EXISTS(SELECT 1 FROM rel_fact_flag AS rff WHERE rff.fact = pk AND verb_flag_must   = 1) ");
        if (r->verb_flag_can)
            strcat(sql, " AND EXISTS(SELECT 1 FROM rel_fact_flag AS rff WHERE rff.fact = pk AND verb_flag_can    = 1) ");
        if (r->verb_flag_may)
            strcat(sql, " AND EXISTS(SELECT 1 FROM rel_fact_flag AS rff WHERE rff.fact = pk AND verb_flag_may    = 1) ");
        if (r->verb_flag_should)
            strcat(sql, " AND EXISTS(SELECT 1 FROM rel_fact_flag AS rff WHERE rff.fact = pk AND verb_flag_should = 1) ");

        strcat(sql, " UNION ALL ");
        {
            int num_of_words = 0;
            char** words = calloc(501*sizeof(char*), 1);
            detect_words(&num_of_words, words, r);
            strcat(sql, "SELECT facts.rel FROM rel_word_fact JOIN facts ON facts.pk = rel_word_fact.fact WHERE facts.rel <> -1 AND (");
            int n = 0;
            while (subject_synonyms[n] && n < 4000) {
                char* subject_synonym_buf = subject_synonyms[n];
                if (subject_synonym_buf[0] == '*')
                    subject_synonym_buf += 1;
                if (strstr(subject_synonym_buf, "*")) {
                    strcat(sql, "rel_word_fact.word GLOB \"");
                    strcat(sql, subject_synonym_buf);
                    strcat(sql, "\" OR ");
                }
                else {
                    strcat(sql, "rel_word_fact.word = \"");
                    strcat(sql, subject_synonym_buf);
                    strcat(sql, "\" OR ");
                }
                ++n;
            }
            while (num_of_words >= 0) {
                if (words[num_of_words]) {
                    if (words[num_of_words][0] != '0') {
                        if (strstr(words[num_of_words], "*")) {
                            strcat(sql, "rel_word_fact.word GLOB \"");
                            strcat(sql, words[num_of_words]);
                            strcat(sql, "\" ");
                        }
                        else {
                            strcat(sql, "rel_word_fact.word = \"");
                            strcat(sql, words[num_of_words]);
                            strcat(sql, "\" ");
                        }
                        free(words[num_of_words]);

                        --num_of_words;

                        if (num_of_words>=0) {
                            if (num_of_words >= 0) strcat(sql, "OR ");
                        }
                    }
                    else {
                        strcat(sql, " 0 ");
                        --num_of_words;
                    }
                }
                else {
                    --num_of_words;
                }
            }
            strcat(sql, ")");
            free(words);
        }
        if (r->verb_flag_want)
            strcat(sql, " AND (rel_word_fact.verb_flag_want   = 1) ");
        if (r->verb_flag_must)
            strcat(sql, " AND (rel_word_fact.verb_flag_must   = 1) ");
        if (r->verb_flag_can)
            strcat(sql, " AND (rel_word_fact.verb_flag_can    = 1) ");
        if (r->verb_flag_may)
            strcat(sql, " AND (rel_word_fact.verb_flag_may    = 1) ");
        if (r->verb_flag_should)
            strcat(sql, " AND (rel_word_fact.verb_flag_should = 1) ");
        strcat(sql, " UNION ALL ");
        {
            int num_of_words = 0;
            char** words = calloc(501*sizeof(char*), 1);
            detect_words(&num_of_words, words, r);
            strcat(sql, "SELECT facts.pk FROM rel_word_fact JOIN facts ON facts.rel = rel_word_fact.fact WHERE facts.rel <> -1 AND (");
            int n = 0;
            while (subject_synonyms[n] && n < 4000) {
                char* subject_synonym_buf = subject_synonyms[n];
                if (subject_synonym_buf[0] == '*')
                    subject_synonym_buf += 1;
                if (strstr(subject_synonym_buf, "*")) {
                    strcat(sql, "rel_word_fact.word GLOB \"");
                    strcat(sql, subject_synonym_buf);
                    strcat(sql, "\" OR ");
                }
                else {
                    strcat(sql, "rel_word_fact.word = \"");
                    strcat(sql, subject_synonym_buf);
                    strcat(sql, "\" OR ");
                }
                ++n;
            }
            while (num_of_words >= 0) {
                if (words[num_of_words]) {
                    if (words[num_of_words][0] != '0') {
                        if (strstr(words[num_of_words], "*")) {
                            strcat(sql, "rel_word_fact.word GLOB \"");
                            strcat(sql, words[num_of_words]);
                            strcat(sql, "\" ");
                        }
                        else {
                            strcat(sql, "rel_word_fact.word = \"");
                            strcat(sql, words[num_of_words]);
                            strcat(sql, "\" ");
                        }
                        free(words[num_of_words]);

                        --num_of_words;

                        if (num_of_words>=0) {
                            if (num_of_words >= 0) strcat(sql, "OR ");
                        }
                    }
                    else {
                        strcat(sql, " 0 ");
                        --num_of_words;
                    }
                }
                else {
                    --num_of_words;
                }
            }
            strcat(sql, ")");
            free(words);
        }
        if (r->verb_flag_want)
            strcat(sql, " AND (rel_word_fact.verb_flag_want   = 1) ");
        if (r->verb_flag_must)
            strcat(sql, " AND (rel_word_fact.verb_flag_must   = 1) ");
        if (r->verb_flag_can)
            strcat(sql, " AND (rel_word_fact.verb_flag_can    = 1) ");
        if (r->verb_flag_may)
            strcat(sql, " AND (rel_word_fact.verb_flag_may    = 1) ");
        if (r->verb_flag_should)
            strcat(sql, " AND (rel_word_fact.verb_flag_should = 1) ");

        strcat(sql, " ORDER BY 1");

        fprintf(output(), "%s\n", sql);
        
        char* err;
        while (sqlite3_exec(sqlite_connection, sql, callback_synonyms, &important_records, &err)) {
            if (strstr(err, "are not unique")) {
                break;
            }
            if (strstr(err, "callback requested query abort")) {
                break;
            }
            fprintf(output(), "Error while executing SQL: %s\n\n%s\n\n", sql, err);
            if (strstr(err, "no such table")) {
                sqlite3_free(err);
                if (sqlite3_exec(sqlite_connection, sqlite_sql_create_table, NULL, NULL, &err)) {
                    fprintf(output(), "Error while executing SQL: %s\n\n%s\n\n", sql, err);
                }
            }
            else {
                break;
            }
        }
        sqlite3_free(err);
    
        free(sql);
        sql = malloc(512000+sizeof(r));
        *sql = 0;
    }
    
    strcat(sql, "delete from cache_facts;");
    {
        strcat(sql, "INSERT INTO cache_facts SELECT * from facts WHERE 0 ");
        int w = 0;
        while (important_records[w] && w < 4000) {
            strcat(sql, " OR pk = ");
            strcat(sql, important_records[w]);
            ++w;
        }
        strcat(sql, ";");
    }

    /////////////////////////// First, select the normal facts ///////////////////////////
    strcat(sql, "SELECT `nmain`.`pk`, `nmain`.`rel`, `nmain`.`verb`, `nmain`.`subjects`, `nmain`.`objects`, `nmain`.`adverbs`, `nmain`.`prio`, `nmain`.`from`, `nmain`.`truth` FROM `cache_facts` AS nmain ");
//                    " LEFT JOIN rel_word_fact ON rel_word_fact.fact = nmain.pk");
    strcat(sql, " WHERE ");
    if (important_records[0]) {
        strcat(sql, " ( ");
        int w = 0;
        while (important_records[w] && w < 4000) {
            ++w;
        }
        --w;
        if (w < 0)
            w = 0;
        strcat(sql, " nmain.pk >= ");
        strcat(sql, important_records[0]);
        strcat(sql, " AND nmain.pk <= ");
        strcat(sql, important_records[w]);
        /**
        int n = 0;
        while (important_records[n] && n < 4000) {
            strcat(sql, " OR nmain.pk = ");
            strcat(sql, important_records[n]);
            strcat(sql, " ");
            ++n;
        }
        **/
        strcat(sql, " ) AND ");
    }
    strcat(sql, " nmain.rel = -1 AND (((nmain.objects <> \"\" OR nmain.adverbs <> \"\") AND nmain.objects <> \"\") OR NOT (nmain.verbgroup = \"be\"))");
    need_and = 1;
    
    /// Match the primary key
    if (r->pkey && *r->pkey != '0' && *r->pkey != ' ' && strlen(r->pkey)) {
        if (need_and) strcat(sql, " AND");
        else          strcat(sql, "WHERE");
        strcat(sql, " (nmain.pk IN (SELECT f2 FROM linking WHERE f1 = ");
        strcat(sql, r->pkey);
        strcat(sql, " AND f1 <> f2) OR nmain.pk IN (SELECT f1 FROM linking WHERE f2 = 0 - ");
        strcat(sql, r->pkey);
        strcat(sql, " AND f1 <> f2))");
    }

    /// The 'd' here stands for the first character of "default"
    if (r->verb && *r->verb != '0' && *r->verb != ' ' && strlen(r->verb)) {
        
        // If we do NOT have a WHERE phrase
        // temporarily disabled
        //if (0 != strcmp(r->context, "q_where")) {
            
            char* buffers_all_verbs = malloc(strlen(r->verb)+2);
            strcpy(buffers_all_verbs, r->verb);
            if (buffers_all_verbs) {
                char* buffer = strtok(buffers_all_verbs, " ");
                char* buffers[10];
                int l = 0;
                while (buffer && l < 10) {
                    buffers[l] = buffer;
                    buffer = strtok(NULL, " ");
                    ++l;
                }
                --l;
                
                short flag_is_divided_verb = 1;
                if (l == 0) {
                    flag_is_divided_verb = 0;
                }
                
                while (l >= 0) {
                    buffer = buffers[l];
                    char* buffers = malloc(strlen(r->verb)+2);
                    strcpy(buffers, buffer);
                    if (buffers) {
                        char* buffer = strtok(buffers, "|");
                        if (buffer) {
                            if (need_and) strcat(sql, " AND");
                            else          strcat(sql, "WHERE");
                            strcat(sql, " ( nmain.verb = \"");
                            strcat(sql, buffer);
                            strcat(sql, "\"");
                            if (flag_is_divided_verb) {
                                strcat(sql, " OR nmain.verb GLOB \"");
                                strcat(sql, buffer);
                                strcat(sql, " *\"");
                                strcat(sql, " OR nmain.verb GLOB \"* ");
                                strcat(sql, buffer);
                                strcat(sql, "\"");
                            }
                            while (buffer = strtok(NULL, "|")) {
                                strcat(sql, " OR nmain.verb = \"");
                                strcat(sql, buffer);
                                strcat(sql, "\"");
                                if (flag_is_divided_verb) {
                                    strcat(sql, " OR nmain.verb GLOB \"* ");
                                    strcat(sql, buffer);
                                    strcat(sql, "\"");
                                    strcat(sql, " OR nmain.verb GLOB \"");
                                    strcat(sql, buffer);
                                    strcat(sql, " *\"");
                                }
                            }
                            strcat(sql, " )");
                            need_and = 1;
                        }
                    }
                    free(buffers);
                    --l;
                }
            }
        //}
    }
    if (subjects_buffer) {
        if (need_and) strcat(sql, " AND");
        else          strcat(sql, "WHERE");
        
        if (strstr(subjects_buffer, "*")) {
            strcat(sql, "\n ( nmain.subjects GLOB \"");
            strcat(sql, subjects_buffer);
            strcat(sql, "\" OR nmain.objects GLOB \"");
            strcat(sql, subjects_buffer);
            strcat(sql, "\" ");
        }
        else {
            strcat(sql, "\n ( nmain.subjects GLOB \"");
            strcat(sql, subjects_buffer);
            strcat(sql, "\" OR nmain.subjects GLOB \"* ");
            strcat(sql, subjects_buffer);
            strcat(sql, "\" OR nmain.subjects GLOB \"");
            strcat(sql, subjects_buffer);
            strcat(sql, " *\" OR nmain.objects GLOB \"");
            strcat(sql, subjects_buffer);
            strcat(sql, "\" OR nmain.subjects GLOB \"");
            strcat(sql, subjects_buffer);
            strcat(sql, "\" ");
        }
        
        int n = 0;
        while (subject_synonyms[n] && n < 4000) {
            if (strstr(subject_synonyms[n], "*")) {
                strcat(sql, "\n OR nmain.subjects GLOB \"");
                strcat(sql, subject_synonyms[n]);
                strcat(sql, "\" OR nmain.objects GLOB \"");
                strcat(sql, subject_synonyms[n]);
                strcat(sql, "\" ");
            }
            else {
                strcat(sql, "\n OR nmain.subjects GLOB \"");
                strcat(sql, subject_synonyms[n]);
                strcat(sql, "\" OR nmain.subjects GLOB \"* ");
                strcat(sql, subject_synonyms[n]);
                strcat(sql, "\" OR nmain.subjects GLOB \"");
                strcat(sql, subject_synonyms[n]);
                strcat(sql, " *\" OR nmain.objects GLOB \"");
                strcat(sql, subject_synonyms[n]);
                strcat(sql, "\" OR nmain.subjects GLOB \"");
                strcat(sql, subject_synonyms[n]);
                strcat(sql, "\" ");
            }
            ++n;
        }
        strcat(sql, ")");
        need_and = 1;
    }
    if (objects_buffer && (0 == subjects_buffer || strcmp(objects_buffer, subjects_buffer))) {
        if (need_and) strcat(sql, " AND");
        else          strcat(sql, "WHERE");
        strcat(sql, "\n ( nmain.objects = \"*\" OR nmain.objects GLOB \"");
        strcat(sql, objects_buffer);
        if (objects_buffer == subjects_buffer && objects_buffer == r->extra) {
            strcat(sql, "\" OR nmain.objects = \"\" OR nmain.objects = \" ");
        }
        strcat(sql, "\" ");

        int n = 0;
        while (subject_synonyms[n] && n < 4000) {
            strcat(sql, "\n OR nmain.objects GLOB \"");
            strcat(sql, subject_synonyms[n]);
            strcat(sql, "\" ");
            ++n;
        }
        strcat(sql, " ) ");

        need_and = 1;
    }
    if (r->context && *r->context != 'd' && strlen(r->context) && !strcmp(r->context, "default")) {
        fprintf(output(), "Context is '%s'.\n", r->context);
    }
    if (r->context && *r->context != 'd' && strlen(r->context) && strcmp(r->context, "default")) {
        fprintf(output(), "Context is '%s' (verb = '%s').\n", r->context, (r->verb && *r->verb != '0' && *r->verb != ' ') ? r->verb : "" );

        char* buffers = calloc(5000, 1);
        *buffers = 0;
        short flag_should_contain = 1;
        
        if (0 == strcmp(r->context, "q_what_weakly")) {
            if (r->verb && *r->verb != '0' && *r->verb != ' ' && (strstr(r->verb, "ist") || strstr(r->verb, "war") || strstr(r->verb, "sind") || strstr(r->verb, "waren"))) {
                //if ( !(r->adverbs && *r->adverbs != '0' && strlen(r->adverbs))) {
                    strcpy(buffers, "* ein*");
                //}
            }
        }
        if (0 == strcmp(r->context, "q_who")) {
            if (r->verb && *r->verb != '0' && *r->verb != ' ' && (strstr(r->verb, "ist") || strstr(r->verb, "war") || strstr(r->verb, "sind") || strstr(r->verb, "waren"))) {
                strcpy(buffers, "ein*");
                flag_should_contain = 0;
                if (need_and) strcat(sql, " AND");
                else          strcat(sql, "WHERE");
                strcat(sql, " ( nmain.adverbs = \"\" )");
                need_and = 1;
            }
        }
        if (0 == strcmp(r->context, "q_where")) {
            strcpy(buffers, "*in *;*im *;*am *;*an *;* auf;* aus;* von;*from *;*at *;*in *;*im *;*am *;*an *;*auf *;*from *;*at *;*aus *;*von *");
        }
        if (0 == strcmp(r->context, "q_from_where")) {
            strcpy(buffers, "* aus;* von;*from *;*aus *;*von *;* from *");
        }
        
        if (buffers) {
            char* buffer = strtok(buffers, ";");
            if (buffer) {
                if (need_and) strcat(sql, " AND");
                else          strcat(sql, "WHERE");
                strcat(sql, " ( ");
                if (0 == strcmp(r->context, "q_who") || 0 == strcmp(r->context, "q_what_weakly")) {
                    strcat(sql, " nmain.adverbs = \"\" AND nmain.truth > 0.9 AND ");
                }
                if (flag_should_contain) {
                    strcat(sql, " (nmain.mix_1 GLOB \"");
                    strcat(sql, buffer);
                    strcat(sql, "\")");
                }
                else {
                    strcat(sql, " ((NOT(nmain.mix_1 GLOB \"");
                    strcat(sql, buffer);
                    strcat(sql, "\")))");
                }
                
                while (buffer = strtok(NULL, ";")) {
                    if (flag_should_contain) {
                        strcat(sql, " OR ( nmain.mix_1 GLOB \"");
                        strcat(sql, buffer);
                        strcat(sql, "\")");
                    }
                    else {
                        strcat(sql, " OR ( (NOT(nmain.mix_1 GLOB \"");
                        strcat(sql, buffer);
                        strcat(sql, "\")))");
                    }
                }
                strcat(sql, " ) ");

                need_and = 1;
            }
        }
        halfree(buffers);
    }
    if (r->adverbs && *r->adverbs != '0' && strlen(r->adverbs)) {
        char* buffer;
        buffer = strtok(r->adverbs, " ;,)(-.");
        if (buffer) {
            if (need_and) strcat(sql, " AND");
            else          strcat(sql, "WHERE");
            strcat(sql, " ( nmain.mix_1 GLOB \"*");
            strcat(sql, buffer);
            strcat(sql, "*\")");
            
            while (buffer = strtok(NULL, " ;,)(-.")) {
                strcat(sql, " AND");
                strcat(sql, " ( nmain.mix_1 GLOB \"*");
                strcat(sql, buffer);
                strcat(sql, "*\")");
            }

            need_and = 1;
        }
    }
    if (r->extra && *r->extra != '0' && strlen(r->extra) && r->extra != objects_buffer) {
        char* buffer = r->extra;
        if (need_and) strcat(sql, " AND");
        else          strcat(sql, "WHERE");
        strcat(sql, " ( nmain.mix_1 GLOB \"* ");
        strcat(sql, buffer);
        strcat(sql, "*\" OR nmain.mix_1 GLOB \"*");
        strcat(sql, buffer);
        strcat(sql, " *\" OR nmain.mix_1 GLOB \"* ");
        strcat(sql, buffer);
        strcat(sql, " *\"");
        if (0 == strcmp(r->context, "q_what_prep")) {
            strcat(sql, " OR EXISTS(SELECT i.subjects FROM facts AS i WHERE (i.verbgroup = \"be\") AND ");
            strcat(sql, "i.subjects <> \"\" AND i.objects GLOB \"");
            strcat(sql, buffer);
            strcat(sql, "*\" AND nmain.adverbs GLOB \"* \"||replace(replace(replace(replace(replace(replace(replace(replace(i.subjects, \"ein \", \"\"), \"eine \", \"\"), \"a \", \"\"), \"an \", \"\"), \"der \", \"\"), \"die \", \"\"), \"das \", \"\"), \"the \", \"\"))");
            strcat(sql, " OR EXISTS(SELECT i.subjects FROM facts AS i WHERE (i.verbgroup = \"be\") AND ");
            strcat(sql, "i.subjects <> \"\" AND i.objects GLOB \"");
            strcat(sql, buffer);
            strcat(sql, "*\" AND nmain.adverbs GLOB replace(replace(replace(replace(replace(replace(replace(replace(i.subjects, \"ein \", \"\"), \"eine \", \"\"), \"a \", \"\"), \"an \", \"\"), \"der \", \"\"), \"die \", \"\"), \"das \", \"\"), \"the \", \"\")||\" *\")");
            strcat(sql, " OR EXISTS(SELECT i.objects FROM facts AS i WHERE (i.verbgroup = \"be\") AND ");
            strcat(sql, "i.objects <> \"\" AND i.subjects GLOB \"");
            strcat(sql, buffer);
            strcat(sql, "*\" AND nmain.adverbs GLOB \"* \"||replace(replace(replace(replace(replace(replace(replace(replace(i.objects, \"ein \", \"\"), \"eine \", \"\"), \"a \", \"\"), \"an \", \"\"), \"der \", \"\"), \"die \", \"\"), \"das \", \"\"), \"the \", \"\"))");
            strcat(sql, " OR EXISTS(SELECT i.objects FROM facts AS i WHERE (i.verbgroup = \"be\") AND ");
            strcat(sql, "i.objects <> \"\" AND i.subjects GLOB \"");
            strcat(sql, buffer);
            strcat(sql, "*\" AND nmain.adverbs GLOB replace(replace(replace(replace(replace(replace(replace(replace(i.objects, \"ein \", \"\"), \"eine \", \"\"), \"a \", \"\"), \"an \", \"\"), \"der \", \"\"), \"die \", \"\"), \"das \", \"\"), \"the \", \"\")||\" *\")");
        
            strcat(sql, " OR EXISTS(SELECT i.subjects FROM facts AS i WHERE (i.verbgroup = \"be\") AND ");
            strcat(sql, "i.subjects <> \"\" AND i.objects GLOB \"*");
            strcat(sql, buffer);
            strcat(sql, "\" AND nmain.adverbs GLOB \"* \"||replace(replace(replace(replace(replace(replace(replace(replace(i.subjects, \"ein \", \"\"), \"eine \", \"\"), \"a \", \"\"), \"an \", \"\"), \"der \", \"\"), \"die \", \"\"), \"das \", \"\"), \"the \", \"\"))");
            strcat(sql, " OR EXISTS(SELECT i.subjects FROM facts AS i WHERE (i.verbgroup = \"be\") AND ");
            strcat(sql, "i.subjects <> \"\" AND i.objects GLOB \"*");
            strcat(sql, buffer);
            strcat(sql, "\" AND nmain.adverbs GLOB replace(replace(replace(replace(replace(replace(replace(replace(i.subjects, \"ein \", \"\"), \"eine \", \"\"), \"a \", \"\"), \"an \", \"\"), \"der \", \"\"), \"die \", \"\"), \"das \", \"\"), \"the \", \"\")||\" *\")");
            strcat(sql, " OR EXISTS(SELECT i.objects FROM facts AS i WHERE (i.verbgroup = \"be\") AND ");
            strcat(sql, "i.objects <> \"\" AND i.subjects GLOB \"*");
            strcat(sql, buffer);
            strcat(sql, "\" AND nmain.adverbs GLOB \"* \"||replace(replace(replace(replace(replace(replace(replace(replace(i.objects, \"ein \", \"\"), \"eine \", \"\"), \"a \", \"\"), \"an \", \"\"), \"der \", \"\"), \"die \", \"\"), \"das \", \"\"), \"the \", \"\"))");
            strcat(sql, " OR EXISTS(SELECT i.objects FROM facts AS i WHERE (i.verbgroup = \"be\") AND ");
            strcat(sql, "i.objects <> \"\" AND i.subjects GLOB \"*");
            strcat(sql, buffer);
            strcat(sql, "\" AND nmain.adverbs GLOB replace(replace(replace(replace(replace(replace(replace(replace(i.objects, \"ein \", \"\"), \"eine \", \"\"), \"a \", \"\"), \"an \", \"\"), \"der \", \"\"), \"die \", \"\"), \"das \", \"\"), \"the \", \"\")||\" *\")");
        }
        strcat(sql, ")");
        need_and = 1;
    }
    
    strcat(sql, " AND nmain.questionword NOT IN (\"wenn\", \"if\", \"falls\", \"when\")"
                " AND NOT EXISTS(SELECT nclause.pk FROM facts AS nclause WHERE nclause.rel = nmain.pk AND ( nclause.questionword IN (\"wenn\", \"if\", \"falls\", \"when\") OR nclause.verb GLOB \"*=>*\" ))");
    
    /////////////////////////// Then, select facts by if statements ///////////////////////////
    
    // There must be no primary key
    if ( !( r->pkey && *r->pkey != '0' && *r->pkey != ' ' && strlen(r->pkey) ) ) {

        need_and = 0;

        strcat(sql, " \n\nUNION ALL "
                    "SELECT DISTINCT \n"
                    "fact.pk, fact.rel, fact.verb, fact.subjects, fact.objects, fact.adverbs, fact.prio, NULL, fact.truth \n"
                    "FROM cache_facts AS main JOIN clauses AS clause ON ");
        if (important_records[0]) {
            strcat(sql, " ( ");
            int w = 0;
            while (important_records[w] && w < 4000) {
                ++w;
            }
            --w;
            if (w < 0)
                w = 0;
            strcat(sql, " main.pk >= ");
            strcat(sql, important_records[0]);
            strcat(sql, " AND main.pk <= ");
            strcat(sql, important_records[w]);
            strcat(sql, " ");
            /**
            int n = 0;
            while (important_records[n] && n < 4000) {
                strcat(sql, " OR nmain.pk = ");
                strcat(sql, important_records[n]);
                strcat(sql, " ");
                ++n;
            }
            **/
            strcat(sql, " ) AND ");
        }
        strcat(sql, " main.pk = clause.rel ");
        strcat(sql, "JOIN cache_facts AS fact ON main.truth = 1 AND \n"
                    "((clause.verbgroup <> \"have\" AND clause.verbgroup <> \"be\" AND fact.verb GLOB clause.verb) OR "
                    "((clause.verbgroup = \"have\") AND (fact.verbgroup = \"have\") ) OR  "
                    "((clause.verbgroup = \"be\") AND (fact.verbgroup = \"be\") )) ");
//                    "JOIN rel_word_fact ON (rel_word_fact.fact = main.pk OR rel_word_fact.fact = clause.pk)\n\n");

        {
            if (need_and) strcat(sql, " AND");
            else          strcat(sql, "WHERE");
            strcat(sql, " fact.questionword = \"\" AND fact.subjects <> \"*\" AND fact.objects <> \"*\"");
            need_and = 1;
            
            {
                if (need_and) strcat(sql, " AND");
                else          strcat(sql, "WHERE");

                strcat(sql, "\n (((main.subjects = \"*\" OR clause.subjects = \"*\") AND (clause.subjects <> \"*\" OR main.subjects <> \"*\")) OR (main.subjects = \"\") OR (clause.subjects = \"\") OR (clause.subjects <> \"*\" AND fact.subjects GLOB clause.subjects) OR (clause.subjects = \"*\" AND fact.subjects GLOB \"");
                if (r->subjects && *r->subjects != '0' && strlen(r->subjects)) {
                    strcat(sql, r->subjects);
                }
                strcat(sql, "\")");
                int n = 0;
                while (subject_synonyms[n] && n < 4000) {
                    strcat(sql, " OR fact.subjects GLOB \"");
                    strcat(sql, subject_synonyms[n]);
                    strcat(sql, "\" ");
                    ++n;
                }
                strcat(sql, " ) AND (clause.objects = \"\" OR fact.objects GLOB clause.objects) AND (clause.adverbs = \"\" OR clause.adverbs = \"\" OR fact.adverbs GLOB clause.adverbs)"
                "\n AND (((fact.objects <> \"\" OR fact.adverbs <> \"\") AND fact.objects <> \"\") OR NOT (fact.verbgroup = \"be\"))\n");
            }
        }

        if (r->verb && *r->verb != '0' && *r->verb != ' ' && strlen(r->verb)) {
            char* buffers = malloc(strlen(r->verb)+2);
            strcpy(buffers, r->verb);
            if (buffers) {
                char* buffer = strtok(buffers, "|");
                if (buffer) {
                    if (need_and) strcat(sql, " AND");
                    else          strcat(sql, "WHERE");
                    strcat(sql, " ( main.verb = \"");
                    strcat(sql, buffer);
                    strcat(sql, "\"");
                    while (buffer = strtok(NULL, "|")) {
                        strcat(sql, " OR main.verb = \"");
                        strcat(sql, buffer);
                        strcat(sql, "\"");
                    }
                    strcat(sql, " )");
                    need_and = 1;
                }
            }
            free(buffers);
        }
        if (r->context && *r->context != 'd' && strlen(r->context) && strcmp(r->context, "default")) {
            char* buffers = calloc(5000, 1);
            *buffers = 0;
            short flag_should_contain = 1;
            
            if (0 == strcmp(r->context, "q_what_weakly")) {
                if (r->verb && *r->verb != '0' && *r->verb != ' ' && (strstr(r->verb, "ist") || strstr(r->verb, "war") || strstr(r->verb, "sind") || strstr(r->verb, "waren"))) {
                    if ( !(r->adverbs && *r->adverbs != '0' && strlen(r->adverbs))) {
                        strcpy(buffers, "ein*");
                    }
                }
            }
            if (0 == strcmp(r->context, "q_who")) {
                strcpy(buffers, "ein*");
                flag_should_contain = 0;
                if (need_and) strcat(sql, " AND");
                else          strcat(sql, "WHERE");
                strcat(sql, " ( main.adverbs = \"\" )");
                need_and = 1;
            }
            if (0 == strcmp(r->context, "q_where")) {
                strcpy(buffers, "*in ;*im ;*am ;*an ;* aus;* von;*from ;*at ;in *;im *;am *;an *;from *;at *;aus *;von *");
            }
            
            if (buffers) {
                char* buffer = strtok(buffers, ";");
                if (buffer) {
                    if (need_and) strcat(sql, " AND");
                    else          strcat(sql, "WHERE");
                    strcat(sql, " ( ");
                    if (0 == strcmp(r->context, "q_who")) {
                        strcat(sql, "( main.adverbs = \"\" ) AND main.truth > 0.9 AND ");
                    }
                    if (flag_should_contain) {
                        strcat(sql, " ( main.mix_1 GLOB \"");
                        strcat(sql, buffer);
                        strcat(sql, "\")");
                    }
                    else {
                        strcat(sql, " ( (NOT(main.mix_1 GLOB \"");
                        strcat(sql, buffer);
                        strcat(sql, "\")))");
                    }
                    
                    while (buffer = strtok(NULL, ";")) {
                        if (flag_should_contain) {
                            strcat(sql, " OR ( main.mix_1 GLOB \"");
                            strcat(sql, buffer);
                            strcat(sql, "\")");
                        }
                        else {
                            strcat(sql, " OR ( (NOT(main.mix_1 GLOB \"");
                            strcat(sql, buffer);
                            strcat(sql, "\")))");
                        }
                    }
                    strcat(sql, " ) ");

                    need_and = 1;
                }
            }
            halfree(buffers);
        }
        if (r->extra && *r->extra != '0' && strlen(r->extra)) {
            char* buffer = r->extra;
            if (need_and) strcat(sql, " AND");
            else          strcat(sql, "WHERE");

            strcat(sql, " EXISTS(SELECT 1 FROM rel_word_fact WHERE word = \"");
            strcat(sql, buffer);
            strcat(sql, "\" AND fact IN (main.pk, clause.pk) ) ");
            
            need_and = 1;
        }
        if (r->subjects && *r->subjects != '0' && strlen(r->subjects)) {
            if (need_and) strcat(sql, " AND");
            else          strcat(sql, "WHERE");
            strcat(sql, " ( ((main.subjects = \"\") OR (main.objects = \"\" AND clause.subjects = \"\") OR \"");
            strcat(sql, r->subjects);
            strcat(sql, "\" GLOB main.subjects OR \"");
            strcat(sql, r->subjects);
            strcat(sql, "\" GLOB main.objects OR main.subjects ");
            if (strstr(r->subjects, "*"))
                strcat(sql, "GLOB \"");
            else
                strcat(sql, "= \"");
            strcat(sql, r->subjects);
            strcat(sql, "\" OR main.objects ");
            if (strstr(r->subjects, "*"))
                strcat(sql, "GLOB \"");
            else
                strcat(sql, "= \"");
            strcat(sql, r->subjects);
            strcat(sql, "\" )");
            strcat(sql, " AND ( main.subjects <> \"*\" OR fact.subjects ");
            if (strstr(r->subjects, "*"))
                strcat(sql, "GLOB \"");
            else
                strcat(sql, "= \"");
            strcat(sql, r->subjects);
            strcat(sql, "\" OR fact.objects = \"\" OR main.objects = \"\" OR fact.objects ");
            if (strstr(r->subjects, "*"))
                strcat(sql, "GLOB \"");
            else
                strcat(sql, "= \"");
            strcat(sql, r->subjects);
            strcat(sql, "\")");
            int n = 0;
            while (subject_synonyms[n] && n < 4000) {
                strcat(sql, " OR ((main.subjects = \"\") OR (main.objects = \"\" AND clause.subjects = \"\") OR \"");
                strcat(sql, subject_synonyms[n]);
                strcat(sql, "\" GLOB main.subjects OR \"");
                strcat(sql, subject_synonyms[n]);
                strcat(sql, "\" GLOB main.objects OR main.subjects ");
                if (strstr(subject_synonyms[n], "*"))
                    strcat(sql, "GLOB \"");
                else
                    strcat(sql, "= \"");
                strcat(sql, subject_synonyms[n]);
                strcat(sql, "\" OR main.objects ");
                if (strstr(subject_synonyms[n], "*"))
                    strcat(sql, "GLOB \"");
                else
                    strcat(sql, "= \"");
                strcat(sql, subject_synonyms[n]);
                strcat(sql, "\" )");
                strcat(sql, " AND ( main.subjects <> \"*\" OR fact.subjects ");
                if (strstr(subject_synonyms[n], "*"))
                    strcat(sql, "GLOB \"");
                else
                    strcat(sql, "= \"");
                strcat(sql, subject_synonyms[n]);
                strcat(sql, "\" OR fact.objects = \"\" OR main.objects = \"\" OR fact.objects ");
                if (strstr(subject_synonyms[n], "*"))
                    strcat(sql, "GLOB \"");
                else
                    strcat(sql, "= \"");
                strcat(sql, subject_synonyms[n]);
                strcat(sql, "\")");
                free(subject_synonyms[n]);
                ++n;
            }
            strcat(sql, ")");
            need_and = 1;
        }
        if (r->objects && *r->objects != '0' && strlen(r->objects)) {
            if (need_and) strcat(sql, " AND");
            else          strcat(sql, "WHERE");
            strcat(sql, " ( ( \"");
            strcat(sql, r->objects);
            strcat(sql, "\" GLOB main.objects OR \"");
            strcat(sql, r->objects);
            strcat(sql, "\" GLOB main.subjects )");
            strcat(sql, " AND ( main.objects <> \"*\" OR main.objects <> \"\" OR \"");
            strcat(sql, r->objects);
            strcat(sql, "\" GLOB main.objects OR \"");
            strcat(sql, r->objects);
            strcat(sql, "\" GLOB main.subjects )");
            int n = 0;
            while (object_synonyms[n] && n < 4000) {
                strcat(sql, " OR ( \"");
                strcat(sql, object_synonyms[n]);
                strcat(sql, "\" GLOB main.objects OR \"");
                strcat(sql, object_synonyms[n]);
                strcat(sql, "\" GLOB main.objects )");
                strcat(sql, " AND ( main.objects <> \"*\" OR main.objects <> \"\" OR \"");
                strcat(sql, object_synonyms[n]);
                strcat(sql, "\" GLOB main.objects OR \"");
                strcat(sql, object_synonyms[n]);
                strcat(sql, "\" GLOB main.objects )");
                free(object_synonyms[n]);
                ++n;
            }
            strcat(sql, " ) ");
            need_and = 1;
        }
        if (r->adverbs && *r->adverbs != '0' && strlen(r->adverbs)) {
            char* buffer;
            buffer = strtok(r->adverbs, " ;,)(-.");
            if (buffer) {
                if (need_and) strcat(sql, " AND");
                else          strcat(sql, "WHERE");
                strcat(sql, "(  (main.subjects IN (\"*\", \"\")) OR (main.objects IN (\"*\", \"\") AND clause.subjects IN (\"*\", \"\")) OR (");
                strcat(sql, " ( main.mix_1 GLOB \"*");
                strcat(sql, buffer);
                strcat(sql, "*\")");
                strcat(sql, " AND ( main.subjects <> \"*\"");
                strcat(sql, " OR main.mix_1 GLOB \"*");
                strcat(sql, buffer);
                strcat(sql, "*\")");
                
                while (buffer = strtok(NULL, " ;,)(-.")) {
                    strcat(sql, " AND");
                    strcat(sql, " ( main.mix_1 GLOB \"*");
                    strcat(sql, buffer);
                    strcat(sql, "*\")");
                    strcat(sql, " AND ( main.subjects <> \"*\"");
                    strcat(sql, " OR main.mix_1 GLOB \"*");
                    strcat(sql, buffer);
                    strcat(sql, "*\")");
                    }

                strcat(sql, "))");
                need_and = 1;
            }
        }
        /// There are no adverbs given in HAL request
        else if (!(r->extra && *r->extra != '0' && strlen(r->extra))) {
            if (need_and) strcat(sql, " AND");
            else          strcat(sql, "WHERE");
            strcat(sql, " ( main.adverbs = \"\" OR main.adverbs = \"*\" )");
            
            need_and = 1;
        }
    }
    
    /// The User asks for nothing, he accepts every fact
    if (0 == need_and) {
        strcat(sql, " WHERE `pk` >= (abs(random()) * (SELECT max(`pk`) FROM `facts`)) LIMIT 10");
    }
        
    /// End of statement
    strcat(sql, ";");
    
    fprintf(output(), "%s\n", sql);
    
    struct DATASET sqlite_current_data;
    sqlite_current_data.size = 0;
    sqlite_current_data.data = 0;
    time_t time_now;
    time(&time_now);
    sqlite_current_data.timeout = time_now + 100;

    char* err;
    while (sqlite3_exec(sqlite_connection, sql, callback, &sqlite_current_data, &err)) {
        if (strstr(err, "are not unique")) {
            break;
        }
        fprintf(output(), "Error while executing SQL: %s\n\n%s\n\n", sql, err);
        if (strstr(err, "no such table")) {
            sqlite3_free(err);
            if (sqlite3_exec(sqlite_connection, sqlite_sql_create_table, NULL, NULL, &err)) {
                fprintf(output(), "Error while executing SQL: %s\n\n%s\n\n", sql, err);
            }
        }
        else {
            break;
        }
    }
    sqlite3_free(err);
    
    free(sql);
    
    return sqlite_current_data;
}
