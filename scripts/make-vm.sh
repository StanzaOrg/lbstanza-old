if [ $1 == "vm" ]
then    
  stanza compiler/stz-vm-ir.stanza compiler/stz-vm-analyze.stanza compiler/stz-vm.stanza compiler/stz-vm-table.stanza compiler/stz-built-ins.stanza tests/bindings.stanza compiler/stz-vm-driver.stanza -o vm -ccfiles tests/mycfunctions.c
elif [ $1 == "gen-bindings" ]
then
  stanza compiler/stz-vm-ir.stanza compiler/stz-dl-ir.stanza compiler/stz-loaded-ids.stanza compiler/stz-vm.stanza compiler/stz-vm-table.stanza compiler/stz-vm-bindings.stanza -o vm-bindings
elif [ $1 == "bindings" ]
then
  ./vm-bindings $2 tests/bindings.stanza
fi
