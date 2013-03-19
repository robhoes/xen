open Arg
open Xentoollog
  
let do_test level = 
  let lgr = Xentoollog.create_stdio_logger ~level:level () in
  begin
    Xentoollog.test lgr;
    Xentoollog.destroy lgr;
  end

let () =
  let debug_level = ref Xentoollog.Info in
  let speclist = [
    ("-v", Arg.Unit (fun () -> debug_level := Xentoollog.Debug), "Verbose");
    ("-q", Arg.Unit (fun () -> debug_level := Xentoollog.Critical), "Quiet");
  ] in
  let usage_msg = "usage: xtl [OPTIONS]" in
  Arg.parse speclist (fun s -> ()) usage_msg;

  do_test !debug_level
