#
# BoundsChecker architecture
# Since BoundsChecker doesn't work correctly with assembly and intrinsic
# functions we need to have an own architecture, which uses spinlocks to
# be fairly fast and swaps pointers with normal C code.
#

