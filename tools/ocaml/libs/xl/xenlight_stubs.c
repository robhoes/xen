/*
 * Copyright (C) 2009-2011 Citrix Ltd.
 * Author Vincent Hanquez <vincent.hanquez@eu.citrix.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; version 2.1 only. with the special
 * exception on linking described in file LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 */

#include <stdlib.h>

#define CAML_NAME_SPACE
#include <caml/alloc.h>
#include <caml/memory.h>
#include <caml/signals.h>
#include <caml/fail.h>
#include <caml/callback.h>

#include <sys/mman.h>
#include <stdint.h>
#include <string.h>

#include <libxl.h>
#include <libxl_utils.h>

#include <unistd.h>

#include "poll_stubs.h"

#define CTX ((libxl_ctx *)ctx)

static char * dup_String_val(value s)
{
	int len;
	char *c;
	len = caml_string_length(s);
	c = calloc(len + 1, sizeof(char));
	if (!c)
		caml_raise_out_of_memory();
	memcpy(c, String_val(s), len);
	return c;
}

static value Val_error(int error)
{
	switch (error) {
	case ERROR_NONSPECIFIC: return Val_int(0);
	case ERROR_VERSION:     return Val_int(1);
	case ERROR_FAIL:        return Val_int(2);
	case ERROR_NI:          return Val_int(3);
	case ERROR_NOMEM:       return Val_int(4);
	case ERROR_INVAL:       return Val_int(5);
	case ERROR_BADFAIL:     return Val_int(6);
	case ERROR_GUEST_TIMEDOUT: return Val_int(7);
	case ERROR_TIMEDOUT:    return Val_int(8);
	case ERROR_NOPARAVIRT:  return Val_int(9);
	case ERROR_NOT_READY:   return Val_int(10);
	case ERROR_OSEVENT_REG_FAIL: return Val_int(11);
	case ERROR_BUFFERFULL:  return Val_int(12);
	case ERROR_UNKNOWN_CHILD: return Val_int(13);
#if 0 /* Let the compiler catch this */
	default:
		caml_raise_sys_error(caml_copy_string("Unknown libxl ERROR"));
		break;
#endif
	}
	/* Should not reach here */
	abort();
}

static void failwith_xl(int error, char *fname)
{
	CAMLlocal1(arg);
	value *exc = caml_named_value("Xenlight.Error");

	if (!exc)
		caml_invalid_argument("Exception Xenlight.Error not initialized, please link xl.cma");

	arg = caml_alloc_small(2, 0);

	Field(arg, 0) = Val_error(error);
	Field(arg, 1) = caml_copy_string(fname);

	caml_raise_with_arg(*exc, arg);
}

CAMLprim value stub_raise_exception(value unit)
{
	CAMLparam1(unit);
	failwith_xl(ERROR_FAIL, "test exception");
	CAMLreturn(Val_unit);
}

CAMLprim value stub_libxl_ctx_alloc(value logger)
{
	CAMLparam1(logger);
	libxl_ctx *ctx;
	int ret;

	ret = libxl_ctx_alloc(&ctx, LIBXL_VERSION, 0, (struct xentoollog_logger *) logger);
	if (ret != 0) \
		failwith_xl(ERROR_FAIL, "cannot init context");
	CAMLreturn((value)ctx);
}

CAMLprim value stub_libxl_ctx_free(value ctx)
{
	CAMLparam1(ctx);
	libxl_ctx_free(CTX);
	CAMLreturn(Val_unit);
}

static int list_len(value v)
{
	int len = 0;
	while ( v != Val_emptylist ) {
		len++;
		v = Field(v, 1);
	}
	return len;
}

static int libxl_key_value_list_val(libxl_key_value_list *c_val,
				    value v)
{
	CAMLparam1(v);
	CAMLlocal1(elem);
	int nr, i;
	libxl_key_value_list array;

	nr = list_len(v);

	array = calloc((nr + 1) * 2, sizeof(char *));
	if (!array)
		caml_raise_out_of_memory();

	for (i=0; v != Val_emptylist; i++, v = Field(v, 1) ) {
		elem = Field(v, 0);

		array[i * 2] = dup_String_val(Field(elem, 0));
		array[i * 2 + 1] = dup_String_val(Field(elem, 1));
	}

	*c_val = array;
	CAMLreturn(0);
}

static int libxl_string_list_val(libxl_string_list *c_val, value v)
{
	CAMLparam1(v);
	int nr, i;
	libxl_key_value_list array;

	nr = list_len(v);

	array = calloc(nr + 1, sizeof(char *));
	if (!array)
		caml_raise_out_of_memory();

	for (i=0; v != Val_emptylist; i++, v = Field(v, 1) )
		array[i] = dup_String_val(Field(v, 0));

	*c_val = array;
	CAMLreturn(0);
}

/* Option type support as per http://www.linux-nantes.org/~fmonnier/ocaml/ocaml-wrapping-c.php */
#define Val_none Val_int(0)
#define Some_val(v) Field(v,0)

static value Val_some(value v)
{
	CAMLparam1(v);
	CAMLlocal1(some);
	some = caml_alloc(1, 0);
	Store_field(some, 0, v);
	CAMLreturn(some);
}

static value Val_mac (libxl_mac *c_val)
{
	CAMLparam0();
	CAMLlocal1(v);
	int i;

	v = caml_alloc_tuple(6);

	for(i=0; i<6; i++)
		Store_field(v, i, Val_int((*c_val)[i]));

	CAMLreturn(v);
}

static int Mac_val(libxl_mac *c_val, value v)
{
	CAMLparam1(v);
	int i;

	for(i=0; i<6; i++)
		(*c_val)[i] = Int_val(Field(v, i));

	CAMLreturn(0);
}

static value Val_bitmap (libxl_bitmap *c_val)
{
	CAMLparam0();
	CAMLlocal1(v);
	int i;

	v = caml_alloc(8 * (c_val->size), 0);
	libxl_for_each_bit(i, *c_val) {
		if (libxl_bitmap_test(c_val, i))
			Store_field(v, i, Val_true);
		else
			Store_field(v, i, Val_false);
	}
	CAMLreturn(v);
}

static int Bitmap_val(libxl_ctx *ctx, libxl_bitmap *c_val, value v)
{
	CAMLparam1(v);
	int i, len = Wosize_val(v);

	c_val->size = 0;
	if (len > 0 && !libxl_bitmap_alloc(ctx, c_val, len))
		failwith_xl(ERROR_NOMEM, "cannot allocate bitmap");
	for (i=0; i<len; i++) {
		if (Int_val(Field(v, i)))
			libxl_bitmap_set(c_val, i);
		else
			libxl_bitmap_reset(c_val, i);
	}
	CAMLreturn(0);
}

static value Val_uuid (libxl_uuid *c_val)
{
	CAMLparam0();
	CAMLlocal1(v);
	uint8_t *uuid = libxl_uuid_bytearray(c_val);
	char buf[LIBXL_UUID_FMTLEN+1];

	sprintf(buf, LIBXL_UUID_FMT, LIBXL__UUID_BYTES(uuid));
	v = caml_copy_string(buf);

	CAMLreturn(v);
}

static int Uuid_val(libxl_uuid *c_val, value v)
{
	CAMLparam1(v);
	int i;

	libxl_uuid_from_string(c_val, dup_String_val(v));

	CAMLreturn(0);
}

static value Val_defbool(libxl_defbool c_val)
{
	CAMLparam0();
	CAMLlocal1(v);

	if (libxl_defbool_is_default(c_val))
		v = Val_none;
	else {
		bool b = libxl_defbool_val(c_val);
		v = Val_some(b ? Val_bool(true) : Val_bool(false));
	}
	CAMLreturn(v);
}

static libxl_defbool Defbool_val(value v)
{
	CAMLparam1(v);
	libxl_defbool db;
	if (v == Val_none)
		libxl_defbool_unset(&db);
	else {
		bool b = Bool_val(Some_val(v));
		libxl_defbool_set(&db, b);
	}
	return db;
}

static value Val_hwcap(libxl_hwcap *c_val)
{
	CAMLparam0();
	CAMLlocal1(hwcap);
	int i;

	hwcap = caml_alloc_tuple(8);
	for (i = 0; i < 8; i++)
		Store_field(hwcap, i, caml_copy_int32((*c_val)[i]));

	CAMLreturn(hwcap);
}

static value Val_string_option(char *c_val)
{
	CAMLparam0();
	if (c_val)
		CAMLreturn(Val_some(caml_copy_string(c_val)));
	else
		CAMLreturn(Val_none);
}

static char *String_option_val(value v)
{
	char *s = NULL;
	if (v != Val_none)
		s = dup_String_val(Some_val(v));
	return s;
}

#include "_libxl_types.inc"

static int domain_wait_event(libxl_ctx *ctx, int domid, libxl_event **event_r)
{
	int ret;
	for (;;) {
		ret = libxl_event_wait(ctx, event_r, LIBXL_EVENTMASK_ALL, 0,0);
		if (ret) {
			return ret;
		}
		if ((*event_r)->domid != domid) {
			char *evstr = libxl_event_to_json(CTX, *event_r);
			free(evstr);
			libxl_event_free(CTX, *event_r);
			continue;
		}
		return ret;
	}
}

value stub_xl_domain_create_new(value ctx, value domain_config)
{
	CAMLparam2(ctx, domain_config);
	int ret;
	libxl_domain_config c_dconfig;
	uint32_t c_domid;

	libxl_domain_config_init(&c_dconfig);
	ret = domain_config_val(CTX, &c_dconfig, domain_config);
	if (ret != 0)
		failwith_xl(ret, "domain_create_new");

	ret = libxl_domain_create_new(CTX, &c_dconfig, &c_domid, NULL, NULL);
	if (ret != 0)
		failwith_xl(ret, "domain_create_new");

	libxl_domain_config_dispose(&c_dconfig);

	CAMLreturn(Val_int(c_domid));
}

value stub_xl_domain_create_restore(value ctx, value domain_config, value restore_fd)
{
	CAMLparam2(ctx, domain_config);
	int ret;
	libxl_domain_config c_dconfig;
	uint32_t c_domid;

	ret = domain_config_val(CTX, &c_dconfig, domain_config);
	if (ret != 0)
		failwith_xl(ret, "domain_create_restore");

	ret = libxl_domain_create_restore(CTX, &c_dconfig, &c_domid, Int_val(restore_fd), NULL, NULL);
	if (ret != 0)
		failwith_xl(ret, "domain_create_restore");

	libxl_domain_config_dispose(&c_dconfig);

	CAMLreturn(Val_int(c_domid));
}

value stub_libxl_domain_wait_shutdown(value ctx, value domid)
{
	CAMLparam2(ctx, domid);
	int ret;
	libxl_event *event;
	libxl_evgen_domain_death *deathw;
	ret = libxl_evenable_domain_death(CTX, Int_val(domid), 0, &deathw);
	if (ret) {
		fprintf(stderr,"wait for death failed (evgen, rc=%d)\n",ret);
		exit(-1);
	}

	for (;;) {
		ret = domain_wait_event(CTX, Int_val(domid), &event);
		if (ret)
			failwith_xl(ret, "domain_shutdown");

		switch (event->type) {
		case LIBXL_EVENT_TYPE_DOMAIN_DEATH:
			goto done;
		case LIBXL_EVENT_TYPE_DOMAIN_SHUTDOWN:
			goto done;
		default:
			break;
		}
		libxl_event_free(CTX, event);
	}
done:
	libxl_event_free(CTX, event);
	libxl_evdisable_domain_death(CTX, deathw);

	CAMLreturn(Val_unit);
}

value stub_libxl_domain_shutdown(value ctx, value domid)
{
	CAMLparam2(ctx, domid);
	int ret;

	ret = libxl_domain_shutdown(CTX, Int_val(domid));

	if (ret != 0)
		failwith_xl(ret, "domain_shutdown");

	CAMLreturn(Val_unit);
}

value stub_libxl_domain_reboot(value ctx, value domid)
{
	CAMLparam2(ctx, domid);
	int ret;

	ret = libxl_domain_reboot(CTX, Int_val(domid));

	if (ret != 0)
		failwith_xl(ret, "domain_reboot");

	CAMLreturn(Val_unit);
}

value stub_libxl_domain_destroy(value ctx, value domid)
{
	CAMLparam2(ctx, domid);
	int ret;

	ret = libxl_domain_destroy(CTX, Int_val(domid), 0);

	if (ret != 0)
		failwith_xl(ret, "domain_destroy");

	CAMLreturn(Val_unit);
}

value stub_libxl_domain_suspend(value ctx, value domid, value fd)
{
	CAMLparam3(ctx, domid, fd);
	int ret;

	ret = libxl_domain_suspend(CTX, Int_val(domid), Int_val(fd), 0, 0);

	if (ret != 0)
		failwith_xl(ret, "domain_suspend");

	CAMLreturn(Val_unit);
}

value stub_libxl_domain_pause(value ctx, value domid)
{
	CAMLparam2(ctx, domid);
	int ret;

	ret = libxl_domain_pause(CTX, Int_val(domid));

	if (ret != 0)
		failwith_xl(ret, "domain_pause");

	CAMLreturn(Val_unit);
}

value stub_libxl_domain_unpause(value ctx, value domid)
{
	CAMLparam2(ctx, domid);
	int ret;

	ret = libxl_domain_unpause(CTX, Int_val(domid));

	if (ret != 0)
		failwith_xl(ret, "domain_unpause");

	CAMLreturn(Val_unit);
}

void async_callback(libxl_ctx *ctx, int rc, void *for_callback)
{
	int *task = (int *) for_callback;
	value *func = caml_named_value("xl_async_callback");
	caml_callback2(*func, (value) for_callback, Val_int(rc));
}

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#define _DEVICE_ADDREMOVE(type,fn,op)					\
value stub_xl_device_##type##_##op(value ctx, value async, value info,	\
	value domid)							\
{									\
	CAMLparam4(ctx, info, domid, async);				\
	libxl_device_##type c_info;					\
	int ret, marker_var;						\
	libxl_asyncop_how *ao_how;					\
									\
	device_##type##_val(CTX, &c_info, info);			\
									\
	if (async != Val_none) {					\
		ao_how = malloc(sizeof(*ao_how));			\
		ao_how->callback = async_callback;			\
		ao_how->u.for_callback = (void *) Some_val(async);	\
	}								\
	else								\
		ao_how = NULL;						\
									\
	ret = libxl_##fn##_##op(CTX, Int_val(domid), &c_info,		\
		ao_how);						\
									\
	libxl_device_##type##_dispose(&c_info);				\
	if (ao_how)							\
		free(ao_how);						\
									\
	if (ret != 0)							\
		failwith_xl(ret, STRINGIFY(type) "_" STRINGIFY(op));	\
									\
	CAMLreturn(Val_unit);						\
}

#define DEVICE_ADDREMOVE(type) \
	_DEVICE_ADDREMOVE(type, device_##type, add) \
 	_DEVICE_ADDREMOVE(type, device_##type, remove) \
 	_DEVICE_ADDREMOVE(type, device_##type, destroy)

DEVICE_ADDREMOVE(disk)
DEVICE_ADDREMOVE(nic)
DEVICE_ADDREMOVE(vfb)
DEVICE_ADDREMOVE(vkb)
DEVICE_ADDREMOVE(pci)
_DEVICE_ADDREMOVE(disk, cdrom, insert)

value stub_xl_device_nic_of_devid(value ctx, value domid, value devid)
{
	CAMLparam3(ctx, domid, devid);
	libxl_device_nic nic;
	libxl_devid_to_device_nic(CTX, Int_val(domid), Int_val(devid), &nic);
	CAMLreturn(Val_device_nic(&nic));
}

value stub_xl_device_nic_list(value ctx, value domid)
{
	CAMLparam2(ctx, domid);
	CAMLlocal2(list, temp);
	libxl_device_nic *c_list;
	int i, nb;
	uint32_t c_domid;

	c_domid = Int_val(domid);

	c_list = libxl_device_nic_list(CTX, c_domid, &nb);
	if (!c_list && nb > 0)
		failwith_xl(ERROR_FAIL, "nic_list");

	list = temp = Val_emptylist;
	for (i = 0; i < nb; i++) {
		list = caml_alloc_small(2, Tag_cons);
		Field(list, 0) = Val_int(0);
		Field(list, 1) = temp;
		temp = list;
		Store_field(list, 0, Val_device_nic(&c_list[i]));
		libxl_device_nic_dispose(&c_list[i]);
	}
	free(c_list);

	CAMLreturn(list);
}

value stub_xl_device_disk_of_vdev(value ctx, value domid, value vdev)
{
	CAMLparam3(ctx, domid, vdev);
	libxl_device_disk disk;
	libxl_vdev_to_device_disk(CTX, Int_val(domid), String_val(vdev), &disk);
	CAMLreturn(Val_device_disk(&disk));
}

value stub_xl_device_pci_list(value ctx, value domid)
{
	CAMLparam2(ctx, domid);
	CAMLlocal2(list, temp);
	libxl_device_pci *c_list;
	int i, nb;
	uint32_t c_domid;

	c_domid = Int_val(domid);

	c_list = libxl_device_pci_list(CTX, c_domid, &nb);
	if (!c_list && nb > 0)
		failwith_xl(ERROR_FAIL, "pci_list");

	list = temp = Val_emptylist;
	for (i = 0; i < nb; i++) {
		list = caml_alloc_small(2, Tag_cons);
		Field(list, 0) = Val_int(0);
		Field(list, 1) = temp;
		temp = list;
		Store_field(list, 0, Val_device_pci(&c_list[i]));
		libxl_device_pci_dispose(&c_list[i]);
	}
	free(c_list);

	CAMLreturn(list);
}

value stub_xl_device_pci_assignable_add(value ctx, value info, value rebind)
{
	CAMLparam3(ctx, info, rebind);
	libxl_device_pci c_info;
	int ret, marker_var;

	device_pci_val(CTX, &c_info, info);

	ret = libxl_device_pci_assignable_add(CTX, &c_info, (int) Bool_val(rebind));

	libxl_device_pci_dispose(&c_info);

	if (ret != 0)
		failwith_xl(ret, "pci_assignable_add");

	CAMLreturn(Val_unit);
}

value stub_xl_device_pci_assignable_remove(value ctx, value info, value rebind)
{
	CAMLparam3(ctx, info, rebind);
	libxl_device_pci c_info;
	int ret, marker_var;

	device_pci_val(CTX, &c_info, info);

	ret = libxl_device_pci_assignable_remove(CTX, &c_info, (int) Bool_val(rebind));

	libxl_device_pci_dispose(&c_info);

	if (ret != 0)
		failwith_xl(ret, "pci_assignable_remove");

	CAMLreturn(Val_unit);
}

value stub_xl_device_pci_assignable_list(value ctx)
{
	CAMLparam1(ctx);
	CAMLlocal2(list, temp);
	libxl_device_pci *c_list;
	int i, nb;
	uint32_t c_domid;

	c_list = libxl_device_pci_assignable_list(CTX, &nb);
	if (!c_list && nb > 0)
		failwith_xl(ERROR_FAIL, "pci_assignable_list");

	list = temp = Val_emptylist;
	for (i = 0; i < nb; i++) {
		list = caml_alloc_small(2, Tag_cons);
		Field(list, 0) = Val_int(0);
		Field(list, 1) = temp;
		temp = list;
		Store_field(list, 0, Val_device_pci(&c_list[i]));
		libxl_device_pci_dispose(&c_list[i]);
	}
	free(c_list);

	CAMLreturn(list);
}

value stub_xl_physinfo_get(value ctx)
{
	CAMLparam1(ctx);
	CAMLlocal1(physinfo);
	libxl_physinfo c_physinfo;
	int ret;

	ret = libxl_get_physinfo(CTX, &c_physinfo);

	if (ret != 0)
		failwith_xl(ret, "get_physinfo");

	physinfo = Val_physinfo(&c_physinfo);

	libxl_physinfo_dispose(&c_physinfo);

	CAMLreturn(physinfo);
}

value stub_xl_cputopology_get(value ctx)
{
	CAMLparam1(ctx);
	CAMLlocal2(topology, v);
	libxl_cputopology *c_topology;
	int i, nr;

	c_topology = libxl_get_cpu_topology(CTX, &nr);

	if (!c_topology)
		failwith_xl(ERROR_FAIL, "get_cpu_topologyinfo");

	topology = caml_alloc_tuple(nr);
	for (i = 0; i < nr; i++) {
		if (c_topology[i].core != LIBXL_CPUTOPOLOGY_INVALID_ENTRY)
			v = Val_some(Val_cputopology(&c_topology[i]));
		else
			v = Val_none;
		Store_field(topology, i, v);
	}

	libxl_cputopology_list_free(c_topology, nr);

	CAMLreturn(topology);
}

value stub_xl_dominfo_list(value ctx)
{
	CAMLparam1(ctx);
	CAMLlocal2(domlist, temp);
	libxl_dominfo *c_domlist;
	int i, nb;

	c_domlist = libxl_list_domain(CTX, &nb);
	if (!c_domlist)
		failwith_xl(ERROR_FAIL, "dominfo_list");

	domlist = temp = Val_emptylist;
	for (i = nb - 1; i >= 0; i--) {
		domlist = caml_alloc_small(2, Tag_cons);
		Field(domlist, 0) = Val_int(0);
		Field(domlist, 1) = temp;
		temp = domlist;

		Store_field(domlist, 0, Val_dominfo(&c_domlist[i]));
	}

	libxl_dominfo_list_free(c_domlist, nb);

	CAMLreturn(domlist);
}

value stub_xl_dominfo_get(value ctx, value domid)
{
	CAMLparam2(ctx, domid);
	CAMLlocal1(dominfo);
	libxl_dominfo c_dominfo;
	int ret;

	ret = libxl_domain_info(CTX, &c_dominfo, Int_val(domid));
	if (ret != 0)
		failwith_xl(ERROR_FAIL, "domain_info");
	dominfo = Val_dominfo(&c_dominfo);

	CAMLreturn(dominfo);
}

value stub_xl_domain_sched_params_get(value ctx, value domid)
{
	CAMLparam2(ctx, domid);
	CAMLlocal1(scinfo);
	libxl_domain_sched_params c_scinfo;
	int ret;

	ret = libxl_domain_sched_params_get(CTX, Int_val(domid), &c_scinfo);
	if (ret != 0)
		failwith_xl(ret, "domain_sched_params_get");

	scinfo = Val_domain_sched_params(&c_scinfo);

	libxl_domain_sched_params_dispose(&c_scinfo);

	CAMLreturn(scinfo);
}

value stub_xl_domain_sched_params_set(value ctx, value domid, value scinfo)
{
	CAMLparam3(ctx, domid, scinfo);
	libxl_domain_sched_params c_scinfo;
	int ret;

	domain_sched_params_val(CTX, &c_scinfo, scinfo);

	ret = libxl_domain_sched_params_set(CTX, Int_val(domid), &c_scinfo);

	libxl_domain_sched_params_dispose(&c_scinfo);

	if (ret != 0)
		failwith_xl(ret, "domain_sched_params_set");

	CAMLreturn(Val_unit);
}

value stub_xl_send_trigger(value ctx, value domid, value trigger, value vcpuid)
{
	CAMLparam4(ctx, domid, trigger, vcpuid);
	int ret;
	libxl_trigger c_trigger = LIBXL_TRIGGER_UNKNOWN;

	trigger_val(CTX, &c_trigger, trigger);

	ret = libxl_send_trigger(CTX, Int_val(domid),
				 c_trigger, Int_val(vcpuid));

	if (ret != 0)
		failwith_xl(ret, "send_trigger");

	CAMLreturn(Val_unit);
}

value stub_xl_send_sysrq(value ctx, value domid, value sysrq)
{
	CAMLparam3(ctx, domid, sysrq);
	int ret;

	ret = libxl_send_sysrq(CTX, Int_val(domid), Int_val(sysrq));

	if (ret != 0)
		failwith_xl(ret, "send_sysrq");

	CAMLreturn(Val_unit);
}

value stub_xl_send_debug_keys(value ctx, value keys)
{
	CAMLparam2(ctx, keys);
	int ret;
	char *c_keys;

	c_keys = dup_String_val(keys);

	ret = libxl_send_debug_keys(CTX, c_keys);
	if (ret != 0)
		failwith_xl(ret, "send_debug_keys");

	free(c_keys);

	CAMLreturn(Val_unit);
}

value stub_xl_xen_console_read(value ctx)
{
	CAMLparam1(ctx);
	CAMLlocal3(list, cons, ml_line);
	int i = 0, ret;
	char *console[32768], *line;
	libxl_xen_console_reader *cr;

	cr = libxl_xen_console_read_start(CTX, 0);
	if (cr)
		for (i = 0; libxl_xen_console_read_line(CTX, cr, &line) > 0; i++)
			console[i] = strdup(line);
	libxl_xen_console_read_finish(CTX, cr);

	list = Val_emptylist;
	for (; i > 0; i--) {
		ml_line = caml_copy_string(console[i - 1]);
		free(console[i - 1]);
		cons = caml_alloc(2, 0);
		Store_field(cons, 0, ml_line);  // head
		Store_field(cons, 1, list);     // tail
		list = cons;
	}

	CAMLreturn(list);
}

int fd_register(void *user, int fd, void **for_app_registration_out,
                     short events, void *for_libxl)
{
	CAMLparam0();
	CAMLlocalN(args, 4);
	value *func = caml_named_value("fd_register");

	args[0] = (value) user;
	args[1] = Val_int(fd);
	args[2] = Val_poll_events(events);
	args[3] = (value) for_libxl;

	caml_callbackN(*func, 4, args);
	CAMLreturn(0);
}

int fd_modify(void *user, int fd, void **for_app_registration_update,
                   short events)
{
	CAMLparam0();
	CAMLlocalN(args, 3);
	value *func = caml_named_value("fd_modify");

	args[0] = (value) user;
	args[1] = Val_int(fd);
	args[2] = Val_poll_events(events);

	caml_callbackN(*func, 3, args);
	CAMLreturn(0);
}

void fd_deregister(void *user, int fd, void *for_app_registration)
{
	CAMLparam0();
	CAMLlocalN(args, 2);
	value *func = caml_named_value("fd_deregister");

	args[0] = (value) user;
	args[1] = Val_int(fd);

	caml_callbackN(*func, 2, args);
	CAMLreturn0;
}

int timeout_register(void *user, void **for_app_registration_out,
                          struct timeval abs, void *for_libxl)
{
	return 0;
}

int timeout_modify(void *user, void **for_app_registration_update,
                         struct timeval abs)
{
	return 0;
}

void timeout_deregister(void *user, void *for_app_registration)
{
	return;
}

value stub_xl_osevent_register_hooks(value ctx, value user)
{
	CAMLparam2(ctx, user);
	libxl_osevent_hooks *hooks;
	hooks = malloc(sizeof(*hooks));

	hooks->fd_register = fd_register;
	hooks->fd_modify = fd_modify;
	hooks->fd_deregister = fd_deregister;
	hooks->timeout_register = timeout_register;
	hooks->timeout_modify = timeout_modify;
	hooks->timeout_deregister = timeout_deregister;

	libxl_osevent_register_hooks(CTX, hooks, (void *) user);

	CAMLreturn((value) hooks);
}

value stub_xl_osevent_occurred_fd(value ctx, value for_libxl, value fd,
	value events, value revents)
{
	CAMLparam5(ctx, for_libxl, fd, events, revents);
	libxl_osevent_occurred_fd(CTX, (void *) for_libxl, Int_val(fd),
		Poll_events_val(events), Poll_events_val(revents));
	CAMLreturn(Val_unit);
}

value stub_xl_osevent_occurred_timeout(value ctx, value for_libxl)
{
	CAMLparam2(ctx, for_libxl);
	libxl_osevent_occurred_timeout(CTX, (void *) for_libxl);
	CAMLreturn(Val_unit);
}

void event_occurs(void *user, const libxl_event *event)
{
	CAMLparam0();
	CAMLlocalN(args, 2);
	value *func = caml_named_value("xl_event_occurs_callback");

	args[0] = (value) user;
	args[1] = Val_event((libxl_event *) event);
	//libxl_event_free(CTX, event); // no ctx here!

	caml_callbackN(*func, 2, args);
	CAMLreturn0;
}

void disaster(void *user, libxl_event_type type,
                     const char *msg, int errnoval)
{
	CAMLparam0();
	CAMLlocalN(args, 2);
	value *func = caml_named_value("xl_event_disaster_callback");

	args[0] = (value) user;
	args[1] = Val_event_type(type);
	args[2] = caml_copy_string(msg);
	args[3] = Val_int(errnoval);

	caml_callbackN(*func, 4, args);
	CAMLreturn0;
}

value stub_xl_event_register_callbacks(value ctx, value user)
{
	CAMLparam2(ctx, user);
	libxl_event_hooks *hooks;
	
	hooks = malloc(sizeof(*hooks));
	hooks->event_occurs_mask = LIBXL_EVENTMASK_ALL;
	hooks->event_occurs = event_occurs;
	hooks->disaster = disaster;

	libxl_event_register_callbacks(CTX, (const libxl_event_hooks *) hooks, (void *) user);

	CAMLreturn((value) hooks);
}

value stub_xl_evenable_domain_death(value ctx, value domid, value user)
{
	CAMLparam3(ctx, domid, user);
	libxl_evgen_domain_death *evgen_out;

	libxl_evenable_domain_death(CTX, Int_val(domid), Int_val(user), &evgen_out);

	CAMLreturn(Val_unit);
}

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
