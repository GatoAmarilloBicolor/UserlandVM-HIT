/*
 * GNU libc wrapper functions for Haiku
 * Provides minimal implementations of GNU-specific functions
 * used by ported GNU utilities like coreutils, findutils, etc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// Memory allocation wrappers
// These wrap the standard malloc/calloc/realloc with error handling

void* xmalloc(size_t size)
{
    if (size == 0) size = 1;
    void* ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "memory exhausted\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void* xcalloc(size_t count, size_t size)
{
    if (count == 0 || size == 0) {
        count = 1;
        size = 1;
    }
    void* ptr = calloc(count, size);
    if (ptr == NULL) {
        fprintf(stderr, "memory exhausted\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void* xrealloc(void* ptr, size_t size)
{
    if (size == 0) size = 1;
    void* new_ptr = realloc(ptr, size);
    if (new_ptr == NULL) {
        fprintf(stderr, "memory exhausted\n");
        exit(EXIT_FAILURE);
    }
    return new_ptr;
}

void* xcharalloc(size_t size)
{
    return xmalloc(size);
}

void* xreallocarray(void* ptr, size_t count, size_t size)
{
    // Check for integer overflow
    if (count > 0 && size > 0 && count > (size_t)-1 / size) {
        fprintf(stderr, "memory allocation overflow\n");
        exit(EXIT_FAILURE);
    }
    return xrealloc(ptr, count * size);
}

void* xireallocarray(void* ptr, size_t count, size_t size)
{
    return xreallocarray(ptr, count, size);
}

void* x2nrealloc(void* ptr, size_t *pn, size_t s)
{
    size_t n = *pn;
    if (n + 1 == 0) {
        fprintf(stderr, "allocation size overflow\n");
        exit(EXIT_FAILURE);
    }
    if (n == 0) n = 1;
    else if (n < 1024) n *= 2;
    else n = n * 1.5;
    *pn = n;
    return xrealloc(ptr, n * s);
}

void* xmemdup(const void *p, size_t s)
{
    void *new = xmalloc(s);
    memcpy(new, p, s);
    return new;
}

char* ximalloc(size_t size)
{
    return (char*)xmalloc(size);
}

char* xicalloc(size_t count, size_t size)
{
    return (char*)xcalloc(count, size);
}

// Error handling (simplified)
// Real error() is complex with format strings, this is minimal

int error_message_count = 0;
void (*error_print_progname)(void) = NULL;
const char* program_name_val = "program";
int exit_failure = 1;
int error_one_per_line = 0;

void error(int status, int errnum, const char *message, ...)
{
    va_list args;
    va_start(args, message);
    
    if (error_print_progname != NULL) {
        error_print_progname();
    } else if (program_name_val) {
        fprintf(stderr, "%s: ", program_name_val);
    }
    
    vfprintf(stderr, message, args);
    
    if (errnum != 0) {
        fprintf(stderr, ": %s", strerror(errnum));
    }
    
    fprintf(stderr, "\n");
    
    va_end(args);
    
    error_message_count++;
    
    if (status != 0) {
        exit(status);
    }
}

// Quoting functions (stubs - just return input or simple quoted version)
const char* quotearg_n_custom_mem(int n, const char *start, size_t len,
                                   const char *left_quote,
                                   const char *right_quote)
{
    static char buffer[8192];
    snprintf(buffer, sizeof(buffer), "%s%.*s%s", 
             left_quote ? left_quote : "", 
             (int)len, start ? start : "", 
             right_quote ? right_quote : "");
    return buffer;
}

const char* quotearg_n_custom(int n, const char *str,
                               const char *left_quote,
                               const char *right_quote)
{
    if (!str) return "";
    return quotearg_n_custom_mem(n, str, strlen(str), left_quote, right_quote);
}

const char* quotearg_n_mem(int n, const char *str, size_t len)
{
    return quotearg_n_custom_mem(n, str, len, "'", "'");
}

const char* quotearg_n(int n, const char *str)
{
    if (!str) return "";
    return quotearg_n_mem(n, str, strlen(str));
}

const char* quotearg_char_mem(const char *str, size_t len, char c)
{
    static char buffer[8192];
    snprintf(buffer, sizeof(buffer), "'%.*s'", (int)len, str ? str : "");
    return buffer;
}

const char* quotearg_char(const char *str, char c)
{
    if (!str) return "";
    return quotearg_char_mem(str, strlen(str), c);
}

const char* quotearg_colon(const char *str)
{
    return quotearg_char(str, ':');
}

const char* quotearg_n_style(int n, int style, const char *str)
{
    if (!str) return "";
    return quotearg_n(n, str);
}

const char* quotearg_n_style_mem(int n, int style, const char *str, size_t len)
{
    return quotearg_n_custom_mem(n, str, len, "'", "'");
}

const char* quotearg_n_mem(int n, const char *str, size_t len);

const char* quotearg_alloc_mem(const char *str, size_t len,
                                size_t *qsize)
{
    static char buffer[8192];
    int len_val = snprintf(buffer, sizeof(buffer), "'%.*s'", (int)len, str ? str : "");
    if (qsize) *qsize = len_val;
    return buffer;
}

const char* quote_n(int n, const char *str)
{
    if (!str) return "";
    return quotearg_n(n, str);
}

const char* quote_n_mem(const char *str, size_t len)
{
    return quotearg_n_custom_mem(0, str, len, "'", "'");
}

// Program name handling
void set_program_name(const char *name)
{
    program_name_val = name;
}

const char* getprogname(void)
{
    return program_name_val;
}

// Version and help functions (stubs)
const char* version_etc_copyright = "Copyright (C) 2025";

void version_etc(FILE *stream, const char *progname, const char *version,
                 const char *copyright, const char *authors, ...)
{
    if (!stream) stream = stdout;
    fprintf(stream, "%s %s\n", progname, version);
}

void version_etc_arn(FILE *stream, const char *progname, const char *version,
                     const char *copyright, size_t n_authors, const char **authors)
{
    version_etc(stream, progname, version, copyright, NULL);
}

void version_etc_va(FILE *stream, const char *progname, const char *version,
                    const char *copyright, va_list authors)
{
    version_etc(stream, progname, version, copyright, NULL);
}

// Locale/encoding functions (stubs)
const char* locale_charset(void)
{
    return "UTF-8";
}

int rpl_mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps)
{
    // Simplified implementation
    if (!s) return 0;
    if (n == 0) return -2;
    if ((unsigned char)*s < 128) {
        if (pwc) *pwc = (unsigned char)*s;
        return 1;
    }
    return -1; // Invalid multibyte sequence
}

// Stream/file functions (mostly no-ops or delegating)
int close_stream(FILE *stream)
{
    if (!stream) return 0;
    return fclose(stream);
}

void fseterr(FILE *fp)
{
    // Mark stream as having error - typically just sets error flag
    if (fp) {
        // In libc, this sets an internal error flag
        // Here we just return since we can't modify FILE internals
    }
}

int rpl_fclose(FILE *stream)
{
    return close_stream(stream);
}

int rpl_fflush(FILE *stream)
{
    return fflush(stream);
}

// Locale locale handling (stubs)
int hard_locale(int category)
{
    // Return 1 if locale is non-trivial, 0 if C locale
    return 0;  // Default to C locale
}

char* setlocale_null_r(int category, char *buf, size_t buflen)
{
    const char* locale = "C";
    if (buflen < strlen(locale) + 1) return NULL;
    strcpy(buf, locale);
    return buf;
}

// Quoting options (stub structure)
typedef struct {
    int style;
    char left_quote;
    char right_quote;
} quoting_options;

quoting_options quote_quoting_options = {0, '\'', '\''};

void set_char_quoting(quoting_options *o, unsigned char c, int i)
{
    // No-op for stub
}

void set_custom_quoting(quoting_options *o, const char *left,
                        const char *right)
{
    // No-op for stub
}

// String functions - most are in libc
int rpl_nl_langinfo(nl_item item)
{
    return nl_langinfo(item);
}

// Memory allocation with error tracking
extern void xalloc_die(void);  // Forward declare

void xalloc_die(void)
{
    fprintf(stderr, "virtual memory exhausted\n");
    exit(EXIT_FAILURE);
}

// Usage function (stub)
void usage(int status)
{
    fprintf(stderr, "Try 'help' for more information.\n");
    exit(status);
}

// Alternate names and compatibility functions
char* rpl_malloc(size_t size)
{
    return (char*)xmalloc(size);
}

char* rpl_calloc(size_t count, size_t size)
{
    return (char*)xcalloc(count, size);
}

char* rpl_realloc(void* ptr, size_t size)
{
    return (char*)xrealloc(ptr, size);
}

// rpl_* functions (replacement functions for broken implementations)
// Most just delegate to the standard versions

FILE* rpl_fseeko(FILE *fp, off_t offset, int whence)
{
    if (fseek(fp, offset, whence) == 0) return fp;
    return NULL;
}

int rpl_vfprintf(FILE *fp, const char *format, va_list args)
{
    return vfprintf(fp, format, args);
}

typedef int (*printf_function) (FILE *, const char *, va_list);
int printf_parse(const char *format, printf_function print_func, FILE *stream, int *argpos)
{
    // Stub - just call printf with the format
    return fprintf(stream, "%s", format);
}

int printf_fetchargs(va_list args, void *spec)
{
    return 0;  // Stub
}

typedef struct {
    unsigned int flags;
    size_t width;
    int left_precision;
    int right_precision;
    size_t pad_width;
    char conversion;
} printf_spec;

// vasnprintf - format string with dynamic allocation  
char* vasnprintf(char *resultbuf, size_t *lengthp,
                 const char *format, va_list args)
{
    // Simplified: use fixed buffer
    static char buffer[16384];
    vsnprintf(buffer, sizeof(buffer), format, args);
    if (lengthp) *lengthp = strlen(buffer);
    return buffer;
}

// Glob function (stub)
int globfree(void *pglob)
{
    // Stub - nothing to free
    return 0;
}

// ============================================================================
// DIRECTORY & FILE OPERATIONS
// ============================================================================

struct dirent {
    long d_ino;
    off_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[256];
};

typedef struct {
    int fd;
    struct dirent entry;
    char path[1024];
} DIR;

DIR* opendir(const char *name)
{
    DIR *dir = (DIR*)malloc(sizeof(DIR));
    if (!dir) return NULL;
    // Simplified: just store the path
    strncpy(dir->path, name ? name : ".", sizeof(dir->path)-1);
    dir->path[sizeof(dir->path)-1] = '\0';
    dir->fd = -1;
    return dir;
}

struct dirent* readdir(DIR *dir)
{
    if (!dir) return NULL;
    // Stub: return NULL to indicate no more entries
    return NULL;
}

int closedir(DIR *dir)
{
    if (!dir) return -1;
    free(dir);
    return 0;
}

// Directory entry test functions
int is_dir(const struct dirent *d) { return 0; }
int is_lnk(const struct dirent *d) { return 0; }
int is_reg(const struct dirent *d) { return 1; }

// ============================================================================
// PATH & STRING UTILITIES
// ============================================================================

char* getcwd_compat(char *buf, size_t size)
{
    const char *cwd = "/";
    if (buf == NULL) {
        buf = (char*)malloc(size ? size : 256);
        if (!buf) return NULL;
    }
    strncpy(buf, cwd, size-1);
    if (size > 0) buf[size-1] = '\0';
    return buf;
}

char* realpath_compat(const char *path, char *resolved)
{
    if (!path) return NULL;
    if (resolved == NULL) {
        resolved = (char*)malloc(1024);
        if (!resolved) return NULL;
    }
    strncpy(resolved, path, 1023);
    resolved[1023] = '\0';
    return resolved;
}

int fnmatch_stub(const char *pattern, const char *string, int flags)
{
    // Extremely simplified: just do strcmp
    if (!pattern || !string) return -1;
    return strcmp(pattern, string) == 0 ? 0 : 1;
}

// ============================================================================
// GETTEXT STUB (i18n)
// ============================================================================

const char* gettext(const char *msgid)
{
    return msgid;
}

const char* ngettext(const char *msgid1, const char *msgid2, unsigned long n)
{
    return n == 1 ? msgid1 : msgid2;
}

const char* dgettext(const char *domainname, const char *msgid)
{
    return msgid;
}

const char* dcgettext(const char *domainname, const char *msgid, int category)
{
    return msgid;
}

const char* bindtextdomain(const char *domainname, const char *dirname)
{
    return dirname;
}

const char* textdomain(const char *domainname)
{
    return domainname;
}

// ============================================================================
// ENVIRONMENT & SIGNAL HANDLING
// ============================================================================

const char* getenv_override(const char *name)
{
    return getenv(name);
}

int setenv_override(const char *name, const char *value, int overwrite)
{
    return setenv(name, value, overwrite);
}

int unsetenv_override(const char *name)
{
    return unsetenv(name);
}

// Signal handling (stubs)
typedef void (*sighandler_t)(int);
sighandler_t signal_stub(int signum, sighandler_t handler)
{
    // Stub - just return the handler
    return handler;
}

// ============================================================================
// ASSERT & DEBUGGING
// ============================================================================

void __assert_fail(const char *assertion, const char *file,
                   unsigned int line, const char *function)
{
    fprintf(stderr, "Assertion failed: %s at %s:%u in %s\n",
            assertion, file, line, function ? function : "?");
    exit(EXIT_FAILURE);
}

void __assert(const char *assertion, const char *file, int line)
{
    fprintf(stderr, "Assertion failed: %s at %s:%d\n",
            assertion, file, line);
    exit(EXIT_FAILURE);
}

// ============================================================================
// REGEX (MINIMAL STUB)
// ============================================================================

typedef struct {
    int unused;
} regex_t;

typedef struct {
    int rm_so;
    int rm_eo;
} regmatch_t;

int regcomp(regex_t *preg, const char *pattern, int cflags)
{
    return 0; // Success stub
}

int regexec(const regex_t *preg, const char *string, size_t nmatch,
            regmatch_t pmatch[], int eflags)
{
    return 1; // No match stub
}

void regfree(regex_t *preg)
{
    // No-op
}

size_t regerror(int errcode, const regex_t *preg,
                char *errbuf, size_t errbuf_size)
{
    const char *msg = "Regex error";
    size_t len = strlen(msg);
    if (errbuf && errbuf_size > 0) {
        strncpy(errbuf, msg, errbuf_size-1);
        errbuf[errbuf_size-1] = '\0';
    }
    return len + 1;
}

// ============================================================================
// TIME FUNCTIONS (STUBS)
// ============================================================================

typedef long time_t;
typedef struct {
    long tv_sec;
    long tv_usec;
} timeval_t;

int gettimeofday(timeval_t *tv, void *tz)
{
    if (tv) {
        tv->tv_sec = 0;
        tv->tv_usec = 0;
    }
    return 0;
}

time_t time(time_t *t)
{
    time_t now = 0;
    if (t) *t = now;
    return now;
}

// ============================================================================
// ALTERNATE SYMBOL NAMES
// ============================================================================

// Some programs look for these with underscores
void* __xmalloc(size_t size) { return xmalloc(size); }
void* __xcalloc(size_t n, size_t s) { return xcalloc(n, s); }
void* __xrealloc(void *p, size_t s) { return xrealloc(p, s); }
void __xalloc_die(void) { xalloc_die(); }
const char* __getprogname(void) { return getprogname(); }
