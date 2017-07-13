#include <string.h>
#include <vector>

#include "utils.hpp"
#include "position.hpp"
#include "reader.hpp"
#include "lexical.hpp"
#include "syntactical.hpp"
#include "machine.hpp"
#include "modules.hpp"
#include "eval.hpp"
#include "builtin.hpp"

#define EXECUTABLE_NAME "egel"

#define EXECUTABLE_VERSION_MAJOR   "0"
#define EXECUTABLE_VERSION_MINOR   "0"
#define EXECUTABLE_VERSION_RELEASE "x"

#define EXECUTABLE_VERSION \
    EXECUTABLE_VERSION_MAJOR "." \
    EXECUTABLE_VERSION_MINOR "." \
    EXECUTABLE_VERSION_RELEASE

#define EXECUTABLE_COPYRIGHT \
    "Copyright (C) 2016"
#define EXECUTABLE_AUTHORS \
    "M.C.A. (Marco) Devillers"

typedef enum {
    OPTION_NONE,
    OPTION_FILE,
    OPTION_DIR,
    OPTION_NUMBER,
    OPTION_TEXT,
} arg_t;

typedef struct {
    char const*     shortname;
    char const*     longname;
    arg_t           argument;
    char const*     description;
} option_t;

static option_t options[] = {
    "-h", "--help",    OPTION_NONE, "display usage",
    "-v", "--version", OPTION_NONE, "display version",
    "-",  "--in",      OPTION_NONE, "interactive mode (default)",
    "-I", "--include", OPTION_DIR,  "add include directory",
    "-L", "--library", OPTION_DIR,  "add library directory",
    "-o", "--output",  OPTION_FILE, "output to file",
    "-T", "--tokens",  OPTION_NONE, "output all tokens (debug)",
    "-U", "--unparse", OPTION_NONE, "output the parse tree (debug)",
    "-X", "--check",   OPTION_NONE, "output analyzed tree (debug)",
    "-D", "--desugar", OPTION_NONE, "output desugared tree (debug)",
    "-C", "--lift",    OPTION_NONE, "output combinator lifted tree (debug)",
    "-B", "--bytes",   OPTION_NONE, "output bytecode (debug)",
};

#define OPTIONS_SIZE    (sizeof(options)/sizeof(option_t))

typedef std::vector<std::pair<UnicodeString, UnicodeString> > StringPairs;

StringPairs parse_options(int argc, char *argv[]) {
    StringPairs pp;
    for (int a = 1; a < argc; a++) {
        uint_t sz = pp.size();

        for(uint_t i = 0; i < OPTIONS_SIZE; i++) {
            
            if ( (strncmp(argv[a], options[i].shortname, 32) == 0) ||
                 (strncmp(argv[a], options[i].longname, 32) == 0) ) {
                
                switch (options[i].argument) {
                case OPTION_NONE:
                    pp.push_back(std::make_pair(UnicodeString(options[i].shortname), UnicodeString("")));
                    break;
                case OPTION_FILE:
                    if (a == argc - 1) goto options_error;
                    pp.push_back(std::make_pair(UnicodeString(options[i].shortname), UnicodeString(argv[a+1])));
                    a++;
                    break;
                case OPTION_DIR:
                    if (a == argc - 1) goto options_error;
                    pp.push_back(std::make_pair(UnicodeString(options[i].shortname), UnicodeString(argv[a+1])));
                    a++;
                    break;
                case OPTION_NUMBER:
                    if (a == argc - 1) goto options_error;
                    pp.push_back(std::make_pair(UnicodeString(options[i].shortname), UnicodeString(argv[a+1])));
                    a++;
                    break;
                case OPTION_TEXT:
                    if (a == argc - 1) goto options_error;
                    pp.push_back(std::make_pair(UnicodeString(options[i].shortname), UnicodeString(argv[a+1])));
                    a++;
                    break;
                };
            };

        }

        if (sz == pp.size()) {
            pp.push_back(std::make_pair(UnicodeString("--"), UnicodeString(argv[a])));
        }
    }
    return pp;

options_error:
    throw 0;
}

void display_usage () {
    std::cout << "Usage: " << EXECUTABLE_NAME << " [options] filename" << std::endl;
    std::cout << "Options:" << std::endl;

    for (uint_t i = 0; i < OPTIONS_SIZE; i++) {
        std::cout << "\t[" << options[i].shortname << "|" << options[i].longname << "] ";
        switch (options[i].argument) {
        case OPTION_NONE:
            std::cout << "      "; 
            break;
        case OPTION_FILE:
            std::cout << "<file>"; 
            break;
        case OPTION_DIR:
            std::cout << "<dir>"; 
            break;
        case OPTION_NUMBER:
            std::cout << "<num>"; 
            break;
        case OPTION_TEXT:
            std::cout << "<text>"; 
            break;
        };
        std::cout << "\t" << options[i].description << std::endl;
    };
}

void display_version () {
    std::cout << EXECUTABLE_NAME << ' ' << EXECUTABLE_VERSION << std::endl;
    std::cout << EXECUTABLE_COPYRIGHT << ' ' << EXECUTABLE_AUTHORS << std::endl;
}

int main(int argc, char *argv[]) {
    // parse the options (quick and dirty)
    StringPairs pp;
    try {
        pp = parse_options(argc, argv);
    } catch (int ex) {
        std::cerr << "options error, try -h." << std::endl;
        return (EXIT_FAILURE);
    };
    
    // check for -h or -v
    for (auto& p : pp) {
        if (p.first == ("-h")) {
            display_usage();
            return (EXIT_SUCCESS);
        };
        if (p.first == ("-v")) {
            display_version();
            return (EXIT_SUCCESS);
        };
    };

    // check for include and library paths
    OptionsPtr oo = Options().clone();
    for (auto& p : pp) {
        if (p.first == ("-I")) {
            oo->add_include_path(p.second);
        };
        if (p.first == ("-L")) {
            oo->add_library_path(p.second);
        };
    };

    // check for flags
    for (auto& p : pp) {
        if (p.first == ("-")) {
            oo->set_interactive(true);
        };
        if (p.first == ("-T")) {
            oo->set_tokenize(true);
        };
        if (p.first == ("-U")) {
            oo->set_unparse(true);
        };
        if (p.first == ("-X")) {
            oo->set_semantical(true);
        };
        if (p.first == ("-D")) {
            oo->set_desugar(true);
        };
        if (p.first == ("-C")) {
            oo->set_lift(true);
        };
        if (p.first == ("-B")) {
            oo->set_bytecode(true);
        };
    };

    // check for unique --/fn
    UnicodeString fn;
    for (auto& p : pp) {
        if (p.first == ("--")) {
            if (fn == "") {
                fn = p.second;
            }
        }
    };

    // start up the module system
    ModuleManager mm;
    mm.set_options(oo);
    Machine m;
    mm.set_machine(&m);
    NamespacePtr env = namespace_nil();
    mm.set_environment(env);

    // load the file
    if (fn != "") {
        Position p("",0,0);
        try {
            mm.load(p, fn);
        } catch (Error e) {
            std::cerr << e << std::endl;
            return (EXIT_FAILURE);
        }
    } else {
        mm.prelude();
    }

    if ((fn == "") || oo->interactive()) {
        eval_interactive(mm);
    } else {
        eval_main(mm);
    }

    return EXIT_SUCCESS;
}
