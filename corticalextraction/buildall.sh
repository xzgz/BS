# Copyright (C) 2016 The Regents of the University of California
# Authored by David W. Shattuck, Ph.D.
#!/bin/bash
export MACHTYPE
make -C ../vol3d $@
make -C ../surflib $@
make -C bse $@
make -C pvc $@
make -C bfc $@
make -C cortex $@
make -C scrubmask $@
make -C tca $@
make -C dewisp $@
make -C dfs $@
make -C pialmesh $@
make -C hemisplit $@
make -C skullfinder $@
