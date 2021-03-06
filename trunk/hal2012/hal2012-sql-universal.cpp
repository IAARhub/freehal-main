/*
 * This file is part of FreeHAL 2012.
 *
 * Copyright(c) 2006, 2007, 2008, 2009, 2010, 2011, 2012 Tobias Schulz and contributors.
 * http://www.freehal.org
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

#include "hal2012-universal.h"
#include "hal2012-sql-universal.h"
#include "hal2012-ram.h"
#include "hal2012-disk.h"
#include "hal2012-sql.h"
#include "hal2012-universal-cxx.h"

int sql_universal_set_filename(const char* filename) {
}

int sql_universal_begin(const char* modes) {
    return universal_begin(modes);
}

int sql_universal_end() {
    return universal_end();
}

char* sql_universal_del_record(struct RECORD* r) {
    if (r->pkey && r->pkey[0]) {
        return universal_del_record(r->pkey);
    }
    return strdup("");
}

char* sql_universal_get_source(struct RECORD* r) {
    printf("sql_universal_get_source: %li, to_number(r->pkey) = %d, r->pkey = %s\n", (long int)r, to_number(r->pkey), r->pkey ? r->pkey : "(null)");
    if (r->pkey && r->pkey[0]) {
        return universal_get_source(r->pkey);
    }
}

int sql_universal_delete_everything_from(const char* filename) {
    return universal_delete_everything_from(filename);
}

int sql_universal_add_filename(const char* filename) {
    return universal_add_filename(filename);
}

int sql_universal_re_index() {
    return universal_re_index();
}

int sql_universal_add_record(struct RECORD* r, const char* relation_to) {
    if (relation_to && relation_to[0]) {
        return 0;
    }
    else {
        int has_conditional_questionword = 0;
        {
            int n;
            for (n = 0; n <= r->num_clauses && n+1 < MAX_CLAUSES && r->clauses && r->clauses[n]; ++n) {
                if (is_conditional_questionword(((struct RECORD*)(r->clauses[n]))->questionword)) {
                    has_conditional_questionword = 1;
                    break;
                }
            }
        }

        struct fact* fact = add_fact(r->subjects, r->objects, r->verb, r->adverbs, r->extra, r->questionword, r->filename, r->line, r->truth, r->verb_flag_want, r->verb_flag_must, r->verb_flag_can, r->verb_flag_may, r->verb_flag_should, r->only_logic, has_conditional_questionword);

        if (fact && fact->pk) {
            FILE* input_key = fopen("_input_key", "w+b");
            if (input_key) {
                fprintf(input_key, "%d", fact->pk);
                fclose(input_key);
            }
        }
        if (r->clauses && r->clauses[0] && fact && fact->pk) {
            int n;
            int broken = 0;
            for (n = 0; n < r->num_clauses && n+1 < MAX_CLAUSES && r->clauses && r->clauses[n]; ++n) {
                if (broken) {
                    continue;
                }
		struct fact* clause = 0;
                clause = add_clause(fact->pk,
                        ((struct RECORD*)(r->clauses[n]))->subjects,
                        ((struct RECORD*)(r->clauses[n]))->objects,
                        ((struct RECORD*)(r->clauses[n]))->verb,
                        ((struct RECORD*)(r->clauses[n]))->adverbs,
                        ((struct RECORD*)(r->clauses[n]))->extra,
                        ((struct RECORD*)(r->clauses[n]))->questionword,
                        ((struct RECORD*)(r->clauses[n]))->filename,
                        ((struct RECORD*)(r->clauses[n]))->line,
                        ((struct RECORD*)(r->clauses[n]))->truth,
                        ((struct RECORD*)(r->clauses[n]))->verb_flag_want,
                        ((struct RECORD*)(r->clauses[n]))->verb_flag_must,
                        ((struct RECORD*)(r->clauses[n]))->verb_flag_can,
                        ((struct RECORD*)(r->clauses[n]))->verb_flag_may,
                        ((struct RECORD*)(r->clauses[n]))->verb_flag_should );

                if (clause) {
                    free(clause);
                }
                else {
                    broken = 1;
                }
            }
        }
        if (fact && fact->pk && is_engine("disk")) {
            free(fact);
        }
    }
    return 0;
}

struct DATASET sql_universal_get_records(struct RECORD* r) {
    if (r->pkey && r->pkey[0] && to_number(r->pkey)) {
        // return search_clauses_as_dataset(to_number(r->pkey));
        return as_dataset(0);
    }
    return search_facts_as_dataset(r->subjects, r->objects, r->verb, r->adverbs, r->extra, r->questionword, r->context);
}

