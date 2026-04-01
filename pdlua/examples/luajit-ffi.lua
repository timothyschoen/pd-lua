local ffi = require("ffi")

ffi.cdef[[
void post(const char *fmt, ...);
float sys_getsr(void);
double clock_getlogicaltime(void);
]]

ffi.C.post("=== Pd via LuaJIT FFI ===")
ffi.C.post("Samplerate: " .. tostring(ffi.C.sys_getsr()))
ffi.C.post("Block size: " .. tostring(ffi.C.sys_getblksize()))
ffi.C.post("Time: " ..  tostring(ffi.C.clock_getlogicaltime()))
