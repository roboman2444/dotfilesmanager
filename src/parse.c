/*
 * Copyright (C) 2015 Ted Meyer
 *
 * see LICENSE for details
 *
 */

#define _DEFAULT_SOURCE

#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pwd.h>

#include "dlist.h"
#include "dmap.h"
#include "bool.h"
#include "regex.h"


typedef
struct config_block {
    dlist *defaults;
    dmap *userhost;
} cblock;

typedef
struct host_specific {
    dlist *symlinks;
} hblock;

dlist *intern_lines(FILE *filein) {
    dlist *lines = dlist_new();
    char line[100] = {0};
    while(fgets(line, 100, filein)) {
        int size = strlen(line);
        if (size>1) {
            if (line[0] != '#') {
                char *copy = calloc(size+1, 1);
                memcpy(copy, line, size-1);
                memset(line, 0, size);
                dlist_add(lines, copy);
            }
        }
    }

    return lines;
}

char *get_user_at_host() {
    uid_t uid = geteuid ();
    struct passwd *pw = getpwuid (uid);
    char *username = pw->pw_name;
    struct utsname unameData;   
    uname(&unameData);
    char *hostname = unameData.nodename;
    char *result = calloc(strlen(username)+strlen(hostname)+6, sizeof(char));
    sprintf(result, "\\[%s@%s\\]", username, hostname);
    return result;
}

int is_symlink(char *file) {
    if (!file) {
        perror("cannot operate on null file");
        exit(1);
    }
    struct stat buf;
    lstat(file, &buf);
    return S_ISLNK(buf.st_mode);
}

void add_symlink(dmap *result, char *line) {
    char *key, *value=line;
    int spaces = 0;
    while(*line) {
        if (*line == ' ') {
            *line = 0;
            spaces++;
        } else if (spaces == 2) {
            key=line;
            spaces = 3;
        }
        line++;
    }
    put(result, key, value);
}

dmap *lines_to_symlinks(dlist *lines) {
    struct state *title = make_state_machine("\\[\\[.+\\]\\]");
    struct state *block = make_state_machine("\\[.+\\]");
    struct state *ushst = make_state_machine(get_user_at_host());
    char *line = NULL;
    bool creating = true;
    dmap *result = map_new();

    each(lines, line) {
        while(*line == ' ') {
            line++;
        }
        if (matches(line, title)) {
            creating = true;
        } else if (matches(line, block)) {
            creating = (matches(line, ushst));
        } else if (creating) {
            add_symlink(result, line);
        }
    }
    return result;
}

dmap *block_to_symlinks(cblock *block) {
    struct state *ushst = make_state_machine(get_user_at_host());
    dmap *result = map_new();

    char *line;
    hblock *hsp;
    each(block->defaults, line) {
        add_symlink(result, line);
    }
    map_each(block->userhost, line, hsp) {
        if (matches(line, ushst)) {
            each(hsp->symlinks, line) {
                add_symlink(result, line);
            }
        }
    }
    return result;
}


char *strdup(const char *str) {
    int n = strlen(str) + 1;
    char *dup = malloc(n);
    if(dup) {
        strcpy(dup, str);
    }
    return dup;
}


// [string] -> [{string, [string], [{string, [string]}]}]
dmap *lines_to_block_structure(dlist *lines) {
    struct state *title = make_state_machine("\\[\\[.+\\]\\]");
    struct state *block = make_state_machine("\\[.+\\]");

    dmap *result = map_new();

    cblock *nextblock = 0;
    dmap *nexthosts = 0;
    dlist *nextlist = 0;

    char *line = 0;

    each(lines, line) {
        while(*line == ' ') {
            line++;
        }
        if (matches(line, title)) {
            nextblock = calloc(sizeof(cblock), 1);
            nextblock->defaults = dlist_new();
            nextblock->userhost = map_new();
            nextlist = nextblock->defaults;
            nexthosts = nextblock->userhost;
            put(result, strdup(line), nextblock);
        } else if (matches(line, block)) {
            if (!nextblock) {
                perror("Found a [user@host] before a [[block]]. Exiting...");
                exit(1);
            }
            hblock *nexthost = calloc(sizeof(hblock), 1);
            nexthost->symlinks = dlist_new();
            nextlist = nexthost->symlinks;
            put(nexthosts, strdup(line), nexthost);   
        } else {
            if (!nextblock) {
                perror("Found a symlink definition outside a [[block]]. Exiting ...");
                exit(1);
            }
            dlist_add(nextlist, line);
        }
    }
    return result;
}

// input is a list of cblcks
void write_to_file(FILE *file, dmap *blocks) {
    cblock *conf;
    char *blockname;
    char *sym;

    map_each(blocks, blockname, conf) {
        fprintf(file, "%s\n", blockname);
        each(conf->defaults, sym) {
            fprintf(file, "%s\n", sym);
        }
        hblock *host;
        map_each(conf->userhost, blockname, host) {
            fprintf(file, "%s\n", blockname);
            each(host->symlinks, sym) {
                fprintf(file, "%s\n", sym);
            }
        }
        fprintf(file, "\n");
    }
}

void cmdlist() {
    puts("dotfiles --test GIT [BLOCK* | --interactive]");
    puts("   clones the GIT repository into /tmp/dotfiles/REPONAME.");
    puts("   if any BLOCK is specified, symlink those block's");
    puts("   contents into their proper locations.");
    puts("   if --interactive is provided instead of BLOCK*");
    puts("   a dialog will open to allow choice of blocks");
    puts("");
    puts("dotfiles --sync");
    puts("   refresh the symlinks in the home directory");
    puts("");
    puts("dotfiles --init");
    puts("   if no DIR is provided, create ~/.dotfiles, otherwise");
    puts("   copies the DIR to ~/.dotfiles and creates a");
    puts("   symlink to ~/.dotfiles in the old location.");
    puts("   then finds a dotfiles file in ~/.dotfiles/");
    puts("   and initializes from it. ");
    puts("");
    puts("dotfiles --track FILE NAME [--block BLOCK] [--host]");
    puts("   moves ~/FILE to ~/.dotfiles/NAME and creates a");
    puts("   symlink named ~/FILE pointing to ~/.dotfiles/NAME");
    puts("   if the --block flag and name are specified,");
    puts("   a symlink will be tracked in that block");
    puts("   otherwise, it will create a new block named");
    puts("   BLOCK_N, where N is some number, and add it there");
    puts("   if the host flag is specified, then the current");
    puts("   user@host sub-block will be where the symlink is");
    puts("   tracked. NOTE: this will NOT track symlinks!");
    puts("");
    puts("dotfiles --list [BLOCK] [--host USER@HOST]");
    puts("   lists all blocks by name, or if a BLOCK name is");
    puts("   specified, list only the contents of that block");
    puts("   if the host flag and HOST are provided, it will");
    puts("   list all the files for that user@host instead of");
    puts("   the current user#host");
    puts("dotfiles --check");
    puts("   check the status of current files, report any that");
    puts("   aren't pointing to the proper symlink");
}

char *get_user_home() {
    char *home = getenv("HOME");
    if (!home) {
        uid_t uid = geteuid ();
        struct passwd *pw = getpwuid (uid);
        home = pw->pw_dir;
    }
    return home;
}

char *get_dotfiles_dir() {
    char *home = get_user_home();
    int len = strlen(home) + 9;
    char *result = calloc(sizeof(char), len);
    sprintf(result, "%s/.dotfiles", home);
    return result;
}

// if a file does not start with /, then prepend /home/[USER]
char *get_home_file(char *s) {
    if (!s) {
        perror("cannot operate on negative string");
        exit(1);
    }
    if (s[0] == '/') {
        return s;
    }

    char *home = get_user_home();
    int more = 1;
    int len = strlen(home) + more + strlen(s);
    char *result = calloc(sizeof(char), len);
    sprintf(result, "%s/%s", home, s);
    return result;
}

char *get_git_dotfiles_file(char *s) {
    char *home = get_dotfiles_dir();
    int len = strlen(home) + 7 + strlen(s);
    char *result = calloc(sizeof(char), len);
    sprintf(result, "%s-remote/%s", home, s);
    return result;
}

char *get_dotfiles_file(char *s) {
    char *home = get_dotfiles_dir();
    int len = strlen(home) + 1 + strlen(s);
    char *result = calloc(sizeof(char), len);
    sprintf(result, "%s/%s", home, s);
    return result;
}

void sync_advanced(dmap *symlinks, char*(*modifier)(char *)) {
    char *name;
    char *lito;

    map_each(symlinks, name, lito) {
        name = get_home_file(name);
        char *name2 = get_home_file(name);
        if (is_symlink(name)) {
            if (unlink(name)) {
                puts("cannot unlink");
            }
        }
        puts(name2);
        if (symlink(modifier(lito), name2)) {
            puts(name2);
            printf("could not symlink file %s\n", name2);
        }
    }
}

void sync() {
    FILE *dots = fopen(get_dotfiles_file("dotfiles"), "r");
    if (dots) {
        dlist *lines = intern_lines(dots);
        dmap *symlinks = lines_to_symlinks(lines);
        sync_advanced(symlinks, &get_dotfiles_file);
    }
}

int does_file_exist(char *filename) {
    struct stat st;
    int result = stat(filename, &st);
    return result == 0;
}

void check() {
    FILE *dots = fopen(get_dotfiles_file("dotfiles"), "r");
    if (dots) {
        dlist *lines = intern_lines(dots);
        dmap *symlinks = lines_to_symlinks(lines);
        char *name;
        char *link;
        map_each(symlinks, name, link) {
            char *file = get_home_file(name);

            printf("checking ... (%s->%s)\n", file, link);
            if (is_symlink(file)) {
                printf("WARNING: %s is not properly linked!\n", file);
            }
        }
    }
}

int main(int argc, char **argv) {
    if (argc == 1) { // no args
        cmdlist();
        return 0;
    }

    if (strncmp(argv[1], "--git", 6) == 0) {
        char *gitcmd = calloc(sizeof(char), strlen(argv[2]) + 30);
        sprintf(gitcmd, "git clone %s ~/.dotfiles-remote", argv[2]);
        system("rm -rf ~/.dotfiles-remote");
        system(gitcmd);
        
        if (argc == 4) { // has a block
            char *block = calloc(sizeof(char), strlen(argv[3])+5);
            sprintf(block, "[[%s]]", argv[3]);
            
            FILE *dots = fopen(get_git_dotfiles_file("dotfiles"), "r");
            if (dots) {
                dlist *lines = intern_lines(dots);
                fclose(dots);
                dmap *in = lines_to_block_structure(lines);
                
                cblock *cblock = get(in, block);
                if (!cblock) {
                    perror("not a valid group, exiting...");
                    return 1;
                }
                dmap *syms = block_to_symlinks(cblock);
                sync_advanced(syms, &get_git_dotfiles_file);
            } else {
                perror("not a valid dotfiles repository");
                return 1;
            }
        }
    }

    if (strncmp(argv[1], "--sync", 7) == 0) {
        sync();
        return 0;
    }

    if (strncmp(argv[1], "--check", 8) == 0) {
        check();
        return 0;
    }

    if (strncmp(argv[1], "--init", 7) == 0) {
        char *dotfiles = get_dotfiles_dir();
        int status = mkdir(dotfiles, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (status) {
            perror("cannot create directory ~/.dotfiles. Exiting ...");
            return 1;
        }

        if (argc == 3) {
            char *mv = calloc(sizeof(char), 23+strlen(argv[2]));
            sprintf(mv, "mv %s/* ~/.dotfiles/", argv[2]);
            system(mv); // move all normal files
            sprintf(mv, "mv %s/.* ~/.dotfiles/", argv[2]);
            system(mv); // move all hidden files

            char *rmdir = calloc(sizeof(char), 7+strlen(argv[2]));
            sprintf(rmdir, "rmdir %s", argv[2]);
            system(rmdir);

            symlink(dotfiles, argv[2]);

            sync();
        }
        return 0;
    }

    if (strncmp(argv[1], "--track", 8) == 0) { // TODO some tweaks on tracking
        char *file = argv[2];
        char *name = get_dotfiles_file(argv[3]);
    
        if (does_file_exist(name)) {
            perror("can't track with that name, it already exists");
            return 1;
        }

        char *mv = calloc(sizeof(char), strlen(file)+5+strlen(name));
        sprintf(mv, "mv %s %s", file, name);
        system(mv);
        symlink(name, file);
        char *arrow = calloc(sizeof(char), strlen(file)+5+strlen(name));
        sprintf(arrow, "%s -> %s", argv[3], argv[2]);
     
        dmap *in;

        FILE *dots = fopen(get_dotfiles_file("dotfiles"), "r");
        if (dots) {
            dlist *lines = intern_lines(dots);
            in = lines_to_block_structure(lines);
            fclose(dots);
        } else {
            in = map_new();
        }

        char *blockname = 0;
        if (argc == 6) {
            if (strncmp(argv[4], "--block", 8)) {
                perror("invalid arguments!");
                cmdlist();
                return 1;
            }
            blockname = calloc(sizeof(char), 5+strlen(argv[5]));
            sprintf(blockname, "[[%s]]", argv[5]);
        } else {
            blockname = calloc(sizeof(char), 14);
            sprintf(blockname, "[[BLOCK_%i]]", in->size);
        }

        cblock *block = get(in, blockname);
        if (!block) {
            block = calloc(sizeof(cblock), 1);
            block->defaults = dlist_new();
            block->userhost = map_new();
            put(in, blockname, block);
        }
        dlist_add(block->defaults, arrow);

        dots = fopen(get_dotfiles_file("dotfiles"), "w");
        if (dots) {
            write_to_file(dots, in);
            fclose(dots);
        } else {
            perror("cannot open ~/.dotfiles/dotfiles for writing. Exiting...");
            return 1;
        }
    }

    if (strncmp(argv[1], "--list", 7) == 0) {
        FILE *dots = fopen(get_dotfiles_file("dotfiles"), "r");
        if (dots) {
            dlist *lines = intern_lines(dots);
            dmap *in = lines_to_block_structure(lines);
            if (argc == 2) {              
                write_to_file(stdout, in);
            }

            if (argc == 3) {
                char *block = calloc(sizeof(char), 5+strlen(argv[2]));
                sprintf(block, "[[%s]]", argv[2]);

                cblock *host = get(in, block);
                if (host) {
                    hblock *h = 0;
                    char *out = 0;
                    each(host->defaults, out) {
                        puts(out);
                    }
                    map_each(host->userhost, out, h) {
                        puts(out);
                        each(h->symlinks, out) {
                            puts(out);
                        }
                    }
                }
            }
        }
        return 0;
    }
}

