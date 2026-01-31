set pagination off
set logging on
set logging file debug_ls.log

# Break when EIP reaches stack area
break InterpreterX86_32.cpp:71
commands
  printf "WARNING: EIP stuck in loop or invalid area\n"
  print /x regs.eip
  continue
end

run sysroot/haiku32/bin/ls
quit
