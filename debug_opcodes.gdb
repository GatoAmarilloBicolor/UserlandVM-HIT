set pagination off
set logging on
set logging file debug_opcodes.log

# Break en InterpreterX86_32.cpp donde detecta UNKNOWN OPCODE
break InterpreterX86_32.cpp:200

commands
  printf "=== OPCODE UNKNOWN 0xf0 ===\n"
  print /x $eip
  print /x *((unsigned char*)$eip)
  print /x *((unsigned char*)($eip+1))
  print /x *((unsigned char*)($eip+2))
  print /x *((unsigned char*)($eip+3))
  info registers eip eax ebx ecx edx esi edi
  x/20i $eip
  continue
end

# Break en MOV Load error
break InterpreterX86_32.cpp:150
commands
  printf "=== MOV Load ERROR ===\n"
  print /x $eip
  info registers
  continue
end

# Break en TLS FS error (opcode 0x64)
break InterpreterX86_32.cpp:300
commands
  printf "=== TLS/FS OPCODE 0x64 ERROR ===\n"
  print /x $eip
  info registers
  continue
end

# Ejecutar con pwd
run sysroot/haiku32/bin/pwd
quit
