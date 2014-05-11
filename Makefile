#
# MDA Top level makefile
#
# This makefile can compile everything given that you have everything
# as explained to you on the wiki
#
# You should only have to change the name of the directories listed at the top 
# to change what you want to build
#

DIRS = fpga software

all:
	for dir in $(DIRS); do $(MAKE) -C $$dir; done
clean:
	for dir in $(DIRS); do $(MAKE) $@ -C $$dir; done

.PHONY: all clean $(DIRS)
