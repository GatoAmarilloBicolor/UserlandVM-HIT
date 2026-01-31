set pagination off
set logging on
set logging file debug_tls.log

# Break en TLS/FS MOV Load error
break InterpreterX86_32.cpp:300

commands
  printf "=== TLS/FS MOV Load ERROR ===\n"
  printf "EIP: 0x%08x\n", $eip
  x/4i $eip
  continue
end

# Ejecutar con top que falla TLS
run sysroot/haiku32/bin/top
quit
