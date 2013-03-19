XEN_ROOT = $(CURDIR)/../../..
OCAML_TOPLEVEL = $(CURDIR)/..
include $(OCAML_TOPLEVEL)/common.make

OCAMLINCLUDE += \
	-I $(OCAML_TOPLEVEL)/libs/xentoollog \
	-I $(OCAML_TOPLEVEL)/libs/xl

OBJS = xtl send_debug_keys list_domains raise_exception

PROGRAMS = xtl send_debug_keys list_domains raise_exception

xtl_LIBS =  \
	-ccopt -L -ccopt $(OCAML_TOPLEVEL)/libs/xentoollog $(OCAML_TOPLEVEL)/libs/xentoollog/xentoollog.cmxa

xtl_OBJS = xtl

send_debug_keys_LIBS =  \
	-ccopt -L -ccopt $(OCAML_TOPLEVEL)/libs/xentoollog $(OCAML_TOPLEVEL)/libs/xentoollog/xentoollog.cmxa \
	-ccopt -L -ccopt $(OCAML_TOPLEVEL)/libs/xl $(OCAML_TOPLEVEL)/libs/xl/xenlight.cmxa

send_debug_keys_OBJS = send_debug_keys

list_domains_LIBS =  \
	-ccopt -L -ccopt $(OCAML_TOPLEVEL)/libs/xentoollog $(OCAML_TOPLEVEL)/libs/xentoollog/xentoollog.cmxa \
	-ccopt -L -ccopt $(OCAML_TOPLEVEL)/libs/xl $(OCAML_TOPLEVEL)/libs/xl/xenlight.cmxa

list_domains_OBJS = list_domains

raise_exception_LIBS =  \
	-ccopt -L -ccopt $(OCAML_TOPLEVEL)/libs/xentoollog $(OCAML_TOPLEVEL)/libs/xentoollog/xentoollog.cmxa \
	-ccopt -L -ccopt $(OCAML_TOPLEVEL)/libs/xl $(OCAML_TOPLEVEL)/libs/xl/xenlight.cmxa

raise_exception_OBJS = raise_exception

OCAML_PROGRAM = xtl send_debug_keys list_domains raise_exception

all: $(PROGRAMS)

bins: $(PROGRAMS)

install: all
	$(INSTALL_DIR) $(DESTDIR)$(BINDIR)
	$(INSTALL_PROG) $(PROGRAMS) $(DESTDIR)$(BINDIR)

include $(OCAML_TOPLEVEL)/Makefile.rules
