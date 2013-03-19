TOPLEVEL=$(CURDIR)/../..
XEN_ROOT=$(TOPLEVEL)/../..
include $(TOPLEVEL)/common.make

CFLAGS += $(CFLAGS_libxenctrl) $(CFLAGS_libxenguest)
OCAMLINCLUDE +=

OBJS = xentoollog
INTF = xentoollog.cmi
LIBS = xentoollog.cma xentoollog.cmxa

LIBS_xentoollog = $(LDLIBS_libxenctrl)

xentoollog_OBJS = $(OBJS)
xentoollog_C_OBJS = xentoollog_stubs

OCAML_LIBRARY = xentoollog

all: $(INTF) $(LIBS)

libs: $(LIBS)

.PHONY: install
install: $(LIBS) META
	mkdir -p $(OCAMLDESTDIR)
	ocamlfind remove -destdir $(OCAMLDESTDIR) xentoollog
	ocamlfind install -destdir $(OCAMLDESTDIR) -ldconf ignore xentoollog META $(INTF) $(LIBS) *.a *.so *.cmx

.PHONY: uninstall
uninstall:
	ocamlfind remove -destdir $(OCAMLDESTDIR) xentoollog

include $(TOPLEVEL)/Makefile.rules
