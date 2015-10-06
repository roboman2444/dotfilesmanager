#include <stdlib.h>
#include <stdio.h>

#include "bool.h"
#include "regex.h"

struct flow {
    struct state *start;
    struct state *end;
};

struct state *make_state() {
    struct state *result = calloc(sizeof(struct state), 1);
    return result;
}

struct flow *parse(char **string) {
    struct flow *last = 0;
    struct flow *result = malloc(sizeof(struct flow));
    result->end = result->start = make_state();
    char lcc = 0;
    struct state *for_range = 0;

    while(**string) {
        switch(**string) {
            case '[':
                (*string)++;
                if (last) {
                    free(last);
                }
                last = parse(string);
                for(int i=0;i<256;i++) {
                    (result->end->next)[i] = (last->start->next)[i];
                }
                last->start = result->end;
                result->end = last->end;
                break;
            case ']':
                return result;
            case '+':
                if (last->start->name) {
                    (last->end->next)[(int)(last->start->name)] = last->start;
                } else {
                    for(int i=0;i<256;i++) {
                        struct state *tmp = (last->start->next)[i];
                        if (tmp) {
                            (last->end->next)[i] = tmp;
                        }
                    }
                }
                break;
            case '-':
                (*string)++;
                if (!lcc) {
                    return 0;
                }
                for(;lcc<=**string;lcc++) {
                    (for_range->next)[(int)lcc] = result->end;
                }
                break;
            case '\\':
                (*string)++;
                //intentional fallthrough
            default:
                lcc = **string;
                for_range = result->end;
                if (last) {
                    free(last);
                }
                last = malloc(sizeof(struct flow));
                last->start = last->end = make_state();
                last->start->name = **string;
                (result->end->next)[(int)**string] = last->end;
                result->end = last->end;
                break;
        }
        (*string)++;
    }

    return result;
}

struct state *make_state_machine(char *string) {
    struct flow *states = parse(&string);
    states->end->is_final = true;
    return states->start;
}

bool matches(const char *str, struct state *regex) {
    int i = 0;
    while(str[i]) {
        struct state *next = (regex->next)[(int)str[i]];
        if (!next) {
            next = (regex->next)['.'];
        }
        if (next) {
            regex = next;
        } else {
            return false;
        }
        i++;
    }
    return regex->is_final;
}

/*
void test(const char *str, struct state *sm, bool expected) {
    bool res = matches(str, sm);
    printf("[%c] %s -> %u\n", expected==res?'S':'F', str, res);
}


int main() {
    struct state *state_machine = make_state_machine("ABC+");
    test("AB", state_machine, false);
    test("ABC", state_machine, true);
    test("ABCC", state_machine, true);
    test("ABCCA", state_machine, false);
    puts("");    
    struct state *sm = make_state_machine("[AB+]+");
    test("AB", sm, true);
    test("ABB", sm, true);
    test("ABAB", sm, true);
    test("AAB", sm, false);
    puts("");    
    struct state *dash = make_state_machine("[A-Z]");
    test("A", dash, true);
    test("B", dash, true);
    test("Y", dash, true);
    test("Z", dash, true);
    test("a", dash, false);
    puts("");    
    struct state *matchall = make_state_machine(".+");
    test("asdasdasd", matchall, true);
    puts("");    
    struct state *useful = make_state_machine("\\[\\[.+\\]\\]");
    test("[[block]]", useful, true);
    test("[[block test]]", useful, true);
    test("[[block]", useful, false);
}
*/
