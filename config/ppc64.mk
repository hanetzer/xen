CONFIG_PPC := y
CONFIG_PPC_64 := y
CONFIG_PPC_$(XEN_OS) := y

CONFIG_XEN_INSTALL_SUFFIX :=

CFLAGS += #-march= -mcpu= etc

# Use only if calling $(LD) directly.
LDFLAGS_DIRECT += -EB

