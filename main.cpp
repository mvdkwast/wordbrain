/* This is a wordbrain puzzle solver.
 * 
 * Create a compiled dictionary first. You will have to find your own word
 * list, I can't distribute any. It must be sorted and not contain any
 * duplicates.
 * 
 *     $wordbrain -c word_list.txt words.tree
 *
 * Now solve puzzles:
 *
 *     $wordbrain -d words.tree geto atuc tegr uaoc 4 6 6
 *
 *
 * This code is copyright (c) 2015 Martijn van der Kwast <martijn@vdkwast.com> 
 * and released under the MIT license. (Which basically means: use and
 * distribute freely but leave the copyright note)
 */

#include <QString>
#include <QList>
#include <QFile>
#include <string.h>
#include <iostream>
#include <sysexits.h>

#include <QTextStream>
#include <QElapsedTimer>                        // for benchmarking

#include <QDebug>

// Builtin constants, may be changed
#define MAX_SIZE 7                             // Maximum number of squares composing the side of the (square) grid.
#define MAX_WORDS 10                           // Maximum number of words to find
#define MAX_NODES 1500000L                     // Maximum number of nodes in the dictionary tree

// #define DO_BENCHMARK                        // if set disable normal output and display benchmark times

// don't touch !
#define MAX_POS (MAX_SIZE*MAX_SIZE)
#define LONGEST_WORD MAX_POS

// magic header to recognize a compile dictionary
static const char MAGIC_COMPILED_DICT_HEADER[] = { 'v', 'd', 'k', 'D', '1', 'c', '7' };

// options that may be set on the command-line
int size = 4;                                   // default bord size
int nwords = 0;                                 // number of words to find
int word_sizes[MAX_WORDS];                      // sizes of words to finds
bool opt_compile = false;                       // if true compile word list instead 
                                                // of solving puzzle
char start_board[MAX_SIZE*MAX_SIZE+1];          // board given on cmdline

// the dictionary must be sorted and may not contain any duplicates or behaviour
// will be undefined in the worst imaginable way.
QString dict_file = "mots.txt";                 // original word list
QString compiled_dict_file = "mots.tree";       // compiled word list

/*
 * Read word list from file into dict_data, with a pointer to each word
 * in words. (dict_data must not be cleared wile words is used.)
 *
 * Words are stored 1 per line in the file.
 */

QByteArray dict_data;
QList<char *> words;

void slurp() {
    QFile f(dict_file);
    if (!f.open(QIODevice::ReadOnly)) {
        std::cerr << "Liste de mots non trouvée: " << qPrintable(dict_file) << std::endl;
        exit(EX_UNAVAILABLE);
    }

    dict_data = f.readAll();
    char *s = dict_data.data();
    char *start = s;

    while (*s) {
        if (*s == '\n') {
            *s = 0;
            words << start;
            start = s + 1;
        }
        s++;
    }
}

/*
 * Represent dictionary as a tree
 * valid words are finished with '*' node.
 *
 * All nodes are allocated in one step to avoid calling malloc many times.
 */
struct wnode_t {
    char c;
    wnode_t *next;
    wnode_t *first_child;
};

wnode_t top { 0, 0, 0 };

static wnode_t wnodes[MAX_NODES];
size_t nnodes = 0;

wnode_t *alloc_node() {
    if (nnodes >= MAX_NODES) {
        std::cerr << "Too many word nodes in dictionary tree. Increase MAX_NODES and recompile" << std::endl;
        exit(EX_SOFTWARE);
    }

    return &wnodes[nnodes++];
}

wnode_t *find_child(wnode_t *parent, char c) {
    for (wnode_t *child=parent->first_child; child; child=child->next) {
        if (child->c == c)
            return child;
    }

    return 0;
}

void add_child(wnode_t *parent, wnode_t *child) {
    wnode_t *node = parent->first_child;
    if (!node) {
        parent->first_child = child;
    }
    else {
        wnode_t *last = node;
        for (; node; node=node->next) {
            last = node;
        }
        last->next = child;
    }
}

wnode_t *add_letter(wnode_t *parent, char c) {
    wnode_t *child = find_child(parent, c);
    if (!child) {
        child = alloc_node();
        child->c = c;
        add_child(parent, child);
    }
    return child;
}

void build_dict_tree() {
    for (char *word: words) {
        char *s = word;
        wnode_t *node = &top;
        while (*s) {
            node = add_letter(node, *s++);
        }
        add_letter(node, '*');
    }
}

void compile_dict() {
    char empty[] = "";
    char *prev = empty;

    QFile out(compiled_dict_file);
    out.open(QIODevice::WriteOnly);
    QTextStream rvr(&out);

    rvr << MAGIC_COMPILED_DICT_HEADER;

    for (char *word: words) {
        char *a = prev;
        char *b = word;
        int i=0;
        while (1) {
            if (*a == *b) {
                i++;
                a++;
                b++;
                continue;
            }

            if (!*a) {
                rvr << '*' << &word[i];
                break;
            }
            else if (!*b) {
                std::cerr << "Dictionnaire non trié" << std::endl;
                exit(EX_DATAERR);
                break;
            }
            else {
                rvr << strlen(prev)-i << &word[i];
                break;
            }
        }

        prev = word;
    }
}

void load_compiled_dict() {
    QFile f(compiled_dict_file);
    if (!f.open(QIODevice::ReadOnly)) {
        std::cerr << "Dictionnaire compilé non trouvé: " << qPrintable(compiled_dict_file) << std::endl;
        exit(EX_UNAVAILABLE);
    }

    QByteArray d = f.readAll();

    char *s = d.data();
    for (size_t i=0; i<sizeof(MAGIC_COMPILED_DICT_HEADER); ++i) {
        if (*s++ != MAGIC_COMPILED_DICT_HEADER[i]) {
            std::cerr << "Fichier dictionnaire invalide: " << qPrintable(compiled_dict_file) << std::endl
                      << "L'avez-vous compilé avec l'option -c ?" << std::endl;
            exit(EX_DATAERR);
        }
    }

    wnode_t *node = &top;

    wnode_t *stack[LONGEST_WORD];
    int pos = 0;

    int nWords = 0;

    while (*s) {
        if (*s == '*') {
            add_letter(node, '*');
            s++;
            nWords++;
        }
        else if (*s >= '0' && *s <= '9') {
            add_letter(node, '*');
            nWords++;

            int n = 0;
            while (*s >= '0' && *s <= '9') {
                n *= 10;
                n += (*s - '0');
                s++;
            }
            for (; n>0; --n) {
                pos--;
                node = stack[pos];
            }  
        }      
        else   {
            stack[pos++] = node;
            node = add_letter(node, *s);
            s++;
        }
    }

    // finish last word
    add_letter(node, '*');

    std::cout << "Le dictionnaire contient " << nWords << " mots." << std::endl;
}

/*
 * Search state for one word search
 */
struct state_t {
    char board[MAX_SIZE*MAX_SIZE];
    char seen_[MAX_SIZE];
    wnode_t *nstack[MAX_POS];
    int xs[MAX_POS];
    int ys[MAX_POS];
    int pos = 0;
    wnode_t *cur = 0;

    void mk_initial_state();
    void mk_next_state(state_t *ps);

    void reset_board();
    char board_letter(int x, int y);
    void board_gravity();

    void clear_seen();
    bool seen(int x, int y);
    void set_seen(int x, int y);
    void unset_seen(int x, int y);

    QString cword();
    void push_wnode(wnode_t *n);
    void pop_wnode();

    void next_pos(int x, int y);
    void find_words();

    void disp_cword();
} state_stack[MAX_WORDS+1];

int cur_word = 0;

void stash_state() {
    cur_word++;
    state_t *ps = &state_stack[cur_word-1];
    state_stack[cur_word].mk_next_state(ps);
}

void unstash_state() {
    cur_word--;
}

/*
 * state constructors
 */

// copy board from original board
void state_t::mk_initial_state() {
    reset_board();
    clear_seen();
}

// copy from previous state, and compact board by applying gravity to letters so they
// fall through holes left by previous step.
void state_t::mk_next_state(state_t *ps) {
    clear_seen();
    memcpy(board, ps->board, sizeof(board));

    for (int y=0; y<size; ++y) {
        for (int x=0; x<size; ++x) {
            if (ps->seen(x, y))
                board[y*size+x] = ' ';
        }
    }

    board_gravity();
}

void state_t::find_words() {
    for (int x=0; x<size; ++x) {
        for (int y=0; y<size; ++y) {
            pos = 0;
            cur = &top;
            next_pos(x, y);
        }
    }
}

/*
 * Operations on game board
 */
void state_t::reset_board() {
    memcpy(board, start_board, sizeof(board));
}

char state_t::board_letter(int x, int y) {
    return board[y*size+x];
}

void state_t::board_gravity() {
    for (int x=0; x<size; ++x) {
        int j=size-1;
        for (int y=size-1; y>=0; --y) {
            char c = board[y*size+x];
            if (c != ' ') {
                board[j*size+x] = c;
                j--;
            }
        }
        for (; j>=0; --j) {
            board[j*size+x] = ' ';
            set_seen(x, j);
        }
    }
}

/*
 * Seen tiles in board
 */
void state_t::clear_seen() {
    memset(seen_, 0, sizeof(seen_));
}
bool state_t::seen(int x, int y) {
    return seen_[y] & (1<<x);
}
void state_t::set_seen(int x, int y) {
    seen_[y] |= (1<<x);
}
void state_t::unset_seen(int x, int y) {
    seen_[y] &= ~(1<<x);
}

/*
 * current word as stack of wnode_t
 */
QString state_t::cword() {
    QString w;
    for (int i=0; i<pos; ++i) {
        w += nstack[i]->c;
    }
    return w;
}

void state_t::push_wnode(wnode_t *n) {
    nstack[pos++] = n;
    cur = n;
}

void state_t::pop_wnode() {
    --pos;
    cur = nstack[pos-1];
}

/*
 * try next position on board
 */
void state_t::next_pos(int x, int y) {
    if (x < 0 || y < 0 || x >= size || y >= size) 
        return;

    if (seen(x, y)) 
        return;

    char c = board_letter(x, y);

    wnode_t *n = find_child(cur, c);
    if (!n) {
        return;
    }

    xs[pos] = x;
    ys[pos] = y;
    push_wnode(n);
    set_seen(x, y);
                                
    if (word_sizes[cur_word] == pos && n->first_child->c == '*') {
        if (cur_word == nwords-1) {
#ifndef DO_BENCHMARK
            disp_cword();
#endif
            unset_seen(x, y);
            pop_wnode();
            return;
        }

        if (cur_word < nwords) {
            stash_state();
            state_stack[cur_word].find_words();
            unstash_state();
        }
    }
                                
    next_pos(x-1, y);               
    next_pos(x+1, y);
    next_pos(x, y-1);
    next_pos(x, y+1);

    next_pos(x-1, y-1);
    next_pos(x-1, y+1);
    next_pos(x+1, y-1);
    next_pos(x+1, y+1);

    unset_seen(x, y);
    pop_wnode();
}

/*
 * display current solution (every word and which letter to pick in what order for every word)
 */
void state_t::disp_cword() {
    std::cout << "****************************************************************************" << std::endl;

    for (int i=0; i<=cur_word; ++i) {
        std::cout << qPrintable(state_stack[i].cword());
        if (i != cur_word) 
            std::cout << " + ";
    }
    std::cout << std::endl;

    for (int y=0; y<size; ++y) {
        for (int i=0; i<=cur_word; ++i) {
            for (int x=0; x<size; ++x) {
                char c = state_stack[i].board[y*size+x];
                std::cout << c;

                char n = ' ';
                for (int j=0; j<state_stack[i].pos; ++j) {
                    if (state_stack[i].xs[j] == x && state_stack[i].ys[j] == y) {
                        n = j+'1';
                    }
                }
                std::cout << n << ' ';
            }
            std::cout << "    |    ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

// remove n arguments at pos from command-line args
void pop_args(int *argc, char **argv, int pos, int n) {
    for (int i=pos; i<*argc+n; ++i) {
        argv[i] = argv[i+n];
    }

    *argc -= n;
}

void die_with_syntax() {
    std::cerr << "syntax: wordbrain [-d mots.tree] {line1, ...} {len1, ...}" << std::endl;
    std::cerr << "        wordbrain -c mots.txt mots.tree" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Exemple: wordbrain oeuf niis cmll alba 6 5 5" << std::endl;
    exit(EX_USAGE);
}

int main(int argc, char *argv[]) {
#ifdef DO_BENCHMARK
    quint64 tload, tcalc;
#endif

    if (argc > 2) {
        if (!strcmp(argv[1], "-c")) {
            if (argc != 4) {
                std::cerr << "L'option -c nécessite 2 arguments: liste de mots et fichier compilé" << std::endl;
                die_with_syntax();
            }

            dict_file = argv[2];
            compiled_dict_file = argv[3];

            slurp();
            compile_dict();
            std::cout << "Compilation d'un dictionnaire de " << words.length() << " mots." << std::endl;
            exit(EX_OK);
        }
    }

    for (int i=1; i<argc; ++i) {
        if (!strcmp(argv[i], "-d")) {
            if (i > argc-2) {
                std::cerr << "L'option -d nécessite de spécifier un dictionnaire" << std::endl;
                die_with_syntax();
            }
            compiled_dict_file = argv[i+1];
            pop_args(&argc, argv, i, 2);
        }
    }

    if (argc < 4) {
        die_with_syntax();
    }

    size = strlen(argv[1]);

    if (size >= MAX_SIZE) {
        std::cerr << "Taille de jeu trop importante. Augmenter MAX_SIZE et recompiler" << std::endl;
        exit(EX_SOFTWARE);
    }

    if (argc < 1 + size) {
        std::cerr << "Pas assez d'arguments pour cette taille." << std::endl;
        die_with_syntax();
    }
    else if (argc == 1+size) {
        std::cerr << "Tailles des mots non spéficiées." << std::endl;
        die_with_syntax();
    }

    for (int i=0; i<size; ++i) {
        if ((int)strlen(argv[1+i]) != size) {
            std::cerr << "Toutes les lignes doivent avoir la même taille" << std::endl;
            exit(EX_USAGE);
        }
        strcpy(start_board+i*size, argv[1+i]);
    }
    
    nwords = argc - size - 1;
    int total = 0;
    for (int i=size+1; i<argc; ++i) {
        bool isInt;
        int n = QString(argv[i]).toInt(&isInt);
        if (!isInt) {
            std::cerr << argv[i] << " n'est pas un chiffre" << std::endl;
            exit(EX_USAGE);
        }
        word_sizes[i-size-1] = n;
        total += n;
    }

    if (total != size*size) {
        std::cerr << "Total de la longueur des mots pas égal à la somme des lettres" << std::endl;
        exit(EX_USAGE);
    }

#ifdef DO_BENCHMARK
    QElapsedTimer tmr;
    tmr.start();
#endif

    load_compiled_dict();
    // qDebug() << "Allocated" << nnodes << "nodes";

#ifdef DO_BENCHMARK
    tload = tmr.nsecsElapsed();
    tmr.restart();
#endif

    state_stack[0].mk_initial_state();

    state_stack[0].find_words();

#ifdef DO_BENCHMARK
    tcalc = tmr.nsecsElapsed();
    std::cout << "load: " << qPrintable(QString("%1 ms").arg(tload/1000000., 0, 'f', 1)) << std::endl;
    std::cout << "lcalc: " << qPrintable(QString("%1 ms").arg(tcalc/1000000., 0, 'f', 1)) << std::endl;
#endif
}
