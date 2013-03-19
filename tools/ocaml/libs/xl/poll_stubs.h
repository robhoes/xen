#include <caml/alloc.h>

short Poll_events_val(value event_list);
value Val_poll_events(short events);
value stub_poll(value fds);

