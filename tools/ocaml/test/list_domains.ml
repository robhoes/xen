open Arg
open Printf
open Xenlight

let bool_as_char b c = if b then c else '-'

let print_dominfo dominfo =
  let id = dominfo.Xenlight.Dominfo.xl_domid
  and running = bool_as_char dominfo.Xenlight.Dominfo.xl_running 'r'
  and blocked = bool_as_char dominfo.Xenlight.Dominfo.xl_blocked 'b'
  and paused = bool_as_char dominfo.Xenlight.Dominfo.xl_paused 'p'
  and shutdown = bool_as_char dominfo.Xenlight.Dominfo.xl_shutdown 's'
  and dying = bool_as_char dominfo.Xenlight.Dominfo.xl_dying 'd'
  and memory = dominfo.Xenlight.Dominfo.xl_current_memkb
  in
  printf "Dom %d: %c%c%c%c%c %LdKB\n" id running blocked paused shutdown dying memory

let _ =
  let logger = Xtl.create_stdio_logger (*~level:Xentoollog.Debug*) () in
  let ctx = Xenlight.ctx_alloc logger in
  try
    let domains = Xenlight.Dominfo.list ctx in
    List.iter (fun d -> print_dominfo d) domains
  with Xenlight.Error(err, fn) -> begin
    printf "Caught Exception: %s: %s\n" (Xenlight.string_of_error err) fn;
  end


