
import re

with open("syscall_table.h", "r") as f:
    content = f.read()

# Match entries in kExtendedSyscallInfos
# Simplified regex: look for "_kern_..." followed by its argument count
matches = re.findall(r'\"(_kern_[a-z0-9_]+)\",\s*(\d+)', content)

for i, (name, args) in enumerate(matches):
    print(f"{i}: {name}")
