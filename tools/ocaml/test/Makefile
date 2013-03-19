XEN_ROOT = $(CURDIR)/../../..
OCAML_TOPLEVEL = $(CURDIR)/..
include $(OCAML_TOPLEVEL)/common.make

OCAMLINCLUDE += \
	-I $(OCAML_TOPLEVEL)/libs/xentoollog \
	-I $(OCAML_TOPLEVEL)/libs/xl

OBJS = xtl send_debug_keys list_domains

PROGRAMS = xtl send_debug_keys list_domains

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

OCAML_PROGRAM = xtl send_debug_keys list_domains

all: $(PROGRAMS)

bins: $(PROGRAMS)

install: all
	$(INSTALL_DIR) $(DESTDIR)$(BINDIR)
	$(INSTALL_PROG) $(PROGRAMS) $(DESTDIR)$(BINDIR)

include $(OCAML_TOPLEVEL)/Makefile.rules
