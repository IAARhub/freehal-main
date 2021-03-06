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

#include "hal2012-xml.h"
#include "hal2012-sql.h"

XML_Object::XML_Object(Mode _mode) : mode(_mode), content(0), text(""), name("") {
    init();
    //cout << "construct: " << "XML_Object" << endl;
}
XML_Object::XML_Object() : mode(LIST), content(0), text(""), name("") {
    init();
    //cout << "construct: " << "XML_Object" << endl;
}
XML_Object::~XML_Object() {
    //cout << "destruct:  " << "XML_Object" << endl;
    if (mode == LIST) {
        int k;
        for (k = 0; k < content->size(); ++k) {
            delete((*content)[k]);
        }
        delete(content);
    }
}
void XML_Object::init() {
    if (mode == LIST) {
        content = new std::vector<XML_Object*>();
    }
    if (mode == TEXT) {
        text = "";
    }
}

Mode XML_Object::get_mode() {
    return mode;
}

XML_Object* operator <<(XML_Object* o, XML_Object& i) {
    (*o->content).push_back(&i);
}
XML_Object* operator <<(XML_Object* o, vector<XML_Object*> i) {
    int k;
    for (k = 0; k < i.size(); ++k) {
        (*o->content).push_back(i[k]);
    }
}
XML_Object* operator >>(XML_Object* i, vector<XML_Object*> o) {
    int k;
    for (k = 0; k < i->content->size(); ++k) {
        o.push_back((*i->content)[k]);
    }
}
void XML_Object::set_name(string name) {
    this->name = name;
}
string XML_Object::get_name() {
    return(name);
}

void trim(std::string& s, const char* t = " \t\n\r\f\v") {
    s.erase(0, s.find_first_not_of(t));
    s.erase(s.find_last_not_of(t) + 1);
}
void XML_Object::set_text(string str) {
    text = str;
    trim(text);
    //cout << "text: " << str << endl;
}
string XML_Object::print_xml() {
    return print_xml(0, 0) + "\n";
}
string XML_Object::print_xml(int level, int secondlevel) {
    if (mode == TEXT) {
        return(text);
    }
    if (mode == LIST) {
        if (name == "clause") {
            ++level;
            secondlevel = 0;
        }

        string str;
        int r;
        str += "<" + name + ">" + (secondlevel==0?"\n":"");
        int k;
        for (k = 0; k < content->size(); ++k) {
            if (secondlevel==0) for (r = 0; r < level+1; ++r) str += "  ";
            str += (*content)[k]->print_xml(level, secondlevel+1) + (secondlevel==0?"\n":"");
        }
        if (secondlevel==0) for (r = 0; r < level; ++r) str += "  ";
        str += "</" + name + ">";
        return(str);
    }
    return(string());
}
string XML_Object::print_str(string tag_name) {
    std::vector<XML_Object*> _content = part(tag_name);

    string str;
    int k;
    for (k = 0; k < _content.size(); ++k) {
        if (k > 0) str += " ";
        str += _content[k]->print_str();
    }
    return(str);
}
string XML_Object::print_str() {
    if (mode == TEXT) {
        return(text);
    }
    if (mode == LIST) {
        string delem = "";
        if (name == "and") {
            delem = "and";
        }
        else if (name == "or") {
            delem = "or";
        }

        if (delem.size() >= 1) {
            delem = " " + delem + " ";
        }

        string str;
        int k;
        for (k = 0; k < content->size(); ++k) {
            if (k > 0) str += delem;
            str += (*content)[k]->print_str();
        }
        return(str);
    }
    return(string(""));
}

XML_Fact::XML_Fact() {
    name = "fact";
    //cout << "construct: " << "XML_Fact" << endl;
}
XML_Fact::~XML_Fact() {
    //cout << "destruct:  " << "XML_Fact" << endl;
}

std::vector<XML_Object*> XML_Object::part(string tag_name) {
    std::vector<XML_Object*> _content;
    int k;
    for (k = 0; k < content->size(); ++k) {
        if ((*content)[k]->get_name() == tag_name) {
            _content.push_back((*content)[k]);
        }
    }
    return _content;
}

string halxml_readfile(string infile) {
    ifstream indata;
    indata.open(infile.c_str());
    if(!indata) {
        return string();
    }
    std::string str;
    indata.seekg(0, std::ios::end);
    str.reserve(indata.tellg());
    indata.seekg(0, std::ios::beg);
    str.assign((std::istreambuf_iterator<char>(indata)), std::istreambuf_iterator<char>());
    indata.close();
    return str;
}

int halxml_ordertags(string& indata, string& predata) {
    int k = 0;
    char num = 0;
    char _num = 0;
    int is_space = 0;
    int indata_size = indata.size();
    while (k < indata_size) {
        _num = num;
        num = indata[k++];
        if (num == '\n' || num == '\r' || num == '\t' || num == ' ') {
            if (!is_space && _num != '>') predata += " ";
            is_space = 1;
            continue;
        }
        is_space = 0;
        //cout << num << endl;

        if (num == '>') {
            predata += "\n";
        }
        else if (_num == '<') {
            if (num == '/') {
                predata += ">\n";
            }
            else {
                predata += "<\n";
                predata.append(1, num);
            }
        }
        else if (num == '<') {
            if (_num != '>') {
                predata += "\n";
            }
        }
        else {
            predata.append(1, num);
        }
    }
    cout << "End-of-file reached.." << endl;
}

XML_Object* halxml_readtree(string tag_name, vector<string>& lines, int& i) {
    XML_Object* tree = new XML_Object(LIST);
    tree->set_name(tag_name);

    while (i < lines.size()) {
        if (lines[i] == ">") {
            ++i;
            if (lines[i] == tag_name) {
                break;
            }
        }
        else if (lines[i] == "<") {
            ++i;
            string tag_name = lines[i];
            ++i;
            {
                tree << *halxml_readtree(tag_name, lines, i);
            }
        }
        else if (string(lines[i]).size() > 0) {
            XML_Object* t = new XML_Object(TEXT);
            t->set_text(lines[i]);
            tree << *t;
        }
        ++i;
    }
    return(tree);
}

XML_Fact* halxml_readfact(vector<string>& lines, int& i) {
    XML_Fact* fact = new XML_Fact();

    while (i < lines.size()) {

        if (lines[i] == ">") {
            ++i;
            if (lines[i] == "fact") {
                break;
            }
        }
        else if (lines[i] == "<") {
            ++i;
            string tag_name = lines[i];
            ++i;
            {
                XML_Object* tree = halxml_readtree(tag_name, lines, i);
                fact << *tree;
            }
        }
        ++i;
    }

    return(fact);
}

vector<XML_Fact*> halxml_readfacts(string& prestr, int(*use)(void*, int, time_t*, const char**, int), time_t* start_ref, const char** filename_ref) {
    vector<string> lines;
    {
        string _str;
        stringstream predata;
        predata << prestr;
        while (getline(predata, _str)) {
            lines.push_back(_str);
        }
    }

    vector<XML_Fact*> xml_facts;
    int i;
    int xml_facts_size = 0;
    for (i = 0; i < lines.size(); ++i) {
        if (lines[i] == "<") {
            ++i;
            if (lines[i] == "fact") {
                ++xml_facts_size;
            }
        }
    }
    i = 0;
    int facts_so_far = 0;
    while (i < lines.size()) {
        if (lines[i] == "<") {
            ++i;
            if (lines[i] == "fact") {
                ++i;
                XML_Fact* fact = halxml_readfact(lines, i);

                if (use) {
                    use(fact, xml_facts_size, start_ref, filename_ref, facts_so_far);
                    ++facts_so_far;
                }
                else {
                    xml_facts.push_back(fact);
                }
            }
        }
        ++i;
    }

    return xml_facts;
}

XML_Fact* record_to_fact(struct RECORD* r, int level = 0) {
    XML_Fact* fact = new XML_Fact();

    XML_Object* tree = 0;
    XML_Object* t = 0;

    tree = new XML_Object(LIST);
    tree->set_name("subject");
    t = new XML_Object(TEXT);
    t->set_text(r->subjects);
    tree << *t;
    fact << *tree;

    tree = new XML_Object(LIST);
    tree->set_name("object");
    t = new XML_Object(TEXT);
    t->set_text(r->objects);
    tree << *t;
    fact << *tree;

    tree = new XML_Object(LIST);
    tree->set_name("verb");
    t = new XML_Object(TEXT);
    t->set_text(r->verb);
    tree << *t;
    fact << *tree;

    tree = new XML_Object(LIST);
    tree->set_name("adverbs");
    t = new XML_Object(TEXT);
    t->set_text(r->adverbs);
    tree << *t;
    fact << *tree;

    tree = new XML_Object(LIST);
    tree->set_name("extra");
    t = new XML_Object(TEXT);
    t->set_text(r->extra);
    tree << *t;
    fact << *tree;

    tree = new XML_Object(LIST);
    tree->set_name("questionword");
    t = new XML_Object(TEXT);
    t->set_text(r->questionword);
    tree << *t;
    fact << *tree;

    tree = new XML_Object(LIST);
    tree->set_name("truth");
    t = new XML_Object(TEXT);
    char truth_str[10];
    snprintf(truth_str, 9, "%f", r->truth);
    t->set_text(truth_str);
    tree << *t;
    fact << *tree;

    tree = new XML_Object(LIST);
    tree->set_name("flags");
    t = new XML_Object(TEXT);
    char flags_str[10];
    snprintf(flags_str, 9, "%d%d%d%d%d", r->verb_flag_want, r->verb_flag_must, r->verb_flag_can, r->verb_flag_may, r->verb_flag_should);
    t->set_text(flags_str);
    tree << *t;
    fact << *tree;

    if (level == 0) {
        int n;
        int broken = 0;
        for (n = 0; n < r->num_clauses && n+1 < MAX_CLAUSES && r->clauses && r->clauses[n]; ++n) {
            struct RECORD* clause = (struct RECORD*)(r->clauses[n]);

/*
    char __tmp[999];
    sprintf(__tmp, "if ((!%i && !%i && !%i && !(%i || %i || %i || %i || %i)) || (%i && %s == ')')) {\n", clause->subjects, clause->objects, clause->verb, clause->verb_flag_want, clause->verb_flag_must, clause->verb_flag_can, clause->verb_flag_may, clause->verb_flag_should, clause->questionword, clause->questionword);

        t = new XML_Object(TEXT);
        t->set_text(__tmp);
        fact << *t;
*/

            if (( (!clause->subjects||0==strcmp(clause->subjects, "")) && (!clause->objects||0==strcmp(clause->objects, "")) && (!clause->verb||0==strcmp(clause->verb, "")) && !(clause->verb_flag_want || clause->verb_flag_must || clause->verb_flag_can || clause->verb_flag_may || clause->verb_flag_should)) || (clause->questionword && clause->questionword[0] == ')')) {
                break;
            }

            XML_Fact* clause_fact = record_to_fact(clause, level+1);
            clause_fact->set_name("clause");
            fact << *clause_fact;
        }
    }

    fact->filename = r->filename;
    fact->line = r->line;

    return fact;
}
extern "C" {
    char* record_to_xml(struct RECORD* r) {
        XML_Fact* fact = record_to_fact(r);
//return strdup("x ");
        const string& xml = fact->print_xml();
        delete(fact);
        return strdup(xml.c_str());
    }

    int add_record_by_xml(struct RECORD* r) {
        XML_Fact* fact = record_to_fact(r);
        int ret = add_xml_fact(fact);
        delete(fact);
        return(ret);
    }

    int use_xml_fact(void* _fact, int xml_facts_size, time_t* start_ref, const char** filename_ref, int k) {
        XML_Fact* xfact = (XML_Fact*)_fact;
        xfact->filename = *filename_ref;
        stringstream sst;
        sst << k+1;
        sst >> xfact->line;

        if (k+1 == xml_facts_size || ((k+1)%200)==0) {
            time_t now = 0;
            time(&now);
            if (now - *start_ref == 0) {
                now = *start_ref + 1;
            }
            {
                long int facts_per_second = k / (now - *start_ref);
                static long int time_needed = 1;
                time_needed = time_needed*0.5 + 0.5*(xml_facts_size-k)/(facts_per_second+1);

//                cout << (k+1) << " of " << xml_facts_size << " added (" << facts_per_second << " facts / sec, " << now - *start_ref << " sec, " << time_needed << " sec needed, " << (100.0/xml_facts_size*(k+1)) << "% done)          \r" << flush;
                cout << (k+1) << " of " << xml_facts_size << " added (" << facts_per_second << " facts / sec, " << now - *start_ref << " sec, " << time_needed << " sec needed, " << (100.0/xml_facts_size*(k+1)) << "% done)          \n" << flush;
            }
        }
        if (k&&((k+1)%15000)==0) {
            cout << endl;
            sql_set_quiet(1);
            sql_end();
            sql_begin("faster");
            sql_set_quiet(0);
        }

        add_xml_fact(xfact);
        delete(xfact);
    }

    int add_xml_fact_file(const char* filename) {
        sql_begin("faster");

        string instr = halxml_readfile(filename);
        if (instr.size() == 0) {
            cerr << "Error: file could not be opened " << endl;
            sql_end();
            return 1;
        }
        string prestr;
        halxml_ordertags(instr, prestr);

        time_t start = 0;
        time(&start);
        cout << filename << ": " << endl;
        vector<XML_Fact*> xml_facts = halxml_readfacts(prestr, use_xml_fact, &start, &filename);

/*        int k;
        int xml_facts_size = xml_facts.size();
        for (k = 0; k < xml_facts_size; ++k) {
        }
*/
        cout << endl << endl;
        sql_end();

        return 0;
    }

    int add_xml_fact(XML_Fact* xfact) {
        int only_logic = 0;
        int has_conditional_questionword = 0;
        /*{
            int n;
            for (n = 0; n <= r->num_clauses && n+1 < MAX_CLAUSES && r->clauses && r->clauses[n]; ++n) {
                if (is_conditional_questionword(((struct RECORD*)(r->clauses[n]))->questionword)) {
                    has_conditional_questionword = 1;
                    break;
                }
            }
        }*/

        string subject = xfact->print_str("subject");
        string object = xfact->print_str("object");
        string verb = xfact->print_str("verb");
        string adverbs = xfact->print_str("adverbs");
        string extra = xfact->print_str("extra");
        string questionword = xfact->print_str("questionword");
        string filename = xfact->filename;
        string line = xfact->line;
        string truth_str = xfact->print_str("truth");

        float truth;
        sscanf(truth_str.c_str(), "%fl", &truth);
	stringstream sst;
	sst << truth_str;
	sst >> truth;

        string verb_flags = xfact->print_str("flags");
        int verb_flag_want = 0, verb_flag_must = 0, verb_flag_can = 0, verb_flag_may = 0, verb_flag_should = 0;
        if (verb_flags.size()==5) sscanf(verb_flags.c_str(), "%d%d%d%d%d", &verb_flag_want, &verb_flag_must, &verb_flag_can, &verb_flag_may, &verb_flag_should);

        struct fact* fact = add_fact(subject.c_str(), object.c_str(), verb.c_str(), adverbs.c_str(), extra.c_str(), questionword.c_str(), filename.c_str(), line.c_str(), truth, verb_flag_want, verb_flag_must, verb_flag_can, verb_flag_may, verb_flag_should, only_logic, has_conditional_questionword);

        if (fact && fact->pk) {
            FILE* input_key = fopen("_input_key", "w+b");
            if (input_key) {
                fprintf(input_key, "%d", fact->pk);
                fclose(input_key);
            }
        }

        if (fact && fact != INVALID_POINTER && fact->pk) {
            std::vector<XML_Object*> _content = xfact->part("clause");
            string str;
            int k;
            for (k = 0; k < _content.size(); ++k) {
                XML_Fact* xclause = (XML_Fact*)_content[k];

                string clause_subject = xclause->print_str("subject");
                string clause_object = xclause->print_str("object");
                string clause_verb = xclause->print_str("verb");
                string clause_adverbs = xclause->print_str("adverbs");
                string clause_extra = xclause->print_str("extra");
                string clause_questionword = xclause->print_str("questionword");
                string clause_truth_str = xclause->print_str("truth");
                float clause_truth;
                sscanf(clause_truth_str.c_str(), "%fl", &clause_truth);
                string clause_verb_flags = xclause->print_str("flags");
                int clause_verb_flag_want = 0, clause_verb_flag_must = 0, clause_verb_flag_can = 0, clause_verb_flag_may = 0, clause_verb_flag_should = 0;
                if (clause_verb_flags.size()==5) sscanf(clause_verb_flags.c_str(), "%d%d%d%d%d", &clause_verb_flag_want, &clause_verb_flag_must, &clause_verb_flag_can, &clause_verb_flag_may, &clause_verb_flag_should);

                struct fact* clause = add_clause(fact->pk, clause_subject.c_str(), clause_object.c_str(), clause_verb.c_str(), clause_adverbs.c_str(), clause_extra.c_str(), clause_questionword.c_str(), filename.c_str(), line.c_str(), clause_truth, clause_verb_flag_want, clause_verb_flag_must, clause_verb_flag_can, clause_verb_flag_may, clause_verb_flag_should);

                if (clause && clause != INVALID_POINTER) {
                    free(clause);
                }
            }

            if (is_engine("disk")) {
                free(fact);
            }
        }

        return 0;
    }
}
#include "hal2012-xml.h"

