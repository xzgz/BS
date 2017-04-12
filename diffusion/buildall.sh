# Copyright (C) 2016 The Regents of the University of California
# Authored by David W. Shattuck, Ph.D.
#!/bin/bash
export MACHTYPE
make -C ../vol3d $@
make -C ../surflib $@
make -C dwisplit $@
make -C odfmax $@
make -C htrack $@
make -C conmat $@
