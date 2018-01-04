LINT_DEF_FILE   := $(TOPDIR)/script/defs.lnt
LINT_SRC_FILE   := $(TOPDIR)/script/all_src.lnt
LINT_OUT        := $(TOPDIR)/script/lint_out.txt
LINT     = $(TOPDIR)/script/lintwine
CYGPATH = winepath -w
CWD	= "`pwd`"
export LINT_SRC_FILE
LINT     = $(TOPDIR)/script/lintwine
GLOBAL_DEF := -D__SSV_UNIX_SIM__
LINT_FILE_LIST = all_src
