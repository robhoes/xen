open Printf
open Xentoollog
open Xenlight

let _ = 
  let logger = Xentoollog.create_stdio_logger (*~level:Xentoollog.Debug*) () in
  let ctx = Xenlight.ctx_alloc logger in
  try
    Xenlight.test_raise_exception ()
  with Xenlight.Error(err, fn) -> begin
    printf "Caught Exception: %s: %s\n" (Xenlight.string_of_error err) fn;
  end;
  Xenlight.ctx_free ctx;
  Xentoollog.destroy logger;

