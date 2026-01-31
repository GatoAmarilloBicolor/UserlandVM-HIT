#ifndef SYSCALL_IPC_H
#define SYSCALL_IPC_H

#include <SupportDefs.h>

// El nombre público del port que el anfitrión (servidor) creará.
// El cargador (cliente) lo buscará por este nombre.
#define USERLAND_VM_SYSCALL_PORT_NAME "userland_vm_syscall_port"

// Estructura para la petición de syscall que el cliente envía al servidor.
struct SyscallRequest {
	port_id		reply_port;   // El port donde el servidor debe enviar la respuesta.
	uint32		syscall;      // El número de la syscall a ejecutar.
	uint64		args[6];      // Los argumentos de la syscall (extraídos de los registros).
};

// Estructura para la respuesta que el servidor envía de vuelta al cliente.
struct SyscallReply {
	uint64		return_value; // El valor de retorno de la syscall ejecutada.
};

#endif // SYSCALL_IPC_H
