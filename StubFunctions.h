/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#pragma once

#include <SupportDefs.h>

class X86_32GuestContext;
class AddressSpace;

/**
 * StubFunctions - Implementations of GNU libc stub functions
 * 
 * These are minimal implementations of functions that are commonly used by
 * GNU coreutils and other programs. Most return dummy values to allow
 * programs to continue execution.
 */
class StubFunctions {
public:
    // Memory allocation
    static status_t xmalloc(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t xcalloc(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t xrealloc(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t xcharalloc(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t xmemdup(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t x2nrealloc(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t xireallocarray(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t xreallocarray(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t ximalloc(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t xicalloc(X86_32GuestContext& ctx, AddressSpace& space);

    // Error handling
    static status_t error(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t xalloc_die(X86_32GuestContext& ctx, AddressSpace& space);

    // Quoting functions
    static status_t quote_quoting_options(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t quotearg_alloc_mem(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t quotearg_n_custom_mem(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t quotearg_n_custom(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t quotearg_n_mem(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t quotearg_n(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t quotearg_char_mem(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t quotearg_char(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t quotearg_colon(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t quotearg_n_style(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t quotearg_n_style_mem(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t quote_n(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t quote_n_mem(X86_32GuestContext& ctx, AddressSpace& space);

    // Version and program functions
    static status_t set_program_name(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t getprogname(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t version_etc(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t version_etc_arn(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t version_etc_va(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t usage(X86_32GuestContext& ctx, AddressSpace& space);

    // Locale and encoding
    static status_t locale_charset(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t hard_locale(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t setlocale_null_r(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t rpl_nl_langinfo(X86_32GuestContext& ctx, AddressSpace& space);

    // Replacement functions (rpl_*)
    static status_t rpl_malloc(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t rpl_calloc(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t rpl_realloc(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t rpl_free(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t rpl_mbrtowc(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t rpl_fclose(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t rpl_fflush(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t rpl_fseeko(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t rpl_vfprintf(X86_32GuestContext& ctx, AddressSpace& space);

    // Miscellaneous
    static status_t close_stdout(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t version_etc_copyright(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t error_message_count(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t error_print_progname(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t program_name(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t exit_failure(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t thrd_exit(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t Version(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t error_one_per_line(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t set_char_quoting(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t set_custom_quoting(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t printf_parse(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t printf_fetchargs(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t vasnprintf(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t fseterr(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t close_stream(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t globfree(X86_32GuestContext& ctx, AddressSpace& space);
    static status_t gl_get_setlocale_null_lock(X86_32GuestContext& ctx, AddressSpace& space);
};
