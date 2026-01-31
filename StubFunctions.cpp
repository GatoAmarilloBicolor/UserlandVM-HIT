/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "StubFunctions.h"
#include "X86_32GuestContext.h"
#include "AddressSpace.h"
#include "Haiku32SyscallDispatcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper to get argument from stack (x86-32 calling convention)
static uint32_t GetStackArg(AddressSpace& space, uint32_t esp, int arg_index) {
    uint32_t arg_addr = esp + 4 + (arg_index * 4);  // Skip return address
    uint32_t value = 0;
    space.Read(arg_addr, &value, 4);
    return value;
}

// Helper to get string from guest memory
static char* GetGuestString(AddressSpace& space, uint32_t addr, char* buffer, size_t bufsize) {
    space.ReadString(addr, buffer, bufsize);
    return buffer;
}

// Helper to write to guest memory
static void SetGuestValue(AddressSpace& space, uint32_t addr, uint32_t value) {
    space.Write(addr, &value, 4);
}

// ============================================================================
// MEMORY ALLOCATION STUBS
// ============================================================================

status_t StubFunctions::xmalloc(X86_32GuestContext& ctx, AddressSpace& space) {
    // uint32_t size = GetStackArg(space, ctx.Registers().esp, 0);
    // For stubs, just return a dummy pointer (non-zero)
    ctx.Registers().eax = 0x40050000;  // Dummy heap address
    printf("[STUB] xmalloc called, returning 0x40050000\n");
    return B_OK;
}

status_t StubFunctions::xcalloc(X86_32GuestContext& ctx, AddressSpace& space) {
    // uint32_t count = GetStackArg(space, ctx.Registers().esp, 0);
    // uint32_t size = GetStackArg(space, ctx.Registers().esp, 1);
    ctx.Registers().eax = 0x40050100;
    printf("[STUB] xcalloc called, returning 0x40050100\n");
    return B_OK;
}

status_t StubFunctions::xrealloc(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40050200;
    printf("[STUB] xrealloc called, returning 0x40050200\n");
    return B_OK;
}

status_t StubFunctions::xcharalloc(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40050300;
    printf("[STUB] xcharalloc called, returning 0x40050300\n");
    return B_OK;
}

status_t StubFunctions::xmemdup(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40050400;
    printf("[STUB] xmemdup called, returning 0x40050400\n");
    return B_OK;
}

status_t StubFunctions::x2nrealloc(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40050500;
    printf("[STUB] x2nrealloc called, returning 0x40050500\n");
    return B_OK;
}

status_t StubFunctions::xireallocarray(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40050600;
    printf("[STUB] xireallocarray called, returning 0x40050600\n");
    return B_OK;
}

status_t StubFunctions::xreallocarray(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40050700;
    printf("[STUB] xreallocarray called, returning 0x40050700\n");
    return B_OK;
}

status_t StubFunctions::ximalloc(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40050800;
    printf("[STUB] ximalloc called, returning 0x40050800\n");
    return B_OK;
}

status_t StubFunctions::xicalloc(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40050900;
    printf("[STUB] xicalloc called, returning 0x40050900\n");
    return B_OK;
}

// ============================================================================
// ERROR & OUTPUT STUBS
// ============================================================================

status_t StubFunctions::error(X86_32GuestContext& ctx, AddressSpace& space) {
    printf("[STUB] error() called (stub does nothing)\n");
    ctx.Registers().eax = 0;
    return B_OK;
}

status_t StubFunctions::xalloc_die(X86_32GuestContext& ctx, AddressSpace& space) {
    printf("[STUB] xalloc_die() called - exiting\n");
    ctx.Registers().eax = 1;
    return B_OK;
}

// ============================================================================
// QUOTING STUBS
// ============================================================================

status_t StubFunctions::quote_quoting_options(X86_32GuestContext& ctx, AddressSpace& space) {
    // Return a pointer to default quoting options
    ctx.Registers().eax = 0x40040000;
    printf("[STUB] quote_quoting_options called, returning 0x40040000\n");
    return B_OK;
}

status_t StubFunctions::quotearg_alloc_mem(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40040100;
    printf("[STUB] quotearg_alloc_mem called, returning 0x40040100\n");
    return B_OK;
}

status_t StubFunctions::quotearg_n_custom_mem(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40040200;
    printf("[STUB] quotearg_n_custom_mem called, returning 0x40040200\n");
    return B_OK;
}

status_t StubFunctions::quotearg_n_custom(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40040300;
    printf("[STUB] quotearg_n_custom called, returning 0x40040300\n");
    return B_OK;
}

status_t StubFunctions::quotearg_n_mem(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40040400;
    printf("[STUB] quotearg_n_mem called, returning 0x40040400\n");
    return B_OK;
}

status_t StubFunctions::quotearg_n(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40040500;
    printf("[STUB] quotearg_n called, returning 0x40040500\n");
    return B_OK;
}

status_t StubFunctions::quotearg_char_mem(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40040600;
    printf("[STUB] quotearg_char_mem called, returning 0x40040600\n");
    return B_OK;
}

status_t StubFunctions::quotearg_char(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40040700;
    printf("[STUB] quotearg_char called, returning 0x40040700\n");
    return B_OK;
}

status_t StubFunctions::quotearg_colon(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40040800;
    printf("[STUB] quotearg_colon called, returning 0x40040800\n");
    return B_OK;
}

status_t StubFunctions::quotearg_n_style(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40040900;
    printf("[STUB] quotearg_n_style called, returning 0x40040900\n");
    return B_OK;
}

status_t StubFunctions::quotearg_n_style_mem(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40040a00;
    printf("[STUB] quotearg_n_style_mem called, returning 0x40040a00\n");
    return B_OK;
}

status_t StubFunctions::quote_n(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40040b00;
    printf("[STUB] quote_n called, returning 0x40040b00\n");
    return B_OK;
}

status_t StubFunctions::quote_n_mem(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40040c00;
    printf("[STUB] quote_n_mem called, returning 0x40040c00\n");
    return B_OK;
}

// ============================================================================
// VERSION & PROGRAM STUBS
// ============================================================================

status_t StubFunctions::set_program_name(X86_32GuestContext& ctx, AddressSpace& space) {
    printf("[STUB] set_program_name() called (stub does nothing)\n");
    ctx.Registers().eax = 0;
    return B_OK;
}

status_t StubFunctions::getprogname(X86_32GuestContext& ctx, AddressSpace& space) {
    // Return "ls" as default program name
    ctx.Registers().eax = 0x40041000;
    printf("[STUB] getprogname called, returning 0x40041000 (\"ls\")\n");
    return B_OK;
}

status_t StubFunctions::version_etc(X86_32GuestContext& ctx, AddressSpace& space) {
    printf("[STUB] version_etc() called (stub does nothing)\n");
    ctx.Registers().eax = 0;
    return B_OK;
}

status_t StubFunctions::version_etc_arn(X86_32GuestContext& ctx, AddressSpace& space) {
    printf("[STUB] version_etc_arn() called (stub does nothing)\n");
    ctx.Registers().eax = 0;
    return B_OK;
}

status_t StubFunctions::version_etc_va(X86_32GuestContext& ctx, AddressSpace& space) {
    printf("[STUB] version_etc_va() called (stub does nothing)\n");
    ctx.Registers().eax = 0;
    return B_OK;
}

status_t StubFunctions::usage(X86_32GuestContext& ctx, AddressSpace& space) {
    printf("[STUB] usage() called (stub does nothing)\n");
    ctx.Registers().eax = 0;
    return B_OK;
}

// ============================================================================
// LOCALE & ENCODING STUBS
// ============================================================================

status_t StubFunctions::locale_charset(X86_32GuestContext& ctx, AddressSpace& space) {
    // Return "UTF-8"
    ctx.Registers().eax = 0x40042000;
    printf("[STUB] locale_charset called, returning 0x40042000 (\"UTF-8\")\n");
    return B_OK;
}

status_t StubFunctions::hard_locale(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0;  // Return 0 (not hard locale)
    printf("[STUB] hard_locale called, returning 0\n");
    return B_OK;
}

status_t StubFunctions::setlocale_null_r(X86_32GuestContext& ctx, AddressSpace& space) {
    printf("[STUB] setlocale_null_r() called (stub does nothing)\n");
    ctx.Registers().eax = 0;
    return B_OK;
}

status_t StubFunctions::rpl_nl_langinfo(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40042100;
    printf("[STUB] rpl_nl_langinfo called, returning 0x40042100\n");
    return B_OK;
}

// ============================================================================
// RPL_* REPLACEMENT STUBS
// ============================================================================

status_t StubFunctions::rpl_malloc(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40050a00;
    printf("[STUB] rpl_malloc called, returning 0x40050a00\n");
    return B_OK;
}

status_t StubFunctions::rpl_calloc(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40050b00;
    printf("[STUB] rpl_calloc called, returning 0x40050b00\n");
    return B_OK;
}

status_t StubFunctions::rpl_realloc(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40050c00;
    printf("[STUB] rpl_realloc called, returning 0x40050c00\n");
    return B_OK;
}

status_t StubFunctions::rpl_free(X86_32GuestContext& ctx, AddressSpace& space) {
    printf("[STUB] rpl_free() called (stub does nothing)\n");
    ctx.Registers().eax = 0;
    return B_OK;
}

status_t StubFunctions::rpl_mbrtowc(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0;
    printf("[STUB] rpl_mbrtowc called, returning 0\n");
    return B_OK;
}

status_t StubFunctions::rpl_fclose(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0;
    printf("[STUB] rpl_fclose called, returning 0\n");
    return B_OK;
}

status_t StubFunctions::rpl_fflush(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0;
    printf("[STUB] rpl_fflush called, returning 0\n");
    return B_OK;
}

status_t StubFunctions::rpl_fseeko(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0;
    printf("[STUB] rpl_fseeko called, returning 0\n");
    return B_OK;
}

status_t StubFunctions::rpl_vfprintf(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0;
    printf("[STUB] rpl_vfprintf called, returning 0\n");
    return B_OK;
}

// ============================================================================
// MISC STUBS
// ============================================================================

status_t StubFunctions::close_stdout(X86_32GuestContext& ctx, AddressSpace& space) {
    printf("[STUB] close_stdout() called (stub does nothing)\n");
    ctx.Registers().eax = 0;
    return B_OK;
}

status_t StubFunctions::version_etc_copyright(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40043000;
    printf("[STUB] version_etc_copyright called, returning 0x40043000\n");
    return B_OK;
}

status_t StubFunctions::error_message_count(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40043100;
    printf("[STUB] error_message_count called, returning 0x40043100\n");
    return B_OK;
}

status_t StubFunctions::error_print_progname(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40043200;
    printf("[STUB] error_print_progname called, returning 0x40043200\n");
    return B_OK;
}

status_t StubFunctions::program_name(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40043300;
    printf("[STUB] program_name called, returning 0x40043300\n");
    return B_OK;
}

status_t StubFunctions::exit_failure(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40043400;
    printf("[STUB] exit_failure called, returning 0x40043400\n");
    return B_OK;
}

status_t StubFunctions::thrd_exit(X86_32GuestContext& ctx, AddressSpace& space) {
    printf("[STUB] thrd_exit() called (stub does nothing)\n");
    ctx.Registers().eax = 0;
    return B_OK;
}

status_t StubFunctions::Version(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40043500;
    printf("[STUB] Version called, returning 0x40043500\n");
    return B_OK;
}

status_t StubFunctions::error_one_per_line(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0;
    printf("[STUB] error_one_per_line called, returning 0\n");
    return B_OK;
}

status_t StubFunctions::set_char_quoting(X86_32GuestContext& ctx, AddressSpace& space) {
    printf("[STUB] set_char_quoting() called (stub does nothing)\n");
    ctx.Registers().eax = 0;
    return B_OK;
}

status_t StubFunctions::set_custom_quoting(X86_32GuestContext& ctx, AddressSpace& space) {
    printf("[STUB] set_custom_quoting() called (stub does nothing)\n");
    ctx.Registers().eax = 0;
    return B_OK;
}

status_t StubFunctions::printf_parse(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0;
    printf("[STUB] printf_parse called, returning 0\n");
    return B_OK;
}

status_t StubFunctions::printf_fetchargs(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0;
    printf("[STUB] printf_fetchargs called, returning 0\n");
    return B_OK;
}

status_t StubFunctions::vasnprintf(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40044000;
    printf("[STUB] vasnprintf called, returning 0x40044000\n");
    return B_OK;
}

status_t StubFunctions::fseterr(X86_32GuestContext& ctx, AddressSpace& space) {
    printf("[STUB] fseterr() called (stub does nothing)\n");
    ctx.Registers().eax = 0;
    return B_OK;
}

status_t StubFunctions::close_stream(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0;
    printf("[STUB] close_stream called, returning 0\n");
    return B_OK;
}

status_t StubFunctions::globfree(X86_32GuestContext& ctx, AddressSpace& space) {
    printf("[STUB] globfree() called (stub does nothing)\n");
    ctx.Registers().eax = 0;
    return B_OK;
}

status_t StubFunctions::gl_get_setlocale_null_lock(X86_32GuestContext& ctx, AddressSpace& space) {
    ctx.Registers().eax = 0x40044100;
    printf("[STUB] gl_get_setlocale_null_lock called, returning 0x40044100\n");
    return B_OK;
}
