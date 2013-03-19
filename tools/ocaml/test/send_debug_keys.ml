open Arg
open Printf
open Xentoollog
open Xenlight

let send_keys ctx s = 
  printf "Sending debug key %s\n" s;
  Xenlight.send_debug_keys ctx s;
  ()
  
let _ = 
  let logger = Xentoollog.create_stdio_logger () in
  let ctx = Xenlight.ctx_alloc logger in
  Arg.parse [
  ] (fun s -> send_keys ctx s) "usage: send_debug_keys <keys>";
  Xenlight.ctx_free ctx;
  Xentoollog.destroy logger
